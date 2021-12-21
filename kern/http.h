#ifndef JOS_KERN_HTTP_H
#define JOS_KERN_HTTP_H

struct str_part {
    char *start;
    size_t length;
};

struct HTTP_hdr {
    struct str_part method, URI, HTTP_version;
};

int http_parse(char *data, size_t length);
int http_reply(int code, const char *page);

#define HTTP_METHOD "GET"
#define HTTP_VER "HTTP/1.1"

#endif
