#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef enum {
        EXPR_NUMBER,
        EXPR_BINARY
} ExprKind;

typedef struct Expr {
        ExprKind kind;
        union {
                i64 number;
                struct {
                        TokenKind op;
                        struct Expr *left;
                        struct Expr *right;
                } binary;
        };
} Expr;

#endif
