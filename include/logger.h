#ifndef COURSE_WORK_LOGGER_H
#define COURSE_WORK_LOGGER_H

#include <stdio.h>

extern FILE *log_file;

void set_log_file(FILE *f);

typedef enum {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
} log_level_t;

extern log_level_t log_level;

void set_log_level(log_level_t l);

void LOG(log_level_t log_level, char *format_string, ...);

#endif //COURSE_WORK_LOGGER_H
