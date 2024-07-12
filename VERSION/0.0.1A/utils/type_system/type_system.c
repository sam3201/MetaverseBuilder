#include "type_system.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TYPE *types;
unsigned int type_count = 0;

void add_type_to_file(char *name) {
    FILE *file = fopen("types.deff", "a");
    if (file != NULL) {
        fprintf(file, "%s\n", name);
        fclose(file);
    }
}

int type_exists(char *name) {
    for (unsigned int i = 0; i < type_count; i++) {
        if (strcmp(types[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

TYPE *initialize_all_entity_types() {
    FILE *file = fopen("types.deff", "r");
    if (file != NULL) {
        char *buffer = NULL;
        size_t len = 0;
        ssize_t read;
        type_count = 0;
        while ((read = getline(&buffer, &len, file)) != -1) {
            buffer[strcspn(buffer, "\n")] = '\0';
            if (!type_exists(buffer)) {
                types[type_count].name = strdup(buffer);
                types[type_count].id = type_count;
                printf("Loaded type: %s\n", buffer);
                type_count++;
            }
        }
        free(buffer);
        fclose(file);
    }
    return types;
}

void add_type(char *name) {
  if (type_exists(name)) {
    printf("Type already exists: %s\n", name);
    return;
  }
  add_type_to_file(name);
  strcpy(types[type_count].name, name);
  types[type_count].id = type_count;
  type_count++;
  printf("Added new type: %s\n", name);
}

void remove_type(char *name) {
    FILE *file = fopen("types.deff", "r");
    FILE *temp = fopen("temp.deff", "w");
    if (file != NULL && temp != NULL) {
        char *buffer = NULL;
        size_t len = 0;
        ssize_t read;
        while ((read = getline(&buffer, &len, file)) != -1) {
            buffer[strcspn(buffer, "\n")] = '\0';
            if (strcmp(buffer, name) != 0) {
                fprintf(temp, "%s\n", buffer);
            }
        }
        free(buffer);
        fclose(file);
        fclose(temp);
        remove("types.deff");
        rename("temp.deff", "types.deff");
        printf("Removed type: %s\n", name);

        for (int i = 0; i < type_count; i++) {
            if (strcmp(types[i].name, name) == 0) {
                free(types[i].name);
                for (int j = i; j < type_count - 1; j++) {
                    types[j] = types[j + 1];
                }
                type_count--;
                break;
            }
        }
    }
}

void print_types() {
    for (int i = 0; i < type_count; i++) {
        printf("Type: %s\n", types[i].name);
    }
}

