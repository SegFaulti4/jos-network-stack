#include <inc/stdio.h>
#include <inc/string.h>
#include <kern/tcp.h>
#include <kern/http.h>
// temporary:
#include <kern/udp.h>

static const char *OK_page = "<html><body><h1>Hello from JOS!</h1></body></html>";
// static const char *Err_page = "<html><body><h1>Ooops! Somebodies marks are decreasing...</h1></body></html>";

int
http_parse(char *data, size_t length) {
    cprintf("Processing HTTP\n");
    struct HTTP_hdr hdr = {};
    char *word_start = data;
    size_t word_len = 0;
    for (int i = 0; i <= length; i++) {
        if (data[i] == ' ' || data[i] == '\n' || i == length) {
            word_len = data + i - word_start;
            if (!hdr.method.start) {
                if (strncmp(word_start, HTTP_METHOD, strlen(HTTP_METHOD))) {
                    cprintf("Only %s requests are supported!\n", HTTP_METHOD);
                    return http_reply(400, NULL);
                }
                hdr.method.start = word_start;
                hdr.method.length = word_len;
            } else if (!hdr.URI.start) {
                hdr.URI.start = word_start;
                hdr.URI.length = word_len;
            } else if (!hdr.HTTP_version.start) {
                if (strncmp(word_start, HTTP_VER, strlen(HTTP_VER))) {
                    cprintf("Only %s is supported!\n", HTTP_VER);
                    return http_reply(505, NULL);
                }
                hdr.HTTP_version.start = word_start;
                hdr.HTTP_version.length = word_len;
                break;
            }
            word_start += word_len + 1;
        }
    }
    if (!hdr.HTTP_version.start) {
        cprintf("HTTP header incomplete!\n");
        return http_reply(400, NULL);
    }

    return http_reply(200, OK_page);
}

int
http_reply(int code, const char *page) {
    static const char *messages[600] = {};
    if (!messages[200]) { // first init
        messages[200] = "200 OK";
        messages[400] = "400 Bad Request";
        messages[404] = "404 Not Found";
        messages[505] = "505 HTTP Version Not Supported";
        messages[520] = "520 Unknown Error";
    }
    if (code < 100 || code >= 600 || !messages[code]) {
        code = 520;
    }

    char reply[1024] = {};
    char *cur_pos = reply;
    memcpy(cur_pos, HTTP_VER, strlen(HTTP_VER));
    cur_pos += strlen(HTTP_VER);
    *cur_pos = ' ';
    cur_pos++;
    memcpy(cur_pos, messages[code], strlen(messages[code]));
    cur_pos += strlen(messages[code]);
    *cur_pos = '\n';
    cur_pos++;

    //memcpy(cur_pos, "\nContent-Length: ", strlen("\nContent-Length: "));
    //size_t page_len = strlen(page);
    //char page_len_text[10] = {};

    if (page) {
        memcpy(cur_pos, page, strlen(page));
        cur_pos += strlen(page);
    }
    *cur_pos = '\0';
    return udp_send(reply, cur_pos - reply);
}
