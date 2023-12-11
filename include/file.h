#ifndef COURSE_WORK_FILE_H
#define COURSE_WORK_FILE_H

#include "strings.h"

extern char *ROOT;

typedef enum {
    HTML_FE,
    CSS_FE,
    JS_FE,
    PNG_FE,
    JPG_FE,
    JPEG_FE,
    SWF_FE,
    GIF_FE,
    UNDEFINED_FE
} file_extension_t;

typedef enum {
    OK_F,
    FILE_NOT_FOUND_F,
    INVALID_F
} file_parse_result_t;

typedef struct {
    string_t *file_path;
    string_t *raw_file_path;
    file_extension_t file_extension;
    size_t file_length;
} file_type_t;

file_type_t *new_file_type();

void free_file_type(file_type_t **file);

file_extension_t get_file_extension(char *filename, int n);

char *get_file_extension_string(file_extension_t fe);

// Поиск следующего подпути. Возвращает индекс следующего слеша
int get_next_subpath(string_t *file_path, int start);

file_parse_result_t parse_file_path(file_type_t *file_type);


#endif //COURSE_WORK_FILE_H
