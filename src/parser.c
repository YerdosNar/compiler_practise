#include "ast.h"
#include "lexer.h"
#include "parser.h"

#include <stdbool.h>
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

static Expr *new_unary(TokenKind op, Expr *operand)
{
        Expr *e = new_expr(EXPR_UNARY);
        e->unary.op = op;
        e->unary.operand = operand;
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

static bool check(Parser *p, TokenKind k)
{
        return p->current.tok_kind == k;
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

Expr *parse_expr(Parser *p)
{
        return parse_or(p);
}

Expr *parse(Parser *p)
{
        Expr *e = parse_expr(p);
        expect(p, TOK_EOF, "Unexpected token after expression");
        return e;
}
