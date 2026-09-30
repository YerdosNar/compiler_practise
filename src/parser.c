#include "ast.h"
#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

void parser_init(Parser *p, Lexer *lx)
{
        p->lx = lx;
        p->current = lexer_next(lx);
}

static Token advance(Parser *p)
{
        Token prev = p->current;
        p->current = lexer_next(p->lx);
        return prev;
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
        Expr *e = malloc(sizeof(Expr));
        if (!e) {
                perror("malloc");
                exit(EXIT_FAILURE);
        }
        e->kind = kind;
        return e;
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

static Expr *parse_primary(Parser *p)
{
        if (p->current.tok_kind == TOK_NUMBER) {
                Token t = advance(p);
                return new_number(token_to_long(t));
        }

        if (p->current.tok_kind == TOK_LPAREN) {
                advance(p);
                Expr *e = parse_expr(p);
                expect(p, TOK_RPAREN, "Expected ')'");
                return e;
        }

        error_at(p->current, "Expected a number or '('");
}

static Expr *parse_mul(Parser *p)
{
        Expr *left = parse_primary(p);
        while (p->current.tok_kind == TOK_STAR  ||
               p->current.tok_kind == TOK_SLASH ||
               p->current.tok_kind == TOK_MODULO)
        {
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_primary(p);
                left = new_binary(op, left, right);
        }
        return left;
}

static Expr *parse_add(Parser *p)
{
        Expr *left = parse_mul(p);
        while (p->current.tok_kind == TOK_PLUS || 
               p->current.tok_kind == TOK_MINUS) {
                TokenKind op = advance(p).tok_kind;
                Expr *right = parse_mul(p);
                left = new_binary(op, left, right);
        }
        return left;
}

Expr *parse_expr(Parser *p)
{
        return parse_add(p);
}

Expr *parse(Parser *p)
{
        Expr *e = parse_expr(p);
        expect(p, TOK_EOF, "Unexpected token after expression");
        return e;
}
