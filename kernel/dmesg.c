#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "stat.h"

#define BUFF_SZ (1 << 20)
#define TICKS_CONVERTER_SZ 10

static char buffer[BUFF_SZ];
static char ticks_str[TICKS_CONVERTER_SZ];
static const char* ticks_prefix = " | Ticks: ";
static int top = 0;

void pr_msg(const char* str) {
    acquire(&tickslock);
    int current_ticks = ticks;
    release(&tickslock);

    while (top < BUFF_SZ && *str != 0)
        buffer[top++] = *(str++);
    for (int i = 0; i < strlen(ticks_prefix) && top < BUFF_SZ; i++)
        buffer[top++] = ticks_prefix[i];

    int ticks_len = (current_ticks == 0);
    ticks_str[0] = '0';
    while (current_ticks > 0 && ticks_len < BUFF_SZ) {
        ticks_str[ticks_len++] = '0' + current_ticks % 10;
        current_ticks /= 10;
    }

    while (ticks_len > 0 && top < BUFF_SZ)
        buffer[top++] = ticks_str[(ticks_len--) - 1];
    buffer[top++] = '\n';
}

uint64 
sys_dmesg(void) {
    for (int i = 0; i < top; i++)
        consputc(buffer[i]);
    return 0;
}