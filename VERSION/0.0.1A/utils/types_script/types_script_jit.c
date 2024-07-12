#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "types_script_jit.h"

Lexer lexer_new(FILE *f) {
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->pos = 0;
    lexer->size = 0; 

    fseek(f, 0, SEEK_END);
    size_t file_len = ftell(f);
    fseek(f, 0, SEEK_SET);

    lexer->red = (char *)malloc(file_len + 1);
    size_t bytes_read = fread(lexer->red, sizeof(char), file_len, f);
    lexer->red[file_len] = '\0'; 
    lexer->tokens = (Token *)malloc((file_len + 1) * sizeof(Token));

    return *lexer;
}

void lexer_free(Lexer lexer) {
    free(lexer.red);
    for (unsigned int i = 0; i < lexer.size; i++) {
        free(lexer.tokens[i].name);
    }
    free(lexer.tokens);
}

Token create_token(TokenType type, char *name) {
    Token token;
    token.type = type;
    token.name = strdup(name);
    return token;
}

Lexer lex(Lexer lexer) {
    size_t token_pos = 0;

    while (lexer.red[lexer.pos] != '\0') {
        if (isspace(lexer.red[lexer.pos])) {
            lexer.pos++;
            continue;
        }

        if (isalpha(lexer.red[lexer.pos])) {
            size_t start = lexer.pos;
            while (isalpha(lexer.red[lexer.pos]) || isdigit(lexer.red[lexer.pos])) {
                lexer.pos++;
            }
            size_t length = lexer.pos - start;
            char *name = strndup(lexer.red + start, length);

            if (strcmp(name, "TYPE") == 0) {
                lexer.tokens[token_pos++] = create_token(TYPE_TOKEN, name);
            } else if (strcmp(name, "SELF") == 0) {
                lexer.tokens[token_pos++] = create_token(TOKEN_SELF, name);
            } else if (strcmp(name, "COLOR") == 0) {
                lexer.tokens[token_pos++] = create_token(COLOR_TOKEN, name);
            } else if (strcmp(name, "POS") == 0) {
                lexer.tokens[token_pos++] = create_token(POS_TOKEN, name);
            } else if (strcmp(name, "ENTITY") == 0) {
                lexer.tokens[token_pos++] = create_token(ENTITY_TOKEN, name);
            } else {
                lexer.tokens[token_pos++] = create_token(NAME_TOKEN, name);
            }

            free(name);
            continue;
        }

        switch (lexer.red[lexer.pos]) {
            case '{':
                lexer.tokens[token_pos++] = create_token(OPEN_CURLY_TOKEN, "{");
                lexer.pos++;
                break;
            case '}':
                lexer.tokens[token_pos++] = create_token(CLOSE_CURLY_TOKEN, "}");
                lexer.pos++;
                break;
            case ',':
                lexer.tokens[token_pos++] = create_token(COMMA_TOKEN, ",");
                lexer.pos++;
                break;
            case ':':
                lexer.tokens[token_pos++] = create_token(COLON_TOKEN, ":");
                lexer.pos++;
                break;
            case '(':
                lexer.tokens[token_pos++] = create_token(OPEN_CURLY_TOKEN, "(");
                lexer.pos++;
                break;
            case ')':
                lexer.tokens[token_pos++] = create_token(CLOSE_CURLY_TOKEN, ")");
                lexer.pos++;
                break;
            case '=':
                lexer.tokens[token_pos++] = create_token(EQUAL_TOKEN, "=");
                lexer.pos++;
                break;
            default:
                if (isdigit(lexer.red[lexer.pos])) {
                    size_t start = lexer.pos;
                    while (isdigit(lexer.red[lexer.pos])) {
                        lexer.pos++;
                    }
                    size_t length = lexer.pos - start;
                    char *number = strndup(lexer.red + start, length);
                    lexer.tokens[token_pos++] = create_token(NUM_TOKEN, number);
                    free(number);
                } else {
                    lexer.tokens[token_pos++] = create_token(INVALID_TOKEN, "INVALID");
                    lexer.pos++;
                }
                break;
        }
    }

    lexer.tokens[token_pos++] = create_token(EOF_TOKEN, "EOF");
    lexer.size = token_pos; 

    for (size_t i = 0; i < lexer.size; i++) {
        printf("Token: %s (%s)\n", lexer.tokens[i].name, token_names[lexer.tokens[i].type]);
    }

    return lexer;
}

