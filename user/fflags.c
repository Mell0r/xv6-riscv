#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        write(2, "Wrong number of arguments. Should be 3.", 39);
        exit(-1);
    }

    fflags(argv[1][0] - '0', argv[2][0] - '0', argv[3][0] - '0');
    return 0;
}