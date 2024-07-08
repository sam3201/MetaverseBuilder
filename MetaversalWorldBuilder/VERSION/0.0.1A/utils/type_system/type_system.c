#include "type_system.h"
#include "type_definitions.h"

const char *TYPE_STRING = {
  FOREACH_TYPE(GENERATE_STRING)

}

struct TypeInfo types[TYPE_COUNT];

void add_type(const char *name) {
    if (dynamic_types == NULL) {
        dynamic_types = (struct TypeInfo *)malloc(sizeof(struct TypeInfo));
    } else {
        dynamic_types = (struct TypeInfo *)realloc(dynamic_types, (dynamic_type_count + 1) * sizeof(struct TypeInfo));
    }

    if (dynamic_types == NULL) {
        fprintf(stderr, "Failed to allocate memory for dynamic types\n");
        return;
    }

    dynamic_types[dynamic_type_count].name = strdup(name);
    dynamic_types[dynamic_type_count].id = TYPE_COUNT + dynamic_type_count;
    dynamic_type_count++;

    FILE *file = fopen("type_definitions.h", "a");
    if (file == NULL) {
        fprintf(stderr, "Failed to open type_definitions.h for writing\n");
        return;
    }

    fprintf(file, "#define TYPE_CREATE(nm) nm##_TYPE,\n");
    fprintf(file, "TYPE_COUNT\n");
    for (size_t i = 0; i < dynamic_type_count; i++) {
        fprintf(file, "TYPE_CREATE(%s)\n", dynamic_types[i].name);
    }

    fclose(file);
}

extern struct TypeInfo *initialize_all_entity_types() {
 for (size_t i = 0; i < TYPE_COUNT; i++) {
    addType(types[i].name); 
  }

  return types; 
}

TYPE initialize_entity_type(const char *name) {
     #define TYPE_CREATE(nm) \
        if (strcmp(name, #nm) == 0) \
            return nm##_TYPE;

  perror("Type not found");
  exit(1);
}

TYPE addType(const char *name) {
    #define TYPE_CREATE(nm) \
        if (strcmp(name, #nm) == 0) { \
            add_type(name); \
            return nm##_TYPE; \
        }


  printf("Type '%s' already exists\n", name);
  return -1;
}

void remove_type(const char *name) {
    for (size_t i = 0; i < dynamic_type_count; i++) {
        if (strcmp(dynamic_types[i].name, name) == 0) {
            free((void *)dynamic_types[i].name);
            for (size_t j = i; j < dynamic_type_count - 1; j++) {
                dynamic_types[j] = dynamic_types[j + 1];
            }
            dynamic_type_count--;

            FILE *file = fopen("type_definitions.h", "w");
            if (file == NULL) {
                fprintf(stderr, "Failed to open type_definitions.h for writing\n");
                return;
            }

            fprintf(file, "#define TYPE_CREATE(nm) nm##_TYPE,\n");
            fprintf(file, "TYPE_COUNT\n");
            for (size_t i = 0; i < dynamic_type_count; i++) {
                fprintf(file, "TYPE_CREATE(%s)\n", dynamic_types[i].name);
            }

            fclose(file);

            return;
        }
    }
    fprintf(stderr, "Type '%s' not found for removal\n", name);
}

void print_types() {
    printf("Static Types:\n");
    for (size_t i = 0; i < TYPE_COUNT; i++) {
        printf("%s (%d)\n", types[i].name, types[i].id);
    }

    printf("Dynamic Types:\n");
    for (size_t i = 0; i < dynamic_type_count; i++) {
        printf("%s (%d)\n", dynamic_types[i].name, dynamic_types[i].id);
    }
}



