#ifndef LRG_READER_H
#define LRG_READER_H
#include <stddef.h>
#define LRG_BUFSIZE 65536
struct Line_reader;
struct Line_reader *reader_create(int fd);
void reader_destroy(struct Line_reader *r);
int reader_next_line(struct Line_reader *r, const char **line, size_t *len);
#endif