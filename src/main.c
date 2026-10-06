#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static const char *op_symbol(TokenKind k)
{
        switch (k) {
                case TOK_PLUS:          return "+";
                case TOK_MINUS:         return "-";
                case TOK_STAR:          return "*";
                case TOK_SLASH:         return "/";
                case TOK_MODULO:        return "%";
                case TOK_AND:           return "&&";
                case TOK_AMP:           return "&";
                case TOK_OR:            return "||";
                case TOK_LT:            return "<";
                case TOK_LE:            return "<=";
                case TOK_GT:            return ">";
                case TOK_GE:            return ">=";
                case TOK_EQ:            return "==";
                case TOK_NE:            return "!=";
                case TOK_NOT:           return "!";
                case TOK_ASSIGN:        return "=";
                default:                return "?";
        }
}

static void print_expr(Expr *e)
{
        switch (e->kind) {
                case EXPR_NUMBER:
                        printf("%lld", (long long)e->number);
                        break;
                case EXPR_VAR:
                        printf("%.*s", (int)e->var->length, e->var->name);
                        break;
                case EXPR_BINARY:
                        printf("(%s ", op_symbol(e->binary.op));
                        print_expr(e->binary.left);
                        printf(" ");
                        print_expr(e->binary.right);
                        printf(")");
                        break;
                case EXPR_UNARY:
                        printf("(%s ", op_symbol(e->unary.op));
                        print_expr(e->unary.operand);
                        printf(")");
                        break;
        }
}

int main(int argc, char **argv) {
        if (argc < 2) {
                fprintf(stderr, "ERROR: Give a string \"+ - * / ( ) \"\n");
                return 1;
        }

        bool ast_mode = argc >= 2 && !strcmp(argv[1], "--ast");
        const char *src = argv[ast_mode ? 2 : 1];
        if (!src) {
                fprintf(stderr, "Usage: %s [--ast] <program>\n", argv[0]);
                return 1;
        }

        Lexer lx;lexer_init(&lx, src);
        Parser p;parser_init(&p,&lx);

        if (ast_mode) {
                print_expr(parse_expr(&p));
                printf("\n");
        } else {
                Stmt *prog = parse(&p);
                codegen(prog, p.locals ? p.locals->offset : 0);
        }

        return 0;
}
