#include "param.h"
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"

void print_pages(pagetable_t pagetable, int level, int last_accessed) {
    if (level > 2)
        return;
    for(int i = 0; i < 512; i++){
        pte_t pte = pagetable[i];
        if(pte & PTE_V) {
            if ((pte & PTE_A) | !last_accessed) {
                for (int k = 0; k < level; k++)
                    printf(" ..");
                printf("%d: pte %p pa %p | flags: Valid %d, Readable %d, Writable %d, Executable %d, User %d, Global %d, Accessed %d, Dirty %d\n", 
                i, pte, PTE2PA(pte), (pte & PTE_V) > 0, 
                (pte & PTE_R) > 0, (pte & PTE_W) > 0,
                (pte & PTE_X) > 0, (pte & PTE_U) > 0,
                (pte & PTE_G) > 0, (pte & PTE_A) > 0, (pte & PTE_D) > 0);
                if (last_accessed)
                    pagetable[i] ^= PTE_A;
            }
            print_pages((pagetable_t)PTE2PA(pte), level + 1, last_accessed);
        }
    }
}

uint64 sys_vmprint() {
    struct proc* cur_proc = myproc();
    if (cur_proc == 0)
        return -1;
    printf("page table %p\n", myproc()->pagetable);
    print_pages(myproc()->pagetable, 0, 0);
    return 0;
}

uint64 sys_pgaccess() {
    struct proc* cur_proc = myproc();
    if (cur_proc == 0)
        return -1;
    printf("page table %p\n", myproc()->pagetable);
    print_pages(myproc()->pagetable, 0, 1);
    return 0;
}