#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include "http.h"
#include "logger.h"


char *get_code_string(status_code_t status_code) {
    switch (status_code) {
        case S_OK:
            return "OK";
        case S_BAD_REQUEST:
            return "Bad Request";
        case S_NOT_FOUND:
            return "Not Found";
        case S_METHOD_NOT_ALLOWED:
            return "Method Not Allowed";
        case S_REQUEST_TIMEOUT:
            return "Request Timeout";
        case S_INTERNAL_ERROR:
            return "Internal Server Error";
    }

    return "invalid";
}

http_resp_t *new_http_resp(http_req_t *req) {
    http_resp_t *resp = malloc(sizeof(http_resp_t));
    resp->req = req;
    resp->resp_header = new_string();

    if (req->recv_header_result != RW_OK) {
        switch (req->recv_header_result) {
            case RW_CONTINUE:
            case RW_BAD_REQUEST:
                resp->status_code = S_BAD_REQUEST;
                break;
            case RW_REQUEST_TIMEOUT:
                resp->status_code = S_REQUEST_TIMEOUT;
                break;
        }
    } else {
        switch (req->parse_result) {
            case PARSE_INVALID_REQ:
                resp->status_code = S_BAD_REQUEST;
                break;
            case PARSE_METHOD_NOT_ALLOWED:
                resp->status_code = S_METHOD_NOT_ALLOWED;
                break;
            case PARSE_INVALID_FILE:
                resp->status_code = S_BAD_REQUEST;
                break;
            case PARSE_FILE_NOT_FOUND:
                resp->status_code = S_NOT_FOUND;
                break;
            case PARSE_OK: // прочитали и распарсили успешно - значит статус 200 ОК
                resp->status_code = S_OK;
                break;
        }
    }

    /*expand_string(resp->resp_header);
    if (resp->status_code == S_OK) {
        resp->resp_header->length = sprintf(resp->resp_header->data, "HTTP/1.0 %d %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", resp->status_code,
                                            get_code_string(resp->status_code), get_file_extension_string(resp->req->file_type->file_extension), resp->req->file_type->file_length);
    } else {
        resp->resp_header->length = sprintf(resp->resp_header->data, "HTTP/1.0 %d %s\r\n\r\n", resp->status_code, get_code_string(resp->status_code));
    }*/
    char code_buf[HTTP_BUF_SIZE];
    sprintf(code_buf, "HTTP/1.1 %d %s\r\n", resp->status_code, get_code_string(resp->status_code));
    add_substr(resp->resp_header, code_buf, strlen(code_buf));
    memset(code_buf, '\0', HTTP_BUF_SIZE);

    if (resp->status_code == S_OK) { // если все ок - выставляем заголовки Content-Type и Content-Length
        sprintf(code_buf, "Content-Type: %s\r\nContent-Length: %ld\r\n\r\n",
                get_file_extension_string(resp->req->file_type->file_extension), resp->req->file_type->file_length);
        add_substr(resp->resp_header, code_buf, strlen(code_buf));
    } else {
        add_substr(resp->resp_header, "\r\n", 2);
    }

    return resp;
}

rw_result_t send_http_resp_header(int fd, http_resp_t *resp) {
    ssize_t wn;

    while (resp->resp_header->length > 0) {
        if ((wn = write(fd, resp->resp_header->data, resp->resp_header->length)) <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) { // ошибки нет, но мы не успели записать все - продолжим когда будет возможность
                return RW_CONTINUE;
            }
            return RW_REQUEST_TIMEOUT;
        }

        memmove(resp->resp_header->data, resp->resp_header->data + wn, resp->resp_header->length - wn);
        resp->resp_header->length -= wn;
    }

    return RW_OK;
}

void free_http_resp(http_resp_t **resp) {
    free_http_req_t(&(*resp)->req);
    free(*resp);
    *resp = NULL;
}

http_req_t *new_http_req() {
    http_req_t *req = malloc(sizeof(http_req_t));
    req->file_type = new_file_type();
    req->raw_http_header = new_string();

    return req;
}

