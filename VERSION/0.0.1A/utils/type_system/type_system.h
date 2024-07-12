#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    unsigned int id;
} TYPE;

#define TYPE(nm) \
    __extension__({ \
        if (!type_exists(#nm)) { \
            add_type_to_file(#nm); \
            printf("Type '%s' created.\n", #nm); \
        } else { \
            printf("Type '%s' already exists.\n", #nm); \
        } \
    })

TYPE *initialize_all_entity_types();
void add_type_to_file(char *name);
int type_exists(char *name);
void add_type(char *name);
void remove_type(char *name);
void print_types();

#endif 

