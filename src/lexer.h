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
        TOK_PLUS,
        TOK_MINUS,
        TOK_STAR,
        TOK_SLASH,
        TOK_LPAREN,
        TOK_RPAREN
} TokenKind;

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

#endif