void free_http_req_t(http_req_t **req) {
    if ((*req)->file_type != NULL) {
        free_file_type(&(*req)->file_type);
    }
    free_string(&(*req)->raw_http_header);
    free(*req);
    *req = NULL;
}

rw_result_t read_raw_header(int fd, http_req_t *req) {
    char read_buf[HTTP_BUF_SIZE];
    ssize_t read_bytes;

    while (1) {
        if ((read_bytes = read(fd, read_buf, HTTP_BUF_SIZE)) < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) { // пока больше нет данных на чтение
                return RW_CONTINUE;
            }
            return RW_REQUEST_TIMEOUT;
        } else if (read_bytes == 0) {
            return RW_BAD_REQUEST;
        }

        add_substr(req->raw_http_header, read_buf, read_bytes);
        // проверка на конец хедера
        if (req->raw_http_header->length >= 4 && !memcmp(req->raw_http_header->data + req->raw_http_header->length - 4,
                                                         "\r\n\r\n", 4)) {
            return RW_OK;
        }
        memset(read_buf, '\0', HTTP_BUF_SIZE);
    }
}

parse_result_t parse_http_header(http_req_t *result_req) {
    char *raw_header = result_req->raw_http_header->data;

    size_t header_length = result_req->raw_http_header->length;

    // GET / HTTP/1.1 - минимальный http запрос (длина 14 символов)
    if (header_length < 14) {
        return PARSE_INVALID_REQ;
    }

    int parse_offset = 0;

    if (strncmp(raw_header, "GET", sizeof("GET") - 1) == 0) {
        result_req->method = GET;
        parse_offset += 4; // GET + пробел
    } else if (strncmp(raw_header, "HEAD", sizeof("HEAD") - 1) == 0) {
        result_req->method = HEAD;
        parse_offset += 5; // HEAD + пробел
    } else {
        return PARSE_METHOD_NOT_ALLOWED;
    }

    char buf[HTTP_BUF_SIZE];
    int buf_offset = 0;

    // считываем путь, указанный в запросе
    while (parse_offset < header_length && raw_header[parse_offset] != ' ') {
        buf[buf_offset++] = raw_header[parse_offset++];
        if (buf_offset == HTTP_BUF_SIZE) {
            add_substr(result_req->file_type->file_path, buf, buf_offset);
            buf_offset = 0;
        }
    }

    ++parse_offset; // пропускаем пробел
    if (parse_offset >= header_length) {
        return PARSE_INVALID_REQ;
    }

    if (buf_offset != 0) {
        add_substr(result_req->file_type->file_path, buf, buf_offset);
    }
    add_substr(result_req->file_type->file_path, "\0", 1);

    // парсим путь до файла, обрабатывая внутри ошибки пути (в т.ч. отсутствия файла)
    file_parse_result_t file_parse_result = parse_file_path(result_req->file_type);
    if (file_parse_result == FILE_NOT_FOUND_F) { // если файл не найден - возвращаем соответствующую страницу
        return PARSE_FILE_NOT_FOUND;
    } else if (file_parse_result != OK_F) {
        return PARSE_INVALID_FILE;
    }

    // проверяем, что есть символы для парсинга версии http запроса
    if (parse_offset + sizeof("HTTP/0.0") - 1 > header_length) {
        return PARSE_INVALID_REQ;
    }

    // парсим версию HTTP
    if (strncmp(raw_header + parse_offset, "HTTP/", sizeof("HTTP/") - 1) != 0) {
        return PARSE_INVALID_REQ;
    }
    parse_offset += sizeof("HTTP/") - 1;
    if (raw_header[parse_offset + 1] != '.') {
        return PARSE_INVALID_REQ;
    }

    int first_prot_num = raw_header[parse_offset] - '0';
    int second_prot_num = raw_header[parse_offset + 2] - '0';

    if (first_prot_num < 0 || second_prot_num < 0 || first_prot_num > 9 || second_prot_num > 9) {
        return PARSE_INVALID_REQ;
    }

    return PARSE_OK;
}

char *get_method_string(method_t method) {
    switch (method) {
        case GET:
            return "GET";
        case HEAD:
            return "HEAD";
    }

    return "UNDEFINED";
}