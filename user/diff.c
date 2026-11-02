#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// Tra ve: so byte cua dong, 0 neu EOF, so am neu loi.
// Giu ca newline de phan biet dong cuoi co/khong co newline.
static int
read_line(int fd, char **buffer, int *capacity)
{
    int length = 0;

    for (;;) {
        char c;
        int n = read(fd, &c, 1);

        if (n < 0)
            return -1;

        if (n == 0)
            return length;

        if (length == *capacity) {
            if (*capacity >= (1 << 28))
                return -2;

            int new_capacity = *capacity ? *capacity * 2 : 128;
            char *new_buffer = malloc(new_capacity);

            if (new_buffer == 0)
                return -2;

            if (length > 0)
                memmove(new_buffer, *buffer, length);
            if (*buffer != 0)
                free(*buffer);
            *buffer = new_buffer;
            *capacity = new_capacity;
        }

        (*buffer)[length++] = c;

        if (c == '\n')
            return length;
    }
}

static int
same_line(char *a, int na, char *b, int nb)
{
    if (na != nb)
        return 0;

    for (int i = 0; i < na; i++) {
        if (a[i] != b[i])
            return 0;
    }

    return 1;
}

static void
print_line(char marker, char *name, int number,
           char *buffer, int length)
{
    printf("%s %s:%d: ", marker == '<' ? "<" : ">", name, number);

    // Khong dung %s vi buffer khong can ky tu ket thuc '\0'.
    write(1, buffer, length);

    if (buffer[length - 1] != '\n') {
        printf("\n");
        printf("\\ No newline at end of file\n");
    }
}

static int
open_file(char *name)
{
    struct stat st;
    int fd = open(name, O_RDONLY);

    if (fd < 0) {
        fprintf(2, "diff: cannot open %s\n", name);
        return -1;
    }

    if (fstat(fd, &st) < 0 || st.type != T_FILE) {
        fprintf(2, "diff: %s is not a regular file\n", name);
        close(fd);
        return -1;
    }

    return fd;
}

int
main(int argc, char *argv[])
{
    char *names[2];
    int file_count = 0;
    int quiet = 0;

    // Chap nhan -q truoc hoac sau hai ten file.
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0) {
            if (quiet)
                goto usage;
            quiet = 1;
        } else {
            if (argv[i][0] == '-' || file_count == 2)
                goto usage;

            names[file_count++] = argv[i];
        }
    }

    if (file_count != 2)
        goto usage;

    int fd1 = open_file(names[0]);
    if (fd1 < 0)
        exit(2);

    int fd2 = open_file(names[1]);
    if (fd2 < 0) {
        close(fd1);
        exit(2);
    }

    char *line1 = 0;
    char *line2 = 0;
    int capacity1 = 0;
    int capacity2 = 0;
    int number = 1;
    int result = 0;

    for (;;) {
        int n1 = read_line(fd1, &line1, &capacity1);
        int n2 = read_line(fd2, &line2, &capacity2);

        if (n1 < 0 || n2 < 0) {
            if (n1 == -2 || n2 == -2)
                fprintf(2, "diff: cannot allocate memory for line\n");
            else
                fprintf(2, "diff: read error\n");

            result = 2;
            break;
        }

        if (n1 == 0 && n2 == 0)
            break;

        if (!same_line(line1, n1, line2, n2)) {
            result = 1;

            if (quiet) {
                printf("diff: files differ\n");
                break;
            }

            if (n1 > 0)
                print_line('<', names[0], number, line1, n1);

            if (n2 > 0)
                print_line('>', names[1], number, line2, n2);
        }

        number++;
    }

    if (line1 != 0) {
        free(line1);
    }
    if (line2 != 0) {
        free(line2);
    }
    close(fd1);
    close(fd2);
    exit(result);

usage:
    fprintf(2, "diff: usage: diff file1 file2 [-q]\n");
    exit(2);
}
