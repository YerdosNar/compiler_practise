#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>

char *tok_kind_name(TokenKind tk)
{
        switch (tk) {
                case TOK_EOF:
                        return "TOK_EOF";
                case TOK_ERROR:
                        return "TOK_ERROR";
                case TOK_NUMBER:
                        return "TOK_NUMBER";
                case TOK_IDENTIFIER:
                        return "TOK_IDENTIFIER";
                case TOK_KW_INT:
                        return "TOK_KW_INT";
                case TOK_KW_CHAR:
                        return "TOK_KW_CHAR";
                case TOK_KW_LONG:
                        return "TOK_KW_LONG";
                case TOK_KW_VOID:
                        return "TOK_KW_VOID";
                case TOK_KW_IF:
                        return "TOK_KW_IF";
                case TOK_KW_ELSE:
                        return "TOK_KW_ELSE";
                case TOK_KW_WHILE:
                        return "TOK_KW_WHILE";
                case TOK_KW_FOR:
                        return "TOK_KW_FOR";
                case TOK_KW_RETURN:
                        return "TOK_KW_RETURN";
                case TOK_PLUS:
                        return "TOK_PLUS";
                case TOK_MINUS:
                        return "TOK_MINUS";
                case TOK_STAR:
                        return "TOK_STAR";
                case TOK_SLASH:
                        return "TOK_SLASH";
                case TOK_LPAREN:
                        return "TOK_LPAREN";
                case TOK_RPAREN:
                        return "TOK_RPAREN";
                case TOK_SEMICOLON:
                        return "TOK_SEMICOLON";
                default:
                        return "Unknown";
        }
}

void print_token(Token t) {
        if (t.tok_kind == TOK_ERROR) {
                fprintf(stderr, "ERROR: Unknown token: %.*s\n", 1, t.start);
                fprintf(stderr, "LINE: %u, COL: %u\n\n", t.line, t.col);
                exit(EXIT_FAILURE);
        }
        printf("%s: %.*s\n", tok_kind_name(t.tok_kind), (int)t.length, t.start);
        printf("LINE: %u, COL: %u\n\n", t.line, t.col);
}

int main(int argc, char **argv) {
        if (argc < 2) {
                fprintf(stderr, "ERROR: Give a string \"+ - * / ( ) \"\n");
                return 1;
        }

        printf("TinyC start\n\n");
        const char *src = argv[1];
        Lexer lx;
        lexer_init(&lx, src);
        while (1) {
                Token t = lexer_next(&lx);
                if (t.tok_kind == TOK_EOF) return 0;
                print_token(t);
        }

        return 0;
}
