#include "lexer.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char* start;
    const char* current;
} Lexer;

Lexer lexer = {0};

void init_lexer(char* source)
{
    lexer.start = source;
    lexer.current = source;
}

static char advance(void)
{
    lexer.current++;
    return lexer.current[-1];
}

static char peek(void)
{
    return *lexer.current;
}

static void skip_whitespace(void)
{
    for(;;)
    {
        char c = peek();
        switch(c)
        {
            case ' ':
            case '\r':
            case '\t':
            case '\n':
                advance();
                break;
            default: return;
        }
    }
}

static Token make_token(TokenKind kind)
{
    return (Token) {
        .kind = kind,
        .len = (int)(lexer.current - lexer.start),
        .start = lexer.start,
    };
}

static bool is_alpha(char c)
{
    return ((c >= 'a' && c <= 'z') || (c == '_'));
}

static bool is_num(char c)
{
    return (c >= '0' && c <= '9');
}

static bool is_caps(char c)
{
    return (c >= 'A' && c <= 'Z');
}

static Token read_number(void)
{
    while(is_num(peek())) advance();
    if(peek() == '.' && is_num(lexer.current[1])) 
    {
        advance();
        while(is_num(peek())) advance();
    }
    return make_token(NUM);
}

static TokenKind match_ident(int start, int len, char* rest, TokenKind expected)
{
    if(start+len != (int)(lexer.current - lexer.start))
    {
        return ILLEGAL;
    }
    if(memcmp(lexer.start+start, rest, len) == 0) return expected;
    return ILLEGAL;
}

static TokenKind identifier(void)
{
    switch(lexer.start[0])
    {
        case 'd': match_ident(1,2,"up", DUP);
        default: return ILLEGAL;
    }
}

static Token read_ident(void)
{
    while(is_alpha(peek())) advance();
    return make_token(identifier());
}

Token next_token(void)
{
    skip_whitespace();
    lexer.start = lexer.current;
    char c = advance();
    if(is_alpha(c)) return read_ident();
    if(is_num(c)) return read_number();
    if(is_caps(c)) fprintf(stderr, "Error: Please use snakecase %c\n", c);
    switch(c)
    {
        case '+': return make_token(PLUS);
        case '-': return make_token(MINUS);
        case '*': return make_token(MULT);
        case '/': return make_token(DIV);
        case '\0': return make_token(EOFF);
        default: return make_token(ILLEGAL);
    }
}
