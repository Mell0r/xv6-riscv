#include "types.h"
#include "param.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"

#define SLEEPLOCK_CONTAINER_SIZE 1024

static struct sleeplock sleeplock_container[SLEEPLOCK_CONTAINER_SIZE];
static struct spinlock slc_lock;
static char slc_used[SLEEPLOCK_CONTAINER_SIZE];

void slcinit() {
    initlock(&slc_lock, "slc_lock");
    for (int i = 0; i < SLEEPLOCK_CONTAINER_SIZE; i++)
        initsleeplock(sleeplock_container + i, "");
}

uint64
sys_create_lock(void) {
    acquire(&slc_lock);
    int free_lock_number = -1;
    for (int i = 0; i < SLEEPLOCK_CONTAINER_SIZE; i++) {
        if (!slc_used[i]) {
            free_lock_number = i;
            break;
        }
    }
    slc_used[free_lock_number] = 1;
    release(&slc_lock);
    return free_lock_number;
}

uint64
sys_switch_lock(void) {
    acquire(&slc_lock);
    int lock_number;
    argint(0, &lock_number);
    if (lock_number < 0 || lock_number >= SLEEPLOCK_CONTAINER_SIZE) {
        release(&slc_lock);
        return -1;
    }
    if (slc_used[lock_number] == 0) {
        release(&slc_lock);
        return -1;
    }

    int request;
    argint(1, &request);
    release(&slc_lock);
    if (request == 0)
        acquiresleep(sleeplock_container + lock_number);
    else if (request == 1)
        releasesleep(sleeplock_container + lock_number);
    else if (request == 2)
        slc_used[lock_number] = 0;
    else
        return -1;
    return 0;
}