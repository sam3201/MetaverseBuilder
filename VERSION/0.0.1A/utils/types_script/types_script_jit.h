#ifndef TYPES_SCRIPT_JIT_H
#define TYPES_SCRIPT_JIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOREACH_TOKEN_TYPE(_) \
    _(TYPE_TOKEN) \
    _(ENTITY_TOKEN) \
    _(TOKEN_CONSTRUCT) \
    _(TOKEN_SELF) \
    _(NAME_TOKEN) \
    _(COLOR_TOKEN) \
    _(POS_TOKEN) \
    _(TOKEN_PROP) \
    _(TOKEN_ATTR) \
    _(TOKEN_FN) \
    _(COLON_TOKEN) \
    _(OPEN_CURLY_TOKEN) \
    _(CLOSE_CURLY_TOKEN) \
    _(COMMA_TOKEN) \
    _(DOT_TOKEN) \
    _(EQUAL_TOKEN) \
    _(NUM_TOKEN) \
    _(DECIMAL_TOKEN) \
    _(STRING_TOKEN) \
    _(WHITESPACE_TOKEN) \
    _(PLUS_TOKEN) \
    _(MINUS_TOKEN) \
    _(MULT_TOKEN) \
    _(DIV_TOKEN) \
    _(MOD_TOKEN) \
    _(TOKEN_RETURN) \
    _(TOKEN_IF) \
    _(TOKEN_ELSE) \
    _(TOKEN_WHILE) \
    _(TOKEN_FOR) \
    _(TOKEN_BREAK) \
    _(TOKEN_CONTINUE) \
    _(COMMENT_TOKEN) \
    _(NULL_TOKEN) \
    _(INVALID_TOKEN) \
    _(EOF_TOKEN)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum { 
    FOREACH_TOKEN_TYPE(GENERATE_ENUM)
    MAX_TOKEN_TYPE
} TokenType;

static const char *token_names[] = {
    FOREACH_TOKEN_TYPE(GENERATE_STRING)
};

typedef struct {
    char *name;
    TokenType type; 
} Token;

typedef struct {
    struct {
        Token *curr;
    } types;
    struct {
        Token *curr;
    } constructs;
    struct {
        Token *curr;
    } colors;
    struct {
        Token *curr;
    } poses;
    struct {
        Token *curr;
    } props;
    struct {
        Token *curr;
    } attrs;
} Aggregated;

typedef struct {
    unsigned int pos; 
    unsigned int size;
    char *red;
    Token *tokens;
} Lexer;

Lexer lexer_new(FILE *f);
void lexer_free(Lexer lexer);
Lexer lex(Lexer lexer);

Token create_token(TokenType type, char *name);

Aggregated *aggregate(Lexer lexer);

#endif

