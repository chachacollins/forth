#ifndef LEXER_H
#define LEXER_H

typedef enum {
    DUP,
    PLUS,
    MINUS,
    DIV,
    MULT,
    NUM,
    ILLEGAL,
    EOFF
} TokenKind;

typedef struct {
    TokenKind kind;
    const char* start;
    int len;
} Token;

void init_lexer(char* source);
Token next_token(void);
#endif
