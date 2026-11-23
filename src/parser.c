#include "include/ast.h"
#include "include/lexer.h"
#include "include/parser.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG

#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define NOC     "\033[0m"

#define IN      "IN"
#define OUT     "OUT"
#define INSIDE  "-"

#define debug(in_out) do {                                      \
        fprintf(stderr, "%-6s|", in_out);                       \
        fprintf(stderr, YELLOW "%-17s" NOC, __func__);          \
        fprintf(stderr, "|%-4d|\n", __LINE__);                  \
} while (0)

#define debug_Token(in_out, t) do {                             \
        fprintf(stderr, "%-6s|", in_out);                       \
        fprintf(stderr, YELLOW "%-17s" NOC, __func__);          \
        fprintf(stderr, "|%-4d|%-4d,%-3d|", __LINE__,           \
                        (t).line, (t).col);                     \
        fprintf(stderr, RED "%.*s\n" NOC,                       \
                        (int)(t).length, (t).start);            \
} while (0)

#define debug_Parser(in_out, p) do {                            \
        fprintf(stderr, "%-6s|", in_out);                       \
        fprintf(stderr, YELLOW "%-17s" NOC, __func__);          \
        fprintf(stderr, "|%-4d|%-4d,%-3d|", __LINE__,           \
                        (p)->current.line, (p)->current.col);   \
        fprintf(stderr, RED "%.*s\n" NOC,                       \
                        (int)(p)->current.length,               \
                        (p)->current.start);                    \
} while (0)

#else
#define debug(in_out)           ((void)0)
#define debug_Token(in_out, t)  ((void)0)
#define debug_Parser(in_out, p) ((void)0)
#endif

/* Forward declarations */
static Stmt *parse_block(Parser *p);
static Stmt *parse_stmt(Parser *p);
static Var *find_var(Var *list, Token name);
static Function *find_function(Function *list, Token name);
static Expr *new_add(Token op, Expr *left, Expr *right);

static void *check_calloc(size_t size, const char *msg)
{
        debug(IN);
        void *ptr = calloc(1, size);
        if (!ptr) {
                perror(msg);
                exit(EXIT_FAILURE);
        }
        debug(OUT);
        return ptr;
}

static bool check(Parser *p, TokenKind k) {return p->current.tok_kind == k;}

static Token advance(Parser *p)
{
        debug_Parser(IN, p);
        Token prev = p->current;
        p->current = lexer_next(p->lx);
        debug_Parser(OUT, p);
        return prev;
}

static _Noreturn void error_at(Token t, const char *fmt, ...)
{
        va_list args;

        fprintf(stderr, "ERROR: ");
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);

        fprintf(stderr, "\nAt %u:%u\n", t.line, t.col);
        exit(EXIT_FAILURE);
}

static void expect(Parser *p, TokenKind kind, const char *msg)
{
        debug(IN);
        Token t = p->current;
        if (t.tok_kind != kind) error_at(t, msg);
        advance(p);
        debug(OUT);
}

static i64 token_to_long(Token t)
{
        debug_Token(IN, t);
        i64 n = 0;
        for (u32 i = 0; i < t.length; i++) n = n * 10 + (t.start[i] - '0');
        debug_Token(OUT, t);
        return n;
}

static Type ty_int_storage = { TY_INT, NULL, 0 };
static Type *ty_int = &ty_int_storage;

static Type ty_char_storage = { TY_CHAR, NULL, 0 };
static Type *ty_char = &ty_char_storage;

static Type *pointer_to(Type *base)
{
        debug(IN);
        Type *t = check_calloc(sizeof(Type), "Type");
        t->kind = TY_PTR;
        t->base = base;
        debug(OUT);
        return t;
}

static Type *array_of(Type *base, u32 len)
{
        debug(IN);
        Type *t = check_calloc(sizeof(Type), "Type");
        t->kind = TY_ARRAY;
        t->base = base;
        t->array_len = len;
        debug(OUT);
        return t;
}

