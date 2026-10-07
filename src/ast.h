#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef enum {
        EXPR_NUMBER,
        EXPR_BINARY,
        EXPR_UNARY,
        EXPR_VAR
} ExprKind;

typedef struct Var {
        struct Var *next;
        TokenKind kind;
        const char *name;
        u32 length;
        i32 offset;
} Var;

typedef struct Expr {
        ExprKind kind;
        union {
                i64 number;
                Var *var;
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
        STMT_IF,
        STMT_WHILE,
        STMT_BLOCK
} StmtKind;


typedef struct Stmt {
        StmtKind kind;
        struct Stmt *next;
        union {
                Expr *expr;
                struct Stmt *body;
                struct {
                        Expr *cond;
                        struct Stmt *then;
                        struct Stmt *els;
                } if_stmt;
                struct {
                        Expr *cond;
                        struct Stmt *body;
                } while_stmt;
        };
} Stmt;

#endif
