#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef enum {
        EXPR_NUMBER,
        EXPR_BINARY,
        EXPR_UNARY,
        EXPR_CALL,
        EXPR_VAR,
        EXPR_STR
} ExprKind;

typedef struct Var {
        struct Var *next;
        TokenKind kind;
        const char *name;
        u32 length;
        u32 offset;
} Var;

typedef struct Expr {
        ExprKind kind;
        struct Expr *next;
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

                struct {
                        const char *name;
                        u32 length;
                        struct Expr *args;
                        u32 nargs;
                } call;

                struct {
                        const char *text;
                        u32 length;
                } str;
        };
} Expr;

typedef enum {
        STMT_EXPR,
        STMT_RETURN,
        STMT_IF,
        STMT_WHILE,
        STMT_FOR,
        STMT_BREAK,
        STMT_BLOCK,
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

                struct {
                        Expr *init;
                        Expr *cond;
                        Expr *inc;
                        struct Stmt *body;
                } for_stmt;
        };
} Stmt;

typedef struct Function {
        struct Function *next;
        const char *name;
        u32 length;
        Stmt *body;
        Var *locals;
        Var *params[6];
        u32 nparams;
        u32 stack_size;
} Function;

#endif
