#include <stdio.h>
#include <stdlib.h>
#include "types_script_jit.h"

typedef struct {
    Lexer lexer;
    Token currToken;
} Parser;

void nextToken(Parser *parser) {
    parser->currToken = parser->lexer.tokens[parser->lexer.pos++];
}

void expectToken(Parser *parser, TokenType type) {
    if (parser->currToken.type != type) {
        fprintf(stderr, "Unexpected token: expected %s but got %s\n", token_names[type], token_names[parser->currToken.type]);
        exit(1);
    }
}

void generateEntityCode(Parser *parser, FILE *outFile) {
    fprintf(outFile, "Entity *createEntity(");
    
    expectToken(parser, TYPE_TOKEN);
    fprintf(outFile, "TYPE type, ");
    nextToken(parser);
    
    expectToken(parser, NAME_TOKEN);
    fprintf(outFile, "char c, ");
    nextToken(parser);
    
    expectToken(parser, POS_TOKEN);
    fprintf(outFile, "uint8_t x, uint8_t y, ");
    nextToken(parser);
    
    expectToken(parser, NUM_TOKEN);
    fprintf(outFile, "uint8_t health, ");
    nextToken(parser);
    
    expectToken(parser, COLOR_TOKEN);
    fprintf(outFile, "Color color, ");
    nextToken(parser);
    
    expectToken(parser, TOKEN_FN);
    fprintf(outFile, "void (*moveFunc)(Canvas *canvas, Entity *entity)) {\n");
    nextToken(parser);
    
    fprintf(outFile, "    Entity *entity = (Entity *)malloc(sizeof(Entity));\n");
    fprintf(outFile, "    entity->type = type;\n");
    fprintf(outFile, "    entity->cell.c = c;\n");
    fprintf(outFile, "    entity->cell.pos.x = x;\n");
    fprintf(outFile, "    entity->cell.pos.y = y;\n");
    fprintf(outFile, "    entity->health = health;\n");
    fprintf(outFile, "    entity->isAlive = 1;\n");
    fprintf(outFile, "    entity->color = color;\n");
    fprintf(outFile, "    entity->moveFunc = moveFunc;\n");
    fprintf(outFile, "    return entity;\n");
    fprintf(outFile, "}\n");
}

void parseEntities(Parser *parser, FILE *outFile) {
    while (parser->lexer.pos < parser->lexer.pos) {
        nextToken(parser);
        if (parser->currToken.type == ENTITY_TOKEN) {
            generateEntityCode(parser, outFile);
        }
    }
}

void parseScript(Lexer lexer, const char *outputFileName) {
    Parser parser = { lexer };
    FILE *outFile = fopen(outputFileName, "w");
    if (!outFile) {
        perror("Failed to open output file");
        exit(1);
    }

    parseEntities(&parser, outFile);
    fclose(outFile);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_script> <output_file>\n", argv[0]);
        return 1;
    }

    FILE *inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        perror("Failed to open input file");
        return 1;
    }

    Lexer lexer = lexer_new(inputFile);
    lexer = lex(lexer);
    fclose(inputFile);

    parseScript(lexer, argv[2]);
    lexer_free(lexer);

    return 0;
}

