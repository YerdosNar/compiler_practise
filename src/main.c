#include "lexer.h"

#include <stdio.h>

char *tok_kind_name(TokenKind tk)
{
        switch (tk) {
                case TOK_EOF:           return "TOK_EOF";
                case TOK_ERROR:         return "TOK_ERROR";
                case TOK_NUMBER:        return "TOK_NUMBER";
                case TOK_IDENTIFIER:    return "TOK_IDENTIFIER";
                case TOK_CHAR_LITERAL:  return "TOK_CHAR_LITERAL";
                case TOK_STRING_LITERAL:return "TOK_STRING_LITERAL";
                case TOK_KW_INT:        return "TOK_KW_INT";
                case TOK_KW_CHAR:       return "TOK_KW_CHAR";
                case TOK_KW_LONG:       return "TOK_KW_LONG";
                case TOK_KW_VOID:       return "TOK_KW_VOID";
                case TOK_KW_IF:         return "TOK_KW_IF";
                case TOK_KW_ELSE:       return "TOK_KW_ELSE";
                case TOK_KW_WHILE:      return "TOK_KW_WHILE";
                case TOK_KW_FOR:        return "TOK_KW_FOR";
                case TOK_KW_RETURN:     return "TOK_KW_RETURN";
                case TOK_ASSIGN:        return "TOK_ASSIGN";
                case TOK_AND:           return "TOK_AND";
                case TOK_OR:            return "TOK_OR";
                case TOK_AMP:           return "TOK_AMP";
                case TOK_EQ:            return "TOK_EQ";
                case TOK_LT:            return "TOK_LT";
                case TOK_LE:            return "TOK_LE";
                case TOK_GT:            return "TOK_GT";
                case TOK_GE:            return "TOK_GE";
                case TOK_NOT:           return "TOK_NOT";
                case TOK_NE:            return "TOK_NE";
                case TOK_PLUS:          return "TOK_PLUS";
                case TOK_MINUS:         return "TOK_MINUS";
                case TOK_STAR:          return "TOK_STAR";
                case TOK_SLASH:         return "TOK_SLASH";
                case TOK_COMMENT_ML:    return "TOK_COMMENT_ML";
                case TOK_COMMENT_SL:    return "TOK_COMMENT_SL";
                case TOK_LPAREN:        return "TOK_LPAREN";
                case TOK_RPAREN:        return "TOK_RPAREN";
                case TOK_LSQUARE:       return "TOK_LSQUARE";
                case TOK_RSQUARE:       return "TOK_RSQUARE";
                case TOK_LCURLY:        return "TOK_LCURLY";
                case TOK_RCURLY:        return "TOK_RCURLY";
                case TOK_COMMA:         return "TOK_COMMA";
                case TOK_SEMICOLON:     return "TOK_SEMICOLON";
                default: return "Unknown";
        }
}

u32 print_token(Token t) {
        if (t.tok_kind == TOK_ERROR) {
                fprintf(stderr, "ERROR: Unknown token: %.*s\n", (int)t.length, t.start);
                fprintf(stderr, "LINE: %u, COL: %u\n\n", t.line, t.col);
                return 1;
        }
        printf("%s: %.*s\n", tok_kind_name(t.tok_kind), (int)t.length, t.start);
        printf("LINE: %u, COL: %u\n\n", t.line, t.col);
        return 0;
}

int main(int argc, char **argv) {
        if (argc < 2) {
                fprintf(stderr, "ERROR: Give a string \"+ - * / ( ) \"\n");
                return 1;
        }

        u32 error_count = 0;
        printf("TinyC start\n\n");
        const char *src = argv[1];
        Lexer lx;
        lexer_init(&lx, src);
        while (1) {
                Token t = lexer_next(&lx);
                if (t.tok_kind == TOK_EOF) break;
                error_count += print_token(t);
        }

        printf("ERRORS: %u\n", error_count);
        if (error_count) return 1;

        return 0;
}
