#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "lexer.h"
#include "nob.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Lexer;

Lexer lexer = {0};

void init_lexer(char* source)
{
    assert(source != NULL);
    lexer.start = source;
    lexer.current = source;
    lexer.line = 1;
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
                advance();
                break;
            case '\n':
                lexer.line++;
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
        .line = lexer.line,
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
        case 'e': 
            if(lexer.current - lexer.start > 0) 
            {
                switch (lexer.start[1])
                {
                    case 'n': return match_ident(2,1,"d", END);
                    case 'l': return match_ident(2,2, "se", ELSE);
                }
            }
            return ILLEGAL; 
        default: return ILLEGAL;
    }
}

static Token read_ident(void)
{
    while(is_alpha(peek())) advance();
    return make_token(identifier());
}

static bool match(char c)
{
    if(peek() == c) 
    {
        advance();
        return true;
    }
    return false;
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
        case '>': return make_token(match('=') ? GREATER_EQUAL : GREATER);
        case '<': return make_token(match('=') ? LESS_EQUAL : LESS);
        case '\0': return make_token(EOFF);
        default: return make_token(ILLEGAL);
    }
}

#define STACK_SIZE 1024
typedef struct {
    int items[STACK_SIZE];
    int len;
} Stack;

void init_stack(Stack *stack)
{
    stack->len = 0;
}

void stack_push(Stack *stack, int value)
{
    stack->items[stack->len++] = value;
}

int stack_pop(Stack *stack)
{
    if(stack->len == 0) return 0;
    return stack->items[--stack->len];
}

//TODO: FIX ERROR HANDLING
 bool generate_tokens(TokenList *tokenlist)
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
            case LESS:
            case LESS_EQUAL:
            case GREATER:
            case GREATER_EQUAL:
            case DIV:
            case NUM:
                nob_da_append(&token_list, tok);
                break;
            case IF:
                stack_push(&addr_stack, token_list.count);
                nob_da_append(&token_list, tok);
                break;
            case ELSE:
            {

                int back_addr = stack_pop(&addr_stack);
                Token *back_tok = &token_list.items[back_addr];
                if(back_tok->kind != IF) 
                {
                    nob_log(NOB_ERROR, "line:%d `else` can only be used in `if` blocks",
                            tok.line);
                    return false;
                }
                back_tok->addr_to = token_list.count;
                tok.addr_fro = token_list.count;
                stack_push(&addr_stack, token_list.count);
                nob_da_append(&token_list, tok);
                break;
            }
            case END:
            {
                int back_addr = stack_pop(&addr_stack);
                Token *back_tok = &token_list.items[back_addr];
                if(!(back_tok->kind == IF || back_tok->kind == ELSE)) 
                {
                    nob_log(NOB_ERROR, "line:%d `end` can only be used in `if` `else` blocks for now",
                            tok.line);
                    return false;
                }
                tok.addr_fro = token_list.count;
                back_tok->addr_to = token_list.count;
                nob_da_append(&token_list, tok);
                break;
            }
            case EOFF:
                loop = false;
                break;
        }
    }
    *tokenlist = token_list;
    return true;
}
