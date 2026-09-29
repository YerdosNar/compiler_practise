#include "lexer.h"

#include <stdio.h>

void print_token(Token t) {
        switch (t.tok_kind) {
        case TOK_EOF:
                printf("EOF: %.*s\n", (int)t.length, t.start);
                printf("LINE: %u, COL: %u\n", t.line, t.col);
                break;
        case TOK_NUMBER:
                printf("NUMBER: %.*s\n", (int)t.length, t.start);
                printf("LINE: %u, COL: %u\n", t.line, t.col);
                break;
        case TOK_LPAREN:
                printf("LPAREN: %.*s\n", (int)t.length, t.start);
                printf("LINE: %u, COL: %u\n", t.line, t.col);
                break;
        case TOK_RPAREN:
                printf("RPAREN: %.*s\n", (int)t.length, t.start);
                printf("LINE: %u, COL: %u\n", t.line, t.col);
                break;
        case TOK_PLUS:
                printf("PLUS: %.*s\n", (int)t.length, t.start);
                printf("LINE: %u, COL: %u\n", t.line, t.col);
                break;
        case TOK_MINUS:
                printf("MINUS: %.*s\n", (int)t.length, t.start);
                printf("LINE: %u, COL: %u\n", t.line, t.col);
                break;
        case TOK_SLASH:
                printf("SLASH: %.*s\n", (int)t.length, t.start);
                printf("LINE: %u, COL: %u\n", t.line, t.col);
                break;
        case TOK_STAR:
                printf("STAR: %.*s\n", (int)t.length, t.start);
                printf("LINE: %u, COL: %u\n", t.line, t.col);
                break;
        default:
                printf("Not supported: %.*s\n", (int)t.length, t.start);
                printf("LINE: %u, COL: %u\n", t.line, t.col);
                break;
        }
}

int main(int argc, char **argv) {
        if (argc < 2) {
                fprintf(stderr, "ERROR: Give a string \"+ - * / ( ) \"\n");
                return 1;
        }

        printf("TinyC start\n");
        {
                const char *src = argv[1];
                Lexer lx;
                lexer_init(&lx, src);
                while (1) {
                        Token t = lexer_next(&lx);
                        switch (t.tok_kind) {
                        case TOK_EOF:
                                return 0;
                        case TOK_ERROR:
                                fprintf(stderr, "ERROR: Not supported\n");
                                return 1;
                        default:
                                print_token(t);
                        }
                }
        }

        return 0;
}