static bool is_ptr(Type *t) { return t->kind == TY_PTR; }

static u32 size_of(Type *t)
{
        debug(IN);
        if (t->kind == TY_ARRAY)
                return t->array_len * size_of(t->base);
        if (t->kind == TY_CHAR)
                return 1;
        debug(OUT);
        return 8; /* int and ptr are 8 bytes for now */
}

static Expr *new_expr(ExprKind kind)
{
        debug(IN);
        Expr *e = check_calloc(sizeof(Expr), "Expr");
        e->kind = kind;
        debug(OUT);
        return e;
}

static Stmt *new_stmt(StmtKind sk)
{
        debug(IN);
        Stmt *st = check_calloc(sizeof(Stmt), "Stmt");
        st->kind = sk;
        debug(OUT);
        return st;
}

static Expr *new_number(i64 value)
{
        debug(IN);
        Expr *e = new_expr(EXPR_NUMBER);
        e->number = value;
        e->type = ty_int;
        debug(OUT);
        return e;
}

static Expr *new_binary(TokenKind op, Expr *left, Expr *right)
{
        debug(IN);
        Expr *e = new_expr(EXPR_BINARY);
        e->binary.op = op;
        e->binary.left = left;
        e->binary.right = right;
        e->type = (op == TOK_ASSIGN) ? left->type : ty_int;
        debug(OUT);
        return e;
}

static Expr *new_unary(TokenKind op, Expr *operand)
{
        debug(IN);
        Expr *e = new_expr(EXPR_UNARY);
        e->unary.op      = op;
        e->unary.operand = operand;
        e->type = ty_int;
        debug(OUT);
        return e;
}

static Expr *new_fn_expr(Parser *p, Token t)
{
        debug_Token(IN, t);
        advance(p);
        Expr *e = new_expr(EXPR_CALL);
        e->call.name   = t.start;
        e->call.length = t.length;

        Expr head = {0};
        Expr *cur = &head;
        while (!check(p, TOK_RPAREN)) {
                debug_Token(INSIDE, t);
                if (e->call.nargs > 0)
                        expect(p, TOK_COMMA,
                                "Expected ',' between args");
                if (e->call.nargs >= 6)
                        error_at(p->current,
                                 "Too many args: %u (max 6)",
                                 e->call.nargs);
                cur->next = parse_expr(p);
                cur = cur->next;
                e->call.nargs++;
        }
        advance(p);
        e->call.args = head.next;
        e->type = ty_int;
        debug_Token(OUT, t);

        return e;
}

static Type *parse_type(Parser *p)
{
        debug_Parser(IN, p);
        Type *ty;

        if (check(p, TOK_KW_CHAR)) {
                advance(p);
                debug_Parser(INSIDE, p);
                ty = ty_char;
        } else {
                expect(p, TOK_KW_INT, "Expected a type");
                ty = ty_int;
        }

        while (check(p, TOK_STAR)) {
                advance(p);
                debug_Parser(INSIDE, p);
                ty = pointer_to(ty);
        }
        debug_Parser(OUT, p);
        return ty;
}

