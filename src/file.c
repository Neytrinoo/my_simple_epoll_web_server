#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "file.h"
#include "logger.h"

#define BASE_PAGE "/index.html"

file_type_t *new_file_type() {
    file_type_t *new = malloc(sizeof(file_type_t));
    new->file_path = new_string();
    new->raw_file_path = new_string();

    return new;
}

void free_file_type(file_type_t **file) {
    free_string(&(*file)->file_path);
    free_string(&(*file)->raw_file_path);
    free(*file);
    *file = NULL;
}

// Поиск следующего подпути. Возвращает индекс следующего слеша
int get_next_subpath(string_t *file_path, int start) {
    int begin = start;
    while (begin < file_path->length && file_path->data[begin] != '/') {
        ++begin;
    }

    return begin;
}

file_extension_t get_file_extension(char *filename, int n) {
    char *dot = strrchr(filename, '.');
    if (!dot) {
        return UNDEFINED_FE;
    }
    ++dot;

    if (strcmp(dot, "html") == 0) {
        return HTML_FE;
    } else if (strcmp(dot, "css") == 0) {
        return CSS_FE;
    } else if (strcmp(dot, "js") == 0) {
        return JS_FE;
    } else if (strcmp(dot, "png") == 0) {
        return PNG_FE;
    } else if (strcmp(dot, "jpg") == 0) {
        return JPG_FE;
    } else if (strcmp(dot, "jpeg") == 0) {
        return JPEG_FE;
    } else if (strcmp(dot, "swf") == 0) {
        return SWF_FE;
    } else if (strcmp(dot, "gif") == 0) {
        return GIF_FE;
    } else if (strcmp(dot, "pdf") == 0) {
        return PDF_FE;
    }

    return UNDEFINED_FE;
}

char *get_file_extension_string(file_extension_t fe) {
    switch (fe) {
        case HTML_FE:
            return "text/html; charset=utf-8";
        case CSS_FE:
            return "text/css; charset=utf-8";
        case JS_FE:
            return "text/javascript; charset=utf-8";
        case PNG_FE:
            return "image/png";
        case JPG_FE:
        case JPEG_FE:
            return "image/jpeg";
        case SWF_FE:
            return "application/x-shockwave-flash";
        case GIF_FE:
            return "image/gif";
        case PDF_FE:
            return "application/pdf";
    }

    return "undefined";
}



int set_file_size(file_type_t *file_type) {
    string_t *full_path_to_file = new_string();
    add_substr(full_path_to_file, ROOT, strlen(ROOT));
    add_substr(full_path_to_file, file_type->file_path->data, file_type->file_path->length);

    struct stat st;
    if (stat(full_path_to_file->data, &st) == 0) {
        file_type->file_length = st.st_size;
    } else {
        return -1;
    }

    // заменяем относительный путь на абсолютный, включающий ROOT
    free_string(&file_type->file_path);
    file_type->file_path = full_path_to_file;

    return 0;
}

file_parse_result_t parse_file_path(file_type_t *file_type) {
    int depth = 0; // глубина вложенности (если будет < 0 - выдаем ошибку)
    string_t *file_path = file_type->file_path;

    if (file_path->data[0] != '/') {
        return INVALID_F;
    }

    if (file_path->data[0] == '/' && file_path->length == 2) {
        memcpy(file_path->data, BASE_PAGE, strlen(BASE_PAGE));
        file_path->length = strlen(BASE_PAGE);
        add_substr(file_path, "\0", 1);
    }

    int now_position = 1;
    int next_position = 1;
    while ((next_position = get_next_subpath(file_path, now_position)) != file_path->length) {
        if (strncmp(file_path->data + now_position, "..", 2) == 0) {
            --depth;
        } else {
            ++depth;
        }

        if (depth < 0) {
            return INVALID_F;
        }

        now_position = ++next_position;
    }

    file_type->file_extension = get_file_extension(file_path->data + now_position, file_path->length - now_position - 1);
    if (file_type->file_extension == UNDEFINED_FE) {
        return INVALID_F;
    }

    if (set_file_size(file_type)) {
        return FILE_NOT_FOUND_F;
    }

    return OK_F;
}
