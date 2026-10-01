#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

/* Unsigned ints */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* Singed ints */
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef enum {
        TOK_EOF,
        TOK_ERROR,

        TOK_NUMBER,
        TOK_IDENTIFIER,

        TOK_CHAR_LITERAL,
        TOK_STRING_LITERAL,

        /* Keywords */
        TOK_KW_INT,
        TOK_KW_CHAR,
        TOK_KW_LONG,
        TOK_KW_VOID,
        TOK_KW_IF,
        TOK_KW_ELSE,
        TOK_KW_WHILE,
        TOK_KW_FOR,
        TOK_KW_RETURN,

        TOK_AND,
        TOK_OR,
        TOK_AMP, /* And bitwise op */

        /* Signs */
        TOK_ASSIGN,
        TOK_EQ,
        TOK_LT,
        TOK_LE,
        TOK_GT,
        TOK_GE,
        TOK_NOT,
        TOK_NE,
        TOK_PLUS,
        TOK_INCREMENT,
        TOK_MINUS,
        TOK_DECREMENT,
        TOK_STAR,
        TOK_SLASH,
        TOK_MODULO,
        TOK_LPAREN,
        TOK_RPAREN,
        TOK_LSQUARE,
        TOK_RSQUARE,
        TOK_LCURLY,
        TOK_RCURLY,
        TOK_COMMA,
        TOK_SEMICOLON,

        TOK_COMMENT_SL,
        TOK_COMMENT_ML,
} TokenKind;

typedef struct {
        TokenKind tok_kw;
        const char *str_kw;
} Keyword_tok;

typedef struct {
        TokenKind tok_kind;
        const char *start;
        u32 length;
        u32 line, col;
} Token;

typedef struct {
        const char *cur;
        u32 line, col;
} Lexer;

void lexer_init(Lexer *lx, const char *source);
Token lexer_next(Lexer *lx);
char *tok_kind_name(TokenKind tk);

#endif
