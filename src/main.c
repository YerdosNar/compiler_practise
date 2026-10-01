#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"

#include <stdio.h>

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
                default:                return "?";
        }
}

static void print_expr(Expr *e)
{
        switch (e->kind) {
                case EXPR_NUMBER:
                        printf("%lld", (long long)e->number);
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

        u32 error_count = 0;

        Lexer lx;lexer_init(&lx, argv[1]);
        Parser p;parser_init(&p,&lx);
        Stmt *prog = parse(&p);
        codegen(prog);

        if (error_count) return 1;

        return 0;
}
