#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const Keyword_tok kv_array[] = {
        { .tok_kw = TOK_KW_CHAR,    .str_kw = "char"   },
        { .tok_kw = TOK_KW_INT,     .str_kw = "int"    },
        { .tok_kw = TOK_KW_LONG,    .str_kw = "long"   },
        { .tok_kw = TOK_KW_VOID,    .str_kw = "void"   },
        { .tok_kw = TOK_KW_IF,      .str_kw = "if"     },
        { .tok_kw = TOK_KW_ELSE,    .str_kw = "else"   },
        { .tok_kw = TOK_KW_WHILE,   .str_kw = "while"  },
        { .tok_kw = TOK_KW_FOR,     .str_kw = "for"    },
        { .tok_kw = TOK_KW_RETURN,  .str_kw = "return" },
};

static TokenKind keyword_or_identifier(const char *start, u32 length)
{
        if (length < 2) return TOK_IDENTIFIER;

        for (u32 i = 0; i < sizeof(kv_array)/sizeof(Keyword_tok); i++) {
                if (strlen(kv_array[i].str_kw) == length) {
                        if (!memcmp(kv_array[i].str_kw, start, length)) {
                                return kv_array[i].tok_kw;
                        }
                }
        }

        return TOK_IDENTIFIER;
}

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

static char skip_white_space(Lexer *lx)
{
        char c = peek(lx);
        while (c == ' '  || c == '\t' ||
               c == '\n' || c == '\r')
        {
                advance(lx);
                c = peek(lx);
        }
        return c;
}

static Token handle_eof_token(Lexer *lx)
{
        Token t = {
                .tok_kind = TOK_EOF,
                .line = lx->line,
                .col = lx->col,
                .start = lx->cur,
                .length = 0
        };
        return t;
}

static Token handle_number_token(Lexer *lx)
{
        Token t = {
                .tok_kind = TOK_NUMBER,
                .start = lx->cur,
                .line = lx->line,
                .col = lx->col,
        };
        char c = peek(lx);
        while (isdigit((u8)c)) {
                advance(lx);
                c = peek(lx);
        }

        t.length = lx->cur - t.start;
        return t;
}

static Token handle_identifier_token(Lexer *lx)
{
        Token t = {
                .start = lx->cur,
                .line = lx->line,
                .col = lx->col
        };

        char c = peek(lx);
        while (isalnum((u8)c)) {
                advance(lx);
                c = peek(lx);
        }

        t.length = lx->cur - t.start;
        t.tok_kind = keyword_or_identifier(t.start, t.length);

        return t;
}

static Token handle_error_token(Lexer *lx)
{
        char c = peek(lx);
        Token t = {
                .tok_kind = TOK_ERROR,
                .start = lx->cur,
                .length = 0,
                .line = lx->line,
                .col = lx->col
        };
        fprintf(stderr, "ERROR: Not supported token: %c\n", c);
        advance(lx);
        return t;
}

Token lexer_next(Lexer *lx) {
        char c = skip_white_space(lx);

        if (c == '\0') return handle_eof_token(lx);
        if (isdigit((u8)c)) return handle_number_token(lx);
        if (isalpha((u8)c)) return handle_identifier_token(lx);

        Token tok = {
                .line = lx->line, 
                .col = lx->col, 
                .start = lx->cur
        };

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
        case ';':
                tok.tok_kind = TOK_SEMICOLON;
                tok.length = 1;
                advance(lx);
                return tok;
        default:
                return handle_error_token(lx);
        }
}
