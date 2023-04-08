#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#define BUFF_SZ (10000)

static char buffer[BUFF_SZ];
static struct spinlock msg_lock;
static const char *ticks_prefix = "[";
static const char *ticks_postfix = "]\n";
static int front = 0, top = 0, words_count = 0;

void msg_init() {
    initlock(&msg_lock, "msg_lock");
}

void append_to_buffer(const char *str, int len) {
    for (int i = 0; i < len; i++) {
        buffer[top++] = str[i];
        top = (top + 1) % BUFF_SZ;
    }
}

int inc_top() {
    int res = top;
    top = (top + 1) % BUFF_SZ;
    return res;
}

int inc_front() {
    int res = front;
    front = (front + 1) % BUFF_SZ;
    return res;
}

static const char digits[] = "0123456789abcdef";

static void append_char(char c) {
    buffer[inc_top()] = c;
    if (top == front) {
        front++;
        if (words_count == 0)
            return;
        while (buffer[inc_front()] != '\0');
        words_count--;
    }
}

static void append_int(int xx, int base, int sign)
{
    char converter_buf[16];
    int i;
    uint x;

    if (sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;

    i = 0;
    do {
        converter_buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign)
        converter_buf[i++] = '-';

    while (--i >= 0)
        append_char(converter_buf[i]);
}

static void append_ptr(uint64 x) {
    append_char('0');
    append_char('x');
    for (int i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
        append_char(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

void append_fstr(const char *fmt, va_list ap) {
    int i, c;
    char *s;

    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (c != '%') {
            append_char(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if (c == 0)
            break;
        switch (c)
        {
        case 'd':
            append_int(va_arg(ap, int), 10, 1);
            break;
        case 'x':
            append_int(va_arg(ap, int), 16, 1);
            break;
        case 'p':
            append_ptr(va_arg(ap, uint64));
            break;
        case 's':
            if ((s = va_arg(ap, char *)) == 0)
                s = "(null)";
            for (; *s; s++)
                append_char(c);
            break;
        case '%':
            append_char('%');
            break;
        default:
            // Print unknown % sequence to draw attention.
            append_char('%');
            append_char(c);
            break;
        }
    }
    va_end(ap);
}

void pr_msg(const char *fmt, ...)
{
    acquire(&msg_lock);

    acquire(&tickslock);
    int current_ticks = ticks;
    release(&tickslock);

    va_list ap;
    va_start(ap, fmt);
    append_fstr(ticks_prefix, ap);
    append_int(current_ticks, 10, 0);
    append_fstr(ticks_postfix, ap);
    append_fstr(fmt, ap);
    va_end(ap);

    append_char('\0');
    words_count++;

    release(&msg_lock);
}

uint64 sys_dmesg(void)
{
    acquire(&msg_lock);

    uint64 addr;
    argaddr(0, &addr);
    if (front < top) {
        if (copyout(myproc()->pagetable, addr, buffer + front, sizeof(char) * (top - front)) < 0) {
            release(&msg_lock);
            return -1;
        }
        release(&msg_lock);
        return top - front;
    } else {
        if (copyout(myproc()->pagetable, addr, buffer + front, sizeof(char) * (BUFF_SZ - front)) < 0) {
            release(&msg_lock);
            return -1;
        }
        if (copyout(myproc()->pagetable, addr + sizeof(char) * (BUFF_SZ - front), buffer, sizeof(char) * top) < 0) {
            release(&msg_lock);
            return -1;
        }
        release(&msg_lock);
        return BUFF_SZ - front + top;
    }
}