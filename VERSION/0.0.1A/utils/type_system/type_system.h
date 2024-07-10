#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct TYPE TYPE;
void add_type_to_file(const char *name);
int type_exists(const char *name);
void initialize_all_entity_types(); 
void add_type(const char *name);
void remove_type(const char *name);
void print_types();

#define TYPE(nm) \
    __extension__({ \
        if (!type_exists(#nm)) { \
            add_type_to_file(#nm); \
            printf("Type '%s' created.\n", #nm); \
        } else { \
            printf("Type '%s' already exists.\n", #nm); \
        } \
    })


#endif 

