#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
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

static char peek (Lexer *lx) { return lx->cur[0]; }
static char peek2(Lexer *lx) { return lx->cur[1]; }

static char advance(Lexer *lx) {
        char cur = *lx->cur;
        if (cur == '\0')
                return cur;

        lx->cur++;
        if (cur == '\n') {
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
        while (isalnum((u8)c) || c == '_') {
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
                .length = 1,
                .line = lx->line,
                .col = lx->col
        };
        fprintf(stderr, "ERROR: Not supported token: %c\n", c);
        advance(lx);
        return t;
}

static Token single_token(Lexer *lx, TokenKind kind)
{
        Token tok = {
                .tok_kind = kind,
                .start = lx->cur,
                .length = 1,
                .line = lx->line,
                .col = lx->col
        };
        advance(lx);
        return tok;
}

static Token multi_token(Lexer *lx, char with, TokenKind wo_eq, TokenKind with_eq)
{
        Token tok = {
                .start = lx->cur,
                .length = 1,
                .line = lx->line,
                .col = lx->col
        };
        if (peek2(lx) == with) {
                tok.tok_kind = with_eq;
                tok.length = 2;
                advance(lx);
        } else {
                tok.tok_kind = wo_eq;
        }
        advance(lx);
        return tok;
}

static Token comment_multi_line(Lexer *lx, Token tok)
{
        advance(lx);advance(lx);
        char c = peek(lx);
        while (c != '*' && peek2(lx) != '/') {
                if (c == '\0') {
                        fprintf(stderr, "ERROR: Multiline comment not closed\n");
                        exit(EXIT_FAILURE);
                }
                advance(lx);
                c = peek(lx);
        }
        /* Consume * / after */
        advance(lx);advance(lx);

        tok.length = lx->cur - tok.start;
        tok.tok_kind = TOK_COMMENT_ML;
        return tok;
}

static Token comment_single_line(Lexer *lx, Token tok)
{
        advance(lx);advance(lx);
        char c = peek(lx);
        while (c != '\n' && c != '\0') {
                advance(lx);
                c = peek(lx);
        }
        tok.length = lx->cur - tok.start;
        tok.tok_kind = TOK_COMMENT_SL;
        return tok;
}

static Token handle_slash_token(Lexer *lx)
{
        Token tok = {
                .tok_kind = TOK_SLASH,
                .start = lx->cur,
                .length = 1,
                .line = lx->line,
                .col = lx->col,
        };
        if (peek2(lx) == '*') return comment_multi_line(lx, tok);
        if (peek2(lx) == '/') return comment_single_line(lx, tok);

        return tok;
}

Token lexer_next(Lexer *lx) {
        char c = skip_white_space(lx);

        if (isdigit((u8)c)) return handle_number_token(lx);
        if (isalpha((u8)c)) return handle_identifier_token(lx);
        if (c == '_')       return handle_identifier_token(lx);
        if (c == '/')       return handle_slash_token(lx);

        switch (c) {
                case '\0': return handle_eof_token(lx);
                case '+': return single_token(lx, TOK_PLUS);
                case '-': return single_token(lx, TOK_MINUS);
                case '*': return single_token(lx, TOK_STAR);
                case '%': return single_token(lx, TOK_MODULO);
                case '(': return single_token(lx, TOK_LPAREN);
                case ')': return single_token(lx, TOK_RPAREN);
                case '[': return single_token(lx, TOK_LSQUARE);
                case ']': return single_token(lx, TOK_RSQUARE);
                case '{': return single_token(lx, TOK_LCURLY);
                case '}': return single_token(lx, TOK_RCURLY);
                case ',': return single_token(lx, TOK_COMMA);
                case ';': return single_token(lx, TOK_SEMICOLON);
                case '<': return multi_token(lx, '=', TOK_LT, TOK_LE);
                case '>': return multi_token(lx, '=', TOK_GT, TOK_GE);
                case '!': return multi_token(lx, '=', TOK_NOT, TOK_NE);
                case '=': return multi_token(lx, '=', TOK_ASSIGN, TOK_EQ);
                case '&': return multi_token(lx, '&', TOK_AMP, TOK_AND);
                case '|': return multi_token(lx, '|', TOK_ERROR, TOK_OR);
                default: return handle_error_token(lx);
        }
}
