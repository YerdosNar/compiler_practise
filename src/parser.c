#include "ast.h"
#include "lexer.h"
#include "parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
static Stmt *parse_block(Parser *p);
static Var *find_var(Var *list, Token name);

static void *check_calloc(size_t size, const char *msg)
{
        void *ptr = calloc(1, size);
        if (!ptr) {
                perror(msg);
                exit(EXIT_FAILURE);
        }
        return ptr;
}

static bool check(Parser *p, TokenKind k) {return p->current.tok_kind == k;}

static Token advance(Parser *p)
{
        Token prev = p->current;
        p->current = lexer_next(p->lx);
        return prev;
}

static _Noreturn void error_at(Token t, const char *msg)
{
        fprintf(stderr, "ERROR: %s\n", msg);
        fprintf(stderr, "At %u:%u\n", t.line, t.col);
        exit(EXIT_FAILURE);
}

static void expect(Parser *p, TokenKind kind, const char *msg)
{
        Token t = p->current;
        if (t.tok_kind != kind) error_at(t, msg);
        advance(p);
}

static i64 token_to_long(Token t)
{
        i64 number = 0;
        for (u32 i = 0; i < t.length; i++) {
                number *= 10;
                number += (t.start[i] - '0');
        }
        return number;
}

static Expr *new_expr(ExprKind kind)
{
        Expr *e = check_calloc(sizeof(Expr), "Expr");
        e->kind = kind;
        return e;
}

static Stmt *new_stmt(StmtKind sk)
{
        Stmt *st = check_calloc(sizeof(Stmt), "Stmt");
        st->kind = sk;
        return st;
}

static Expr *new_number(i64 value)
{
        Expr *e = new_expr(EXPR_NUMBER);
        e->number = value;
        return e;
}

static Expr *new_binary(TokenKind op, Expr *left, Expr *right)
{
        Expr *e = new_expr(EXPR_BINARY);
        e->binary.op = op;
        e->binary.left = left;
        e->binary.right = right;
        return e;
}

static Expr *new_unary(TokenKind op, Expr *operand)
{
        Expr *e = new_expr(EXPR_UNARY);
        e->unary.op = op;
        e->unary.operand = operand;
        return e;
}

static Expr *parse_primary(Parser *p)
{
        if (check(p, TOK_NUMBER)) {
                Token t = advance(p);
                return new_number(token_to_long(t));
        }

        if (check(p, TOK_LPAREN)) {
                advance(p);
                Expr *e = parse_expr(p);
                expect(p, TOK_RPAREN, "Expected ')'");
                return e;
        }

        if (check(p, TOK_IDENTIFIER)) {
                Token t = advance(p);
                Var *v = find_var(p->locals, t);
                if (!v) error_at(t, "Undeclared variable");
                Expr *e = new_expr(EXPR_VAR);
                e->var = v;
                return e;
        }

        error_at(p->current, "Expected a number or '('");
}

static Expr *parse_unary(Parser *p)
{
        if (check(p, TOK_MINUS) || 
            check(p, TOK_PLUS)  ||
            check(p, TOK_NOT))
        {
                TokenKind op = advance(p).tok_kind;
                Expr *operand = parse_unary(p);
                return new_unary(op, operand);
        }
        return parse_primary(p);
}

static Expr *parse_mul(Parser *p)
{
        Expr *left = parse_unary(p);
        while (check(p, TOK_STAR) ||
               check(p, TOK_SLASH) ||
               check(p, TOK_MODULO))
        {
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_unary(p);
                left = new_binary(op, left, right);
        }
        return left;
}

static Expr *parse_add(Parser *p)
{
        Expr *left = parse_mul(p);
        while (check(p, TOK_PLUS) || 
               check(p, TOK_MINUS))
        {
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_mul(p);
                left = new_binary(op, left, right);
        }
        return left;
}

static Expr *parse_relational(Parser *p)
{
        Expr *left = parse_add(p);
        while (check(p, TOK_LT) || 
               check(p, TOK_LE) ||
               check(p, TOK_GT) ||
               check(p, TOK_GE))
        {
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_add(p);
                left = new_binary(op, left, right);
        }
        return left;
}

