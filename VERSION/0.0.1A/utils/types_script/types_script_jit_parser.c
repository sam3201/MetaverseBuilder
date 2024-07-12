#include <stdio.h>
#include <stdlib.h>
#include "types_script_jit.h"

typedef struct {
    Lexer lexer;
    Token currToken;
} Parser;

void nextToken(Parser *parser) {
    if (parser->lexer.pos < parser->lexer.size) {
        parser->currToken = parser->lexer.tokens[parser->lexer.pos++];
        printf("Next token: %s (%s)\n", parser->currToken.name, token_names[parser->currToken.type]);
    } else {
        parser->currToken = create_token(EOF_TOKEN, "EOF");
    }
}

void expectToken(Parser *parser, TokenType type) {
    if (parser->currToken.type != type) {
        fprintf(stderr, "Unexpected token: expected %s but got %s\n", token_names[type], token_names[parser->currToken.type]);
        exit(1);
    }
}

void generateEntityCode(Parser *parser, FILE *outFile) {
    printf("Generating entity code...\n");

    fputs("Entity *createEntity(TYPE type, char c, uint8_t x, uint8_t y, uint8_t health, Color color, void (*moveFunc)(Canvas *canvas, Entity *entity)) {\n", outFile);
    fputs("    Entity *entity = (Entity *)malloc(sizeof(Entity));\n", outFile);
    fputs("    entity->type = type;\n", outFile);
    fputs("    entity->cell.c = c;\n", outFile);
    fputs("    entity->cell.pos.x = x;\n", outFile);
    fputs("    entity->cell.pos.y = y;\n", outFile);
    fputs("    entity->health = health;\n", outFile);
    fputs("    entity->isAlive = 1;\n", outFile);
    fputs("    entity->color = color;\n", outFile);
    fputs("    entity->moveFunc = moveFunc;\n", outFile);
    fputs("    return entity;\n", outFile);
    fputs("}\n", outFile);

    if (fflush(outFile) != 0) {
        perror("Error flushing file buffer");
        return;
    }

    printf("Entity code generated successfully.\n");
}

void parseScript(Lexer lexer, const char *outputFileName) {
    Parser parser = { lexer, lexer.tokens[0] };
    FILE *outFile = fopen(outputFileName, "a");
    if (!outFile) {
        perror("Failed to open output file");
        exit(1);
    }

    printf("Starting to parse entities...\n");
    while (parser.currToken.type != EOF_TOKEN) {
        printf("Processing token: %s (%s)\n", parser.currToken.name, token_names[parser.currToken.type]);

        switch(parser.currToken.type) {
            case ENTITY_TOKEN:
                printf("Entity token found. Generating entity code.\n");
                generateEntityCode(&parser, outFile);
                break;
            case TYPE_TOKEN:
                printf("Type token found. Expecting entity definition.\n");
                nextToken(&parser);
                if (parser.currToken.type == OPEN_CURLY_TOKEN) {
                    printf("Found open curly brace. Parsing entity definition.\n");
                    while (parser.currToken.type != CLOSE_CURLY_TOKEN && parser.currToken.type != EOF_TOKEN) {
                        nextToken(&parser);
                        printf("Entity definition token: %s (%s)\n", parser.currToken.name, token_names[parser.currToken.type]);
                    }
                    if (parser.currToken.type == CLOSE_CURLY_TOKEN) {
                        printf("Found closing curly brace. Entity definition complete.\n");
                    } else {
                        printf("Warning: Reached EOF before finding closing curly brace.\n");
                    }
                } else {
                    printf("Warning: Expected open curly brace after TYPE, found %s instead.\n", token_names[parser.currToken.type]);
                }
                break;
            default:
                printf("Unhandled token type: %s\n", token_names[parser.currToken.type]);
                break;
        }

        nextToken(&parser);
    }
    printf("Finished parsing entities.\n");

    if (fclose(outFile) != 0) {
        perror("Error closing output file");
    }

    printf("Finished writing to output file.\n");
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_script.deff> <output_file>\n", argv[0]);
        return 1;
    }

    FILE *inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        perror("Failed to open input file");
        return 1;
    }

    printf("Input file opened successfully.\n");

    Lexer lexer = lexer_new(inputFile);
    printf("Lexer created.\n");

    lexer = lex(lexer);
    printf("Lexing completed.\n");

    fclose(inputFile);

    printf("Tokens after lexing:\n");
    for (unsigned int i = 0; i < lexer.size; i++) {
        printf("Token %d: %s (%s)\n", i, lexer.tokens[i].name, token_names[lexer.tokens[i].type]);
    }

    printf("Starting parsing...\n");
    parseScript(lexer, argv[2]);
    printf("Parsing completed.\n");

    lexer_free(lexer);
    printf("Lexer freed.\n");

    return 0;
}

