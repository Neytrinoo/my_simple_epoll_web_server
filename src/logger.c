#include <stdarg.h>
#include <time.h>

#include "logger.h"

FILE *log_file = NULL;
log_level_t log_level = DEBUG;

void set_log_file(FILE *f) {
    log_file = f;
}

void set_log_level(log_level_t l) {
    log_level = l;
}

void log_print_data() {
    time_t mytime = time(NULL);
    struct tm *now = localtime(&mytime);
    fprintf(log_file,"%04d.%02d.%02d %02d:%02d:%04d: ", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
}

void LOG(log_level_t level, char *format_string, ...) {
    if (level < log_level) {
        return;
    }
    va_list factor;
    va_start(factor, format_string);
    log_print_data();
    vfprintf(log_file, format_string, factor);
    fprintf(log_file, "\n");
    va_end(factor);
}