static Expr *parse_primary(Parser *p)
{
        debug_Parser(IN, p);

        if (check(p, TOK_NUMBER)) {
                Token t = advance(p);
                debug_Parser(INSIDE, p);
                return new_number(token_to_long(t));
        }

        if (check(p, TOK_LPAREN)) {
                advance(p);
                debug_Parser(INSIDE, p);
                Expr *e = parse_expr(p);
                expect(p, TOK_RPAREN, "Expected ')'");
                return e;
        }

        if (check(p, TOK_IDENTIFIER)) {
                Token t = advance(p);
                debug_Parser(INSIDE, p);
                if (check(p, TOK_LPAREN)) return new_fn_expr(p, t);
                Var *v = find_var(p->locals, t);
                if (!v) error_at(t, "Undeclared variable: '%.*s'",
                            (int)t.length,
                                 t.start);
                Expr *e = new_expr(EXPR_VAR);
                e->type = v->type;
                e->var = v;

                /* An array name means the address of its first element */
                if (v->type->kind == TY_ARRAY) {
                        Expr *addr = new_expr(EXPR_ADDR);
                        addr->unary.operand = e;
                        addr->type = pointer_to(v->type->base);
                        return addr;
                }
                return e;
        }

        if (check(p, TOK_CHAR_LITERAL)) {
                Token t = advance(p);
                char c = t.start[1]; /* 'a': char after the quote */
                if (c == '\\') {
                        switch (t.start[2]) {
                        case 'n': c = '\n'; break;
                        case 't': c = '\t'; break;
                        case '0': c = '\0'; break;
                        default:  c = t.start[2]; break;
                        }
                }
                return new_number(c);
        }

        if (check(p, TOK_STRING_LITERAL)) {
                Token t = advance(p);
                debug_Parser(INSIDE, p);
                Expr *e = new_expr(EXPR_STR);
                e->str.text = t.start;
                e->str.length = t.length;
                e->type = pointer_to(ty_char);
                return e;
        }

        debug_Parser(OUT, p);
        error_at(p->current, "Expected a number or '('");
}

static Expr *new_deref(Token op, Expr *operand)
{
        debug_Token(IN, op);
        if (!is_ptr(operand->type))
                error_at(op, "Cannot dereference a non-pointer");
        Expr *e = new_expr(EXPR_DEREF);
        e->unary.operand = operand;
        e->type = operand->type->base;
        debug_Token(OUT, op);
        return e;
}

/* postfix = primary ("[" expr "]")* */
static Expr *parse_postfix(Parser *p)
{
        debug_Parser(IN, p);
        Expr *e = parse_primary(p);
        while (check(p, TOK_LSQUARE)) {
                Token op = advance(p);
                debug_Parser(INSIDE, p);
                Expr *index = parse_expr(p);
                expect(p, TOK_RSQUARE, "Expected ']'");
                e = new_deref(op, new_add(op, e, index));
        }
        debug_Parser(OUT, p);
        return e;
}

static Expr *parse_unary(Parser *p)
{
        debug_Parser(IN, p);
        if (check(p, TOK_AMP)   ||
            check(p, TOK_STAR))
        {
                debug_Parser(INSIDE, p);
                Token op = advance(p);
                Expr *operand = parse_unary(p);

                if (op.tok_kind == TOK_AMP) {
                        Expr *e = new_expr(EXPR_ADDR);
                        e->unary.operand = operand;
                        e->type = pointer_to(operand->type);
                        return e;
                }

                return new_deref(op, operand);
        }

        if (check(p, TOK_MINUS) ||
            check(p, TOK_PLUS)  ||
            check(p, TOK_NOT))
        {
                debug_Parser(INSIDE, p);
                TokenKind op = advance(p).tok_kind;
                Expr *operand = parse_unary(p);
                return new_unary(op, operand);
        }

        debug_Parser(OUT, p);
        return parse_postfix(p);
}

static Expr *parse_mul(Parser *p)
{
        debug_Parser(IN, p);
        Expr *left = parse_unary(p);
        while (check(p, TOK_STAR) ||
               check(p, TOK_SLASH) ||
               check(p, TOK_MODULO))
        {
                debug_Parser(INSIDE, p);
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_unary(p);
                left = new_binary(op, left, right);
        }
        debug_Parser(OUT, p);
        return left;
}

