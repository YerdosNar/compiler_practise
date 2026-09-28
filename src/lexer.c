#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>

void lexer_init(Lexer *lx, const char *source) {
        lx->line = 1;
        lx->col = 1;
        lx->cur = source;
}

static char peek(Lexer *lx) { return lx->cur[0]; }

static char advance(Lexer *lx) {
        char cur = *lx->cur;
        if (cur == '\0')
                return cur;

        lx->cur++;
        if (cur == '\n') {
                /* Set to the first char in the next line */
                lx->line++;
                lx->col = 1;
        } else {
                lx->col++;
        }

        return cur;
}

Token lexer_next(Lexer *lx) {
        char c = peek(lx);

        while (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                advance(lx);
                c = peek(lx);
        }

        Token tok = {.line = lx->line, .col = lx->col, .start = lx->cur};

        if (c == '\0') {
                tok.tok_kind = TOK_EOF;
                tok.length = 0;
                return tok;
        }

        switch (c) {
        case '+':
                tok.tok_kind = TOK_PLUS;
                tok.length = 1;
                advance(lx);
                return tok;
        case '-':
                tok.tok_kind = TOK_MINUS;
                tok.length = 1;
                advance(lx);
                return tok;
        case '*':
                tok.tok_kind = TOK_STAR;
                tok.length = 1;
                advance(lx);
                return tok;
        case '/':
                tok.tok_kind = TOK_SLASH;
                tok.length = 1;
                advance(lx);
                return tok;
        case '(':
                tok.tok_kind = TOK_LPAREN;
                tok.length = 1;
                advance(lx);
                return tok;
        case ')':
                tok.tok_kind = TOK_RPAREN;
                tok.length = 1;
                advance(lx);
                return tok;
        default:
                tok.tok_kind = TOK_ERROR;
                tok.length = 0;
                fprintf(stderr, "ERROR: Not supported token: %c!\n", c);
                advance(lx);
                return tok;
        }
}
