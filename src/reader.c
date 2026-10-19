#include "reader.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
struct Line_reader {
    int fd;
    char buf[LRG_BUFSIZE];
    size_t start, end;
    char *line;
    size_t len, cap;
    int eof;
};
struct Line_reader *reader_create(int fd) {
    struct Line_reader *r = calloc(1, sizeof *r);
    if (r == NULL)
        return NULL;
    r->fd = fd;
    return r;
}
void reader_destroy(struct Line_reader *r) {
    free(r->line);
    free(r);
}
static int line_append(struct Line_reader *r, const char *data, size_t n) {
    size_t need = r->len + n + 1;
    if (need > r->cap) {
        size_t newcap = (r->cap == 0) ? 128 : r->cap;
        while (newcap < need)
            newcap *= 2;
        char *p = realloc(r->line, newcap);
        if (p == NULL)
            return -1;
        r->line = p;
        r->cap = newcap;
    }
    memcpy(r->line + r->len, data, n);
    r->len += n;
    r->line[r->len] = '\0';
    return 0;
}
int reader_next_line(struct Line_reader *r, const char **line, size_t *len) {
    for (;;) {
        if (r->start == r->end) {
            if (r->eof)
                return 0;
            ssize_t n = read(r->fd, r->buf, sizeof r->buf);
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                return -1;
            }
            if (n == 0) {
                r->eof = 1;
                if (r->len > 0) {
                    *line = r->line;
                    *len = r->len;
                    r->len = 0;
                    return 1;
                }
                return 0;
            }
            r->start = 0;
            r->end = n;
        }
        char *p = memchr(r->buf + r->start, '\n', r->end - r->start);
        if (p != NULL) {
            size_t seglen = p - (r->buf + r->start);
            if (line_append(r, r->buf + r->start, seglen) < 0)
                return -1;
            r->start = r->start + seglen + 1;
            *line = r->line;
            *len = r->len;
            r->len = 0;
            return 1;
        } else {
            if (line_append(r, r->buf + r->start, r->end - r->start) < 0)
                return -1;
            r->start = r->end;
        }
    }
}