static Expr *new_add(Token op, Expr *left, Expr *right)
{
        debug_Token(IN, op);
        if (is_ptr(right->type)) { /* 1 + p is the same as p+1 */
                Expr *tmp = left;
                left = right;
                right = tmp;
        }
        if (!is_ptr(left->type)) /* int + int */
                return new_binary(TOK_PLUS, left, right);
        if (is_ptr(right->type))
                error_at(op, "Cannot add two pointers");

        /* pointer + int: move by whole elements, not bytes */
        Expr *scaled = new_binary(TOK_STAR, right,
                                  new_number(size_of(left->type->base)));
        Expr *e = new_binary(TOK_PLUS, left, scaled);
        e->type = left->type;
        debug_Token(OUT, op);
        return e;
}

static Expr *new_sub(Token op, Expr *left, Expr *right)
{
        if (!is_ptr(left->type)) {
                if (is_ptr(right->type))
                        error_at(op,
                                "Cannot subtract a pointer from an integer");
                return new_binary(TOK_MINUS, left, right); /* int - int */
        }

        u32 size = size_of(left->type->base);
        if (is_ptr(right->type)) { /* pointer - pointer: elements apart */
                Expr *bytes = new_binary(TOK_MINUS, left, right);
                return new_binary(TOK_SLASH, bytes, new_number(size));
        }

        Expr *scaled = new_binary(TOK_STAR, right, new_number(size));
        Expr *e = new_binary(TOK_MINUS, left, scaled);
        e->type = left->type;
        return e;
}

static Expr *parse_add(Parser *p)
{
        debug_Parser(IN, p);
        Expr *left = parse_mul(p);
        while (check(p, TOK_PLUS) ||
               check(p, TOK_MINUS))
        {
                debug_Parser(INSIDE, p);
                Token op = advance(p);
                Expr *right = parse_mul(p);
                left = (op.tok_kind == TOK_PLUS) ? new_add(op, left, right)
                                                 : new_sub(op, left, right);
        }
        debug_Parser(OUT, p);
        return left;
}

static Expr *parse_relational(Parser *p)
{
        debug_Parser(IN, p);
        Expr *left = parse_add(p);
        while (check(p, TOK_LT) ||
               check(p, TOK_LE) ||
               check(p, TOK_GT) ||
               check(p, TOK_GE))
        {
                debug_Parser(INSIDE, p);
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_add(p);
                left = new_binary(op, left, right);
        }
        debug_Parser(OUT, p);
        return left;
}

static Expr *parse_equality(Parser *p)
{
        debug_Parser(IN, p);
        Expr *left = parse_relational(p);
        while (check(p, TOK_EQ) ||
               check(p, TOK_NE))
        {
                debug_Parser(INSIDE, p);
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_relational(p);
                left = new_binary(op, left, right);
        }
        debug_Parser(OUT, p);
        return left;
}

static Expr *parse_and(Parser *p)
{
        debug_Parser(IN, p);
        Expr *left = parse_equality(p);
        while (check(p, TOK_AND)) {
                debug_Parser(INSIDE, p);
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_equality(p);
                left = new_binary(op, left, right);
        }
        debug_Parser(OUT, p);
        return left;
}

static Expr *parse_or(Parser *p)
{
        debug_Parser(IN, p);
        Expr *left = parse_and(p);
        while (check(p, TOK_OR)) {
                debug_Parser(INSIDE, p);
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_and(p);
                left = new_binary(op, left, right);
        }
        debug_Parser(OUT, p);
        return left;
}

static Expr *parse_assign(Parser *p)
{
        debug_Parser(IN, p);
        Expr *left = parse_or(p);
        if (check(p, TOK_ASSIGN)) {
                debug_Parser(INSIDE, p);
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_assign(p);
                return new_binary(op, left, right);
        }
        debug_Parser(OUT, p);
        return left;
}

/* It does absolutely nothing, just exists for convenience of change */
Expr *parse_expr(Parser *p) {return parse_assign(p);}

static Var *find_var(Var *list, Token name)
{
        for (Var *v = list; v; v = v->next) {
                if (v->length == name.length &&
                   !memcmp(v->name, name.start, name.length))
                        return v;
        }
        return NULL;
}

