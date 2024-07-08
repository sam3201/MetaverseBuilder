#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TYPE_CREATE
#undef TYPE_CREATE
#endif

#define TYPE_CREATE(nm) nm##_TYPE,

typedef enum {
    #include "type_definitions.h"
    TYPE_COUNT
} TYPE;

struct TypeInfo {
    const char *name;
    TYPE id;
} ;

extern struct TypeInfo types[TYPE_COUNT];
extern struct TypeInfo *dynamic_types;
extern size_t dynamic_type_count;

extern struct TypeInfo *initialize_all_entity_types(); 
TYPE initalize_entity_type(TYPE type);
void add_type(const char *name);
TYPE addType(const char *name);
void remove_type(const char *name);
void print_types();

#endif