static Expr *parse_equality(Parser *p)
{
        Expr *left = parse_relational(p);
        while (check(p, TOK_EQ) ||
               check(p, TOK_NE))
        {
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_relational(p); 
                left = new_binary(op, left, right);
        }
        return left;
}

static Expr *parse_and(Parser *p)
{
        Expr *left = parse_equality(p);
        while (check(p, TOK_AND)) {
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_equality(p);
                left = new_binary(op, left, right);
        }
        return left;
}

static Expr *parse_or(Parser *p)
{
        Expr *left = parse_and(p);
        while (check(p, TOK_OR)) {
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_and(p);
                left = new_binary(op, left, right);
        }
        return left;
}

static Expr *parse_assign(Parser *p)
{
        Expr *left = parse_or(p);
        if (check(p, TOK_ASSIGN)) {
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_assign(p);
                return new_binary(op, left, right);
        }
        return left;
}

/* It does absolutely nothing, just exists for convenience of change */
Expr *parse_expr(Parser *p) {return parse_assign(p);}

static Var *find_var(Var *list, Token name)
{
        for (Var *v = list; v; v = v->next) {
                if (v->length == name.length && 
                   !memcmp(v->name, name.start, name.length))
                {
                        return v;
                }
        }
        return NULL;
}

static Var *declare_var(Parser *p, Token name)
{
        if (find_var(p->locals, name)) 
                error_at(name, "Redeclared variable");

        Var *v = check_calloc(sizeof(Var), "Var");
        v->name = name.start;
        v->length = name.length;
        v->offset = (p->locals ? p->locals->offset : 0) + 8;
        v->next = p->locals;
        p->locals = v;
        return v;
}

static Stmt *parse_stmt(Parser *p)
{
        if (check(p, TOK_KW_IF)) {
                advance(p);
                expect(p, TOK_LPAREN, "Expected '(' after 'if'");
                Stmt *s = new_stmt(STMT_IF);
                s->if_stmt.cond = parse_expr(p);
                expect(p, TOK_RPAREN, "Expected ')' after condition");
                s->if_stmt.then = parse_stmt(p);
                if (check(p, TOK_KW_ELSE)) {
                        advance(p);
                        s->if_stmt.els = parse_stmt(p);
                }
                return s;
        }

        if (check(p, TOK_KW_INT)) {
                advance(p);
                Token name = p->current;
                expect(p, TOK_IDENTIFIER, "Expected variable name after 'int'");
                declare_var(p, name);
                expect(p, TOK_SEMICOLON, "Expected ';' after declaration");
                return new_stmt(STMT_BLOCK);
        }

        if (check(p, TOK_KW_RETURN)) {
                advance(p);
                Stmt *s = new_stmt(STMT_RETURN);
                s->expr = parse_expr(p);
                expect(p, TOK_SEMICOLON, "Expected ';' after return");
                return s;
        }

        if (check(p, TOK_LCURLY)) return parse_block(p);

        Stmt *s = new_stmt(STMT_EXPR);
        s->expr = parse_expr(p);
        expect(p, TOK_SEMICOLON, "Expected ';' after expression");
        return s;
}

static Stmt *parse_block(Parser *p)
{
        expect(p, TOK_LCURLY, "Expected '{'");

        Stmt head = {0};
        Stmt *cur = &head;
        while (!check(p, TOK_RCURLY)) {
                if (check(p, TOK_EOF)) 
                        error_at(p->current, "Expected '}'");
                cur->next = parse_stmt(p);
                cur = cur->next;
        }
        advance(p);

        Stmt *s = new_stmt(STMT_BLOCK);
        s->body = head.next;
        return s;
}

Stmt *parse(Parser *p)
{
        Stmt *st = parse_block(p);
        expect(p, TOK_EOF, "Unexpected token after expression");
        return st;
}

void parser_init(Parser *p, Lexer *lx)
{
        p->lx = lx;
        p->current = lexer_next(lx);
        p->locals = NULL;
}

