#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "lexer.h"
#include "nob.h"

typedef struct {
    const char* start;
    const char* current;
} Lexer;

Lexer lexer = {0};

void init_lexer(char* source)
{
    assert(source != NULL);
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
        case 'd': return match_ident(1,2,"up", DUP);
        case 'p': return match_ident(1,4, "rint", PRINT);
        case 'i': return match_ident(1,1,"f", IF);
        case 'e': return match_ident(1,2,"nd", END);
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
    switch(c)
    {
        case '+': return make_token(PLUS);
        case '-': return make_token(MINUS);
        case '*': return make_token(MULT);
        case '/': return make_token(DIV);
        case '.': return make_token(DOT);
        case '=': return make_token(EQUAL);
        case '\0': return make_token(EOFF);
        default: return make_token(ILLEGAL);
    }
}

#define STACK_SIZE 1024
typedef struct {
    int items[STACK_SIZE];
    int* sp;
} Stack;

void init_stack(Stack *stack)
{
    stack->sp = stack->items;
}

void stack_push(Stack *stack, int value)
{
    *stack->sp++ = value;
}

int stack_pop(Stack *stack)
{
    stack->sp--;
    return *stack->sp;
}

TokenList generate_tokens(void)
{
    TokenList token_list = {0};
    Stack addr_stack = {0};
    init_stack(&addr_stack);
    bool loop = true;
    while(loop)
    {
        Token tok = next_token();
        switch(tok.kind)
        {
            case EQUAL:
            case PRINT:
            case DOT:
            case ILLEGAL:
            case PLUS:
            case DUP:
            case MINUS:
            case MULT:
            case DIV:
            case NUM:
                nob_da_append(&token_list, tok);
                break;
            case IF:
                stack_push(&addr_stack, token_list.count);
                nob_da_append(&token_list, tok);
                break;
            case END:
            {
                int back_addr = stack_pop(&addr_stack);
                Token *if_tok = &token_list.items[back_addr];
                assert(if_tok->kind == IF);
                if_tok->addr = token_list.count;
                tok.addr = token_list.count;
                nob_da_append(&token_list, tok);
                break;
            }
            case EOFF:
                loop = false;
                break;
        }
    }
    return token_list;
}
