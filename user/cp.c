#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int src, dst;
    int n;
    char buffer[1024];
    struct stat src_stat, dst_stat;

    if (argc != 3) {
        fprintf(2, "cp: usage: cp source destination\n");
        exit(1);
    }

    // Mo file nguon de doc.
    src = open(argv[1], O_RDONLY);
    if (src < 0) {
        fprintf(2, "cp: cannot open %s\n", argv[1]);
        exit(1);
    }

    if (fstat(src, &src_stat) < 0) {
        fprintf(2, "cp: cannot stat %s\n", argv[1]);
        close(src);
        exit(1);
    }

    if (src_stat.type != T_FILE) {
        fprintf(2, "cp: %s is not a regular file\n", argv[1]);
        close(src);
        exit(1);
    }

    // Kiem tra truoc khi xoa noi dung cu cua file dich.
    if (stat(argv[2], &dst_stat) == 0) {
        if (src_stat.dev == dst_stat.dev &&
            src_stat.ino == dst_stat.ino) {
            fprintf(2, "cp: source and destination are the same file\n");
            close(src);
            exit(1);
        }

        if (dst_stat.type != T_FILE) {
            fprintf(2, "cp: %s is not a regular file\n", argv[2]);
            close(src);
            exit(1);
        }
    }

    // Tao file neu chua co; xoa noi dung cu neu da co.
    dst = open(argv[2], O_WRONLY | O_CREATE | O_TRUNC);
    if (dst < 0) {
        fprintf(2, "cp: cannot open %s\n", argv[2]);
        close(src);
        exit(1);
    }

    // Doc tung khoi, ghi het khoi do roi moi doc tiep.
    while ((n = read(src, buffer, sizeof(buffer))) > 0) {
        int written = 0;

        while (written < n) {
            int count = write(dst, buffer + written, n - written);

            if (count <= 0) {
                fprintf(2, "cp: write failed for %s\n", argv[2]);
                close(src);
                close(dst);
                exit(1);
            }

            written += count;
        }
    }

    if (n < 0) {
        fprintf(2, "cp: read failed for %s\n", argv[1]);
        close(src);
        close(dst);
        exit(1);
    }

    close(src);
    close(dst);
    exit(0);
}
