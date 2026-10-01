#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef enum {
        EXPR_NUMBER,
        EXPR_BINARY,
        EXPR_UNARY
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
                struct {
                        TokenKind op;
                        struct Expr *operand;
                } unary;
        };
} Expr;

typedef enum {
        STMT_EXPR,
        STMT_RETURN,
        STMT_BLOCK
} StmtKind;

typedef struct Stmt {
        StmtKind kind;
        struct Stmt *next;
        union {
                Expr *expr;
                struct Stmt *body;
        };
} Stmt;

#endif
