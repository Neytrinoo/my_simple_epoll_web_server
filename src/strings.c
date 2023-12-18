#include <string.h>

#include "strings.h"

string_t *new_string() {
    string_t *string = malloc(sizeof(string_t));
    if (!string) {
        return NULL;
    }

    string->cap = STRING_BASE_CAP;
    string->length = 0;
    string->data = malloc(STRING_BASE_CAP);
    if (!string->data) {
        free(string);
        return NULL;
    }

    return string;
}

void expand_string(string_t *string) {
    char *new_data = malloc(string->cap * 2);

    strncpy(new_data, string->data, string->cap);

    free(string->data);
    string->data = new_data;
    string->cap *= 2;
}

void add_substr(string_t *string, char *substr, size_t n) {
    while (string->length + n >= string->cap) {
        expand_string(string);
    }

    memcpy(string->data+string->length, substr, n);
    string->length += n;
}

void free_string(string_t **string) {
    free((*string)->data);
    free(*string);
    *string = NULL;
}