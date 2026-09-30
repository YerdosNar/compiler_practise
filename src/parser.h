#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

typedef struct {
        Lexer *lx;
        Token current;
} Parser;

void parser_init(Parser *p, Lexer *lx);
Expr *parse_expr(Parser *p);
Expr *parse(Parser *p);

#endif
