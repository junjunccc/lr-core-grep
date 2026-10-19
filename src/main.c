#include "reader.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
struct Args {
    int opt_n;
    int opt_i;
    int opt_c;
    const char *pattern;
    size_t plen;
    int nfiles;
    char **files;
};
static int fold(int c) {
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}
static int byte_eq(unsigned char a, unsigned char b, int ic) {
    if (!ic)
        return a == b;
    return fold(a) == fold(b);
}
static int contains(const char *hay, size_t hlen, const char *needle,
                    size_t nlen, int ic) {
    if (nlen == 0)
        return 1;
    if (nlen > hlen)
        return 0;

    for (size_t i = 0; i + nlen <= hlen; i++) {
        size_t j = 0;
        while (j < nlen && byte_eq(hay[i + j], needle[j], ic))
            j++;
        if (j == nlen)
            return 1;
    }
    return 0;
}
static long process_input(const char *name, int fd, const struct Args *args) {
    struct Line_reader *r = reader_create(fd);
    if (r == NULL)
        return -1;
    const char *line;
    size_t len;
    size_t lineno = 0;
    long matched = 0;
    int rc;
    while ((rc = reader_next_line(r, &line, &len)) == 1) {
        lineno++;
        if (!contains(line, len, args->pattern, args->plen, args->opt_i))
            continue;
        matched++;
        if (args->opt_c)
            continue;
        if (args->nfiles > 1)
            printf("%s:", name);
        if (args->opt_n)
            printf("%zu:", lineno);
        fwrite(line, 1, len, stdout);
        putchar('\n');
    }
    reader_destroy(r);
    if (rc < 0)
        return -1;
    if (args->opt_c) {
        if (args->nfiles > 1)
            printf("%s:", name);
        printf("%ld\n", matched);
    }
    return matched;
}
int main(int argc, char *argv[]) {
    struct Args args = {0};
    int i = 1;
    for (; i < argc; i++) {
        char *arg = argv[i];
        if (arg[0] != '-' || arg[1] == '\0')
            break;
        for (int j = 1; arg[j] != '\0'; j++) {
            char c = arg[j];
            if (c == 'n')
                args.opt_n = 1;
            else if (c == 'i')
                args.opt_i = 1;
            else if (c == 'c')
                args.opt_c = 1;
            else {
                fprintf(stderr, "usage: lrg [-nic] PATTERN [FILE...]\n");
                return 2;
            }
        }
    }
    if (i >= argc) {
        fprintf(stderr, "usage: lrg [-nic] PATTERN [FILE...]\n");
        return 2;
    }
    args.pattern = argv[i];
    args.plen = strlen(args.pattern);
    i++;
    args.nfiles = argc - i;
    args.files = argv + i;
    long total = 0;
    int had_error = 0;
    if (args.nfiles > 0) {
        for (int k = 0; k < args.nfiles; k++) {
            const char *name = args.files[k];
            int fd = open(name, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "lrg: %s: %s\n", name, strerror(errno));
                had_error = 1;
                continue;
            }
            long m = process_input(name, fd, &args);
            close(fd);
            if (m < 0) {
                fprintf(stderr, "lrg: %s: %s\n", name, strerror(errno));
                had_error = 1;
            } else {
                total += m;
            }
        }
    } else {
        long m = process_input("(standard input)", STDIN_FILENO, &args);
        if (m < 0) {
            fprintf(stderr, "lrg: (standard input): %s\n", strerror(errno));
            had_error = 1;
        } else {
            total += m;
        }
    }
    if (had_error)
        return 2;
    if (total > 0)
        return 0;
    return 1;
}