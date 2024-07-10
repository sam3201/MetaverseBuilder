#include "type_system.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void add_type_to_file(const char *name) {
    FILE *file = fopen("types.def", "a");
    if (file != NULL) {
        fprintf(file, "%s\n", name);
        fclose(file);
    }
}

int type_exists(const char *name) {
    FILE *file = fopen("types.def", "r");
    if (file != NULL) {
        char *buffer = NULL;
        size_t len = 0;
        ssize_t read;
        while ((read = getline(&buffer, &len, file)) != -1) {
            buffer[strcspn(buffer, "\n")] = '\0'; 
            if (strcmp(buffer, name) == 0) {
                free(buffer);
                fclose(file);
                return 1;
            }
        }
        free(buffer);
        fclose(file);
    }
    return 0;
}

void initialize_all_entity_types() {
    FILE *file = fopen("types.def", "r");
    if (file != NULL) {
        char *buffer = NULL;
        size_t len = 0;
        ssize_t read;
        while ((read = getline(&buffer, &len, file)) != -1) {
            buffer[strcspn(buffer, "\n")] = '\0'; 
            printf("Loaded type: %s\n", buffer);
        }
        free(buffer);
        fclose(file);
    }
}

void add_type(const char *name) {
    if (!type_exists(name)) {
        add_type_to_file(name);
        printf("Added new type: %s\n", name);
    } else {
        printf("Type '%s' already exists.\n", name);
    }
}

void remove_type(const char *name) {
    FILE *file = fopen("types.def", "r");
    FILE *temp = fopen("temp.def", "w");
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
        remove("types.def");
        rename("temp.def", "types.def");
        printf("Removed type: %s\n", name);
    }
}

void print_types() {
    FILE *file = fopen("types.def", "r");
    if (file != NULL) {
        char *buffer = NULL;
        size_t len = 0;
        ssize_t read;
        while ((read = getline(&buffer, &len, file)) != -1) {
            buffer[strcspn(buffer, "\n")] = '\0'; 
            printf("Type: %s\n", buffer);
        }
        free(buffer);
        fclose(file);
    }
}