static Var *declare_var(Parser *p, Token name, Type *type)
{
        debug_Parser(IN, p);
        if (find_var(p->locals, name))
                error_at(name, "Redeclared variable: '%.*s'",
                    (int)name.length,
                         name.start);

        Var *v    = check_calloc(sizeof(Var), "Var");
        v->name   = name.start;
        v->length = name.length;
        v->offset = (p->locals ? p->locals->offset : 0) + size_of(type);
        v->next   = p->locals;
        v->type   = type;
        p->locals = v;
        debug_Parser(OUT, p);
        return v;
}

static Stmt *parse_for(Parser *p)
{
        debug_Parser(IN, p);
        advance(p);
        expect(p, TOK_LPAREN, "Expected '(' after 'for'");
        Stmt *s = new_stmt(STMT_FOR);

        if (!check(p, TOK_SEMICOLON)) s->for_stmt.init = parse_expr(p);
        expect(p, TOK_SEMICOLON, "Expected ';' after for-initializer");

        if (!check(p, TOK_SEMICOLON)) s->for_stmt.cond = parse_expr(p);
        expect(p, TOK_SEMICOLON, "Expected ';' after for-condition");

        if (!check(p, TOK_RPAREN)) s->for_stmt.inc = parse_expr(p);
        expect(p, TOK_RPAREN, "Expected ')' after for-clauses");

        s->for_stmt.body = parse_stmt(p);
        debug_Parser(OUT, p);
        return s;
}

static Stmt *parse_while(Parser *p)
{
        debug_Parser(IN, p);
        advance(p);
        expect(p, TOK_LPAREN, "Expected '(' after 'while'");
        Stmt *s = new_stmt(STMT_WHILE);
        s->while_stmt.cond = parse_expr(p);
        expect(p, TOK_RPAREN, "Expected ')' after condition");
        s->while_stmt.body = parse_stmt(p);
        debug_Parser(OUT, p);
        return s;
}

static Stmt *parse_if(Parser *p)
{
        debug_Parser(IN, p);
        advance(p);
        expect(p, TOK_LPAREN, "Expected '(' after 'if'");
        Stmt *s = new_stmt(STMT_IF);
        s->if_stmt.cond = parse_expr(p);
        expect(p, TOK_RPAREN, "Expected ')' after condition");
        s->if_stmt.then = parse_stmt(p);
        if (check(p, TOK_KW_ELSE)) {
                advance(p);
                debug_Parser(INSIDE, p);
                s->if_stmt.els = parse_stmt(p);
        }
        debug_Parser(OUT, p);
        return s;
}

static Stmt *parse_declaration(Parser *p)
{
        debug_Parser(IN, p);
        Type *ty = parse_type(p);
        Token name = p->current;
        expect(p, TOK_IDENTIFIER, "Expected variable name after 'int'");
        if (check(p, TOK_LSQUARE)) {
                advance(p);
                debug_Parser(INSIDE, p);
                Token len = p->current;
                expect(p, TOK_NUMBER, "Expected array length");
                expect(p, TOK_RSQUARE, "Expected ']'");
                ty = array_of(ty, token_to_long(len));
        }
        declare_var(p, name, ty);
        expect(p, TOK_SEMICOLON, "Expected ';' after declaration");
        debug_Parser(OUT, p);
        return new_stmt(STMT_BLOCK);
}

static Stmt *parse_return(Parser *p)
{
        debug_Parser(IN, p);
        advance(p);
        Stmt *s = new_stmt(STMT_RETURN);
        s->expr = parse_expr(p);
        expect(p, TOK_SEMICOLON, "Expected ';' after return");
        debug_Parser(OUT, p);
        return s;
}

static Stmt *parse_break(Parser *p)
{
        debug_Parser(IN, p);
        advance(p);
        expect(p, TOK_SEMICOLON, "Expected ';' after 'break'");
        debug_Parser(OUT, p);
        return new_stmt(STMT_BREAK);
}

