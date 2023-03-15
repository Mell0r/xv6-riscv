#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

unsigned int safe_read(int fd, char* buf, int n) {
    int was_read = read(fd, buf, n);
    if (was_read < 0) {
        write(2, "Unsuccsessful write attempt!", 28);
        exit(-1);
    }
    return was_read;
}

unsigned int safe_write(int fd, char* buf, int n) {
    int was_written = write(fd, buf, n);
    if (was_written < 0) {
        write(2, "Unsuccsessful write attempt!", 28);
        exit(-1);
    }
    return was_written;
}

void safe_close(int fd) {
    int res = close(fd);
    if (res == -1) {
        write(2, "Unsuccsessful close attempt!", 28);
        exit(-1);
    }
}

void safe_lock_switch(int lock_number, int request) {
    int res = switch_lock(lock_number, request);
    if (res == -1) {
        write(2, "Unsuccsessful lock switch attempt!", 34);
        exit(-1);
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        write(2, "Not enough arguments.", 21);
        exit(-1);
    }

    int p_to_ch[2], ch_to_p[2];
    if (pipe(p_to_ch) == -1 || pipe(ch_to_p) == -1) {
        write(2, "Cannot create pipe!", 19);
        exit(-1);
    }

    int print_lock = create_lock();
    if (print_lock == -1) {
        write(2, "Cannot create lock!", 19);
        exit(-1);
    }
    if (fork() == 0) {
        safe_close(ch_to_p[0]);
        safe_close(p_to_ch[1]);

        char a;
        int pid = getpid();
        if (pid == -1) {
            write(2, "Cannot get pid!", 15);
            exit(-1);
        }
        while (safe_read(p_to_ch[0], &a, 1)) {
            safe_lock_switch(print_lock, 0);

            fprintf(0, "%d: received %c\n", pid, a);
            safe_write(ch_to_p[1], &a, 1);

            safe_lock_switch(print_lock, 1);
        }
        safe_close(ch_to_p[1]);
        safe_close(p_to_ch[0]);
    }
    else {
        safe_close(p_to_ch[0]);
        safe_close(ch_to_p[1]);

        char* inp = argv[1];
        while (*inp != 0) 
            safe_write(p_to_ch[1], inp++, 1);

        safe_close(p_to_ch[1]);

        char a;
        int pid = getpid();
        while (safe_read(ch_to_p[0], &a, 1)) {
            safe_lock_switch(print_lock, 0);

            fprintf(0, "%d: received %c\n", pid, a);

            safe_lock_switch(print_lock, 1);
        }
        safe_close(ch_to_p[0]);
    }
    exit(0);
}