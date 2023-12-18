#ifndef COURSE_WORK_STRINGS_H
#define COURSE_WORK_STRINGS_H

#include <stddef.h>
#include <malloc.h>

typedef struct {
    char *data;
    size_t length;
    size_t cap;
} string_t;

#define STRING_BASE_CAP 128

string_t *new_string();

void add_substr(string_t *string, char *substr, size_t n);

void expand_string(string_t *string);

void free_string(string_t **string);

#endif //COURSE_WORK_STRINGS_H