static Stmt *parse_stmt(Parser *p)
{
        debug_Parser(IN, p);
        TokenKind tk = p->current.tok_kind;
        switch (tk) {
                case TOK_KW_FOR:    return parse_for(p);
                case TOK_KW_WHILE:  return parse_while(p);
                case TOK_KW_IF:     return parse_if(p);
                case TOK_KW_CHAR:
                case TOK_KW_INT:    return parse_declaration(p);
                case TOK_KW_RETURN: return parse_return(p);
                case TOK_KW_BREAK:  return parse_break(p);
                case TOK_LCURLY:    return parse_block(p);

                default: {
                        Stmt *s = new_stmt(STMT_EXPR);
                        s->expr = parse_expr(p);
                        expect(p, TOK_SEMICOLON,
                               "Expected ';' after expression");
                        debug_Parser(OUT, p);
                        return s;
                }
        }
}

static Stmt *parse_block(Parser *p)
{
        debug_Parser(IN, p);
        expect(p, TOK_LCURLY, "Expected '{'");

        Stmt head = {0};
        Stmt *cur = &head;
        while (!check(p, TOK_RCURLY)) {
                debug_Parser(INSIDE, p);
                if (check(p, TOK_EOF))
                        error_at(p->current, "Expected '}'");
                cur->next = parse_stmt(p);
                cur = cur->next;
        }
        advance(p);

        Stmt *s = new_stmt(STMT_BLOCK);
        s->body = head.next;
        debug_Parser(OUT, p);
        return s;
}

static Function *find_function(Function *list, Token name)
{
        for (Function *f = list; f; f = f->next) {
                if (f->length == name.length &&
                   !memcmp(f->name, name.start, name.length))
                        return f;
        }
        return NULL;
}

static Function *parse_function(Parser *p, Function *list)
{
        debug_Parser(IN, p);
        expect(p, TOK_KW_INT, "Expected 'int' before function name");
        debug_Parser(INSIDE, p);
        while (check(p, TOK_STAR)) advance(p); /* 'int *p' is just 'int p' for now */
        debug_Parser(INSIDE, p);
        Token name = p->current;
        expect(p, TOK_IDENTIFIER, "Expected function name");
        debug_Parser(INSIDE, p);
        if (find_function(list, name))
                error_at(name, "Redefinition of function '%.*s'",
                         (int)name.length, name.start);
        expect(p, TOK_LPAREN, "Expected '(' after function name");
        debug_Parser(INSIDE, p);

        p->locals    = NULL;
        Function *fn = check_calloc(sizeof(Function), "Function");
        fn->name     = name.start;
        fn->length   = name.length;

        while (!check(p, TOK_RPAREN)) {
                debug_Parser(INSIDE, p);
                if (fn->nparams > 0)
                        expect(p, TOK_COMMA,
                               "Expected ',' between parameters");
                if (fn->nparams >= 6)
                        error_at(p->current,
                                 "Too many parameters: %u (max 6)",
                                 fn->nparams);
                Type *ty = parse_type(p);
                Token param = p->current;
                expect(p, TOK_IDENTIFIER, "Extected parameter name");
                fn->params[fn->nparams++] = declare_var(p, param, ty);
        }
        advance(p);

        fn->body = parse_block(p);
        fn->locals = p->locals;
        fn->stack_size = p->locals ? p->locals->offset : 0;

        debug_Parser(OUT, p);
        return fn;
}

Function *parse(Parser *p)
{
        debug_Parser(IN, p);
        Function head = {0};
        Function *cur = &head;
        while (!check(p, TOK_EOF)) {
                debug_Parser(INSIDE, p);
                cur->next = parse_function(p, head.next);
                cur = cur->next;
        }
        debug_Parser(OUT, p);
        return head.next;
}

void parser_init(Parser *p, Lexer *lx)
{
        p->lx = lx;
        p->current = lexer_next(lx);
        p->locals = NULL;
}
