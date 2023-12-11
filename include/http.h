#ifndef COURSE_WORK_HTTP_H
#define COURSE_WORK_HTTP_H

#include <stdbool.h>

#include "strings.h"
#include "file.h"
#include "file_reader.h"

#define HTTP_BUF_SIZE 512

typedef enum {
    GET,
    HEAD
} method_t;

typedef enum {
    PARSE_OK,
    PARSE_INVALID_REQ,
    PARSE_INVALID_FILE,
    PARSE_FILE_NOT_FOUND,
    PARSE_METHOD_NOT_ALLOWED,
} parse_result_t;

typedef enum {
    RW_OK,
    RW_CONTINUE,
    RW_BAD_REQUEST,
    RW_REQUEST_TIMEOUT
} rw_result_t;


typedef struct {
    string_t *raw_http_header;
    method_t method;
    file_type_t *file_type;
    rw_result_t recv_header_result;
    parse_result_t parse_result;
} http_req_t;

typedef enum {
    S_OK = 200,
    S_BAD_REQUEST = 400,
    S_NOT_FOUND = 404,
    S_METHOD_NOT_ALLOWED = 405,
    S_REQUEST_TIMEOUT = 408,
    S_INTERNAL_ERROR = 500
} status_code_t;

typedef struct {
    int status_code;
    http_req_t *req;
    string_t *resp_header;
    file_reader_t *file_reader;
} http_resp_t;

http_req_t *new_http_req();

void free_http_req_t(http_req_t **req);

// парсит http хедер. Если вернулась ошибка - нужно очистить http_req_t
parse_result_t parse_http_header(http_req_t *result_req);

// true возвращается в случае, когда весь хедер считан и нужно сменить состояние connection на send_header
rw_result_t read_raw_header(int fd, http_req_t *req);

http_resp_t *new_http_resp(http_req_t *req);

void free_http_resp(http_resp_t **resp);

rw_result_t send_http_resp_header(int fd, http_resp_t *resp);

char *get_method_string(method_t method);

#endif //COURSE_WORK_HTTP_H
