#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

char *flags = "PWUTCADSGV";

static uintptr_t
str2va(char *s) // Used in mon_showmappings
{
    uintptr_t va = strtol(s, NULL, 0);
    return (va >= 0 && va <= 0xFFFFFFFF) ? va : -1;
}

static void
display_entry(pde_t *pgdir, uintptr_t va_begin, uintptr_t va_end) 
{
    // For single entry
    int perm;
    int ind;
    pte_t *pt_entry;
    pt_entry = pgdir_walk(pgdir, (void *)va_begin, false);    

    cprintf("0x%08x: ", va_begin);
    if (!pt_entry || !((*pt_entry) & PTE_P))
        cprintf("Not mapped\n");
    else {
        perm = (*pt_entry) & 0xFFF;
        for (ind = strlen(flags) - 1; ind >= 0; ind--)
            cprintf("%c", perm & (0x1 << ind) ? flags[ind] : '-');
        cprintf("  %08x\n",PTE_ADDR(*pt_entry));
    }
    if (va_begin + PGSIZE < va_begin)
        return;
    va_begin += PGSIZE;
    if (va_begin > va_end)
        return;
    
    // For range of entry
    int skipping = 0;
    int prev_perm = perm;
    int prev_addr = (pt_entry ? PTE_ADDR(*pt_entry) : 0);
    int prev_va = va_begin - PGSIZE;
    pde_t *prev_entry = pt_entry;
    
    for (;;) {
        pt_entry = pgdir_walk(pgdir, (void *)va_begin, false);    
        if (!pt_entry || !((*pt_entry) & PTE_P)) {
            if (!prev_entry || !((*prev_entry) & PTE_P))
                skipping = 1;
            else {
                if (skipping) {                    
                    cprintf("                ...\n");
                    cprintf("0x%08x: ", prev_va);
                    for (ind = strlen(flags) - 1; ind >= 0; ind--) 
                        cprintf("%c", prev_perm & (0x1 << ind) ? flags[ind] : '-');
                    cprintf("  %08x\n",prev_addr);
                    cprintf("====================================\n");
                }
                skipping = 0;
                cprintf("0x%08x: ", va_begin);
                cprintf("Not mapped\n");                
            }
            prev_va = va_begin;
            prev_entry = pt_entry;
        } else if ((perm = (*pt_entry) & 0xFFF) == prev_perm
                && (((int)PTE_ADDR(*pt_entry)- prev_addr == PGSIZE)
                    || ((int)PTE_ADDR(*pt_entry)- prev_addr == -PGSIZE))
                ) {
            skipping = 1;
            prev_addr = PTE_ADDR(*pt_entry);
            prev_va = va_begin;
            prev_entry = pt_entry;
        } else {
            if (skipping) {
                cprintf("                ...\n");
                if (!prev_entry || !((*prev_entry) & PTE_P)) {
                    cprintf("0x%08x: ", prev_va);
                    cprintf("Not mapped\n");
                } else {
                    cprintf("0x%08x: ", prev_va);
                    for (ind = strlen(flags) - 1; ind >= 0; ind--) {
                        cprintf("%c", prev_perm & (0x1 << ind) ? flags[ind] : '-');
                    }
                    cprintf("  %08x\n",prev_addr);
                }
                cprintf("====================================\n");
            }

            cprintf("0x%08x: ", va_begin);
            for (ind = strlen(flags) - 1; ind >= 0; ind--) {
                cprintf("%c", perm & (0x1 << ind) ? flags[ind] : '-');
            }
            cprintf("  %08x\n",PTE_ADDR(*pt_entry));
            skipping = 0;
            prev_perm = perm;
            prev_va = va_begin;
            prev_addr = PTE_ADDR(*pt_entry);
            prev_entry = pt_entry;
        }
        if (va_begin + PGSIZE < va_begin)
            break;
        va_begin += PGSIZE;
        if (va_begin > va_end)
            break;
    }
    if (skipping) {
        cprintf("                ...\n");
        if (!prev_entry || !((*prev_entry) & PTE_P)) {
            cprintf("0x%08x: ", prev_va);
            cprintf("Not mapped\n");
        } else {
            cprintf("0x%08x: ", prev_va);
            for (ind = strlen(flags) - 1; ind >= 0; ind--) {
                cprintf("%c", prev_perm & (0x1 << ind) ? flags[ind] : '-');
            }
            cprintf("  %08x\n",prev_addr);
        }
    }
}

static void
memory_dump(pde_t *pgdir, uintptr_t va_begin, int N)
{
    // boundary is page aligned.
    uintptr_t va_end = va_begin + N;
    uintptr_t boundary = ROUNDUP(va_begin, PGSIZE);
    pte_t *pt_entry = pgdir_walk(pgdir, (void *)va_begin, false);
    uint32_t buf;
    int col_count = 0;

    for(; va_begin < va_end; ) {
        if (va_begin >= boundary) {
            pt_entry = pgdir_walk(pgdir, (void *)va_begin, false);
            boundary += PGSIZE;
        }
        while (!pt_entry || !((*pt_entry) & PTE_P)) {
            cprintf("Can't access mem here.\n");
            va_begin += PGSIZE;
            va_begin = ROUNDDOWN(va_begin, PGSIZE);
            boundary += PGSIZE;
            if (va_begin >= va_end)
                return;
            pt_entry = pgdir_walk(pgdir, (void *)va_begin, false);
        }

        if (!(col_count++ % 4))
            cprintf("0x%08x: ", va_begin);

        // Crossing boundary
        if (va_begin + sizeof(uint32_t) > boundary) {
            uintptr_t tmp_end = va_begin + sizeof(uint32_t);
            buf = 0;
            int i = 0;
            for (;;) {
                buf |= *(uint8_t *)va_begin << (i*8);
                va_begin += sizeof(uint8_t);
                i++;
                if (va_begin == boundary)
                    break;
            }
            pt_entry = pgdir_walk(pgdir, (void *)va_begin, false);
            if (!pt_entry || !((*pt_entry) & PTE_P)) {
                cprintf("%08x ", buf);
                cprintf("Can't access mem after boundary.\n");
                return;
            }
            for (;;) {
                buf |= *(uint8_t *)va_begin << (i*8);
                va_begin += sizeof(uint8_t);
                i++;
                if (va_begin == tmp_end )
                    break;
            }
            cprintf("%08x ", buf);
        } else {
            cprintf("%08x ", *(uint32_t *)va_begin);
            va_begin += sizeof(uint32_t);
        }
        if (!(col_count % 4))
            cprintf("\n");
    }
    if ((col_count % 4))
        cprintf("\n");
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
    // translate AVL bits to single V bit
    if (argc == 1) {
        cprintf("Fault: no arguments\n\n");
        goto incorrect;
    }

    uintptr_t va_begin, va_end;
    pte_t *pt_entry;
    pde_t *pgdir = (pde_t *)KADDR(rcr3());

    if (argc == 2) {
        va_begin = str2va(argv[1]);
        display_entry(pgdir, va_begin, va_begin);
        return 0;
    }

    // Range output
    char *option = argv[1];
    if (option[0] != '-') {
        if (argc > 3) {
            cprintf("Too many arguments\n\n");
            goto incorrect;
        }
        va_begin = str2va(argv[1]);
        va_end = str2va(argv[2]);
        display_entry(pgdir, va_begin, va_end);
        return 0;
    }

    //Dump 
    if (option[1] == 'd') {
        if (argc > 3) {
            cprintf("Too many arguments\n\n");
            goto incorrect;
        }
        va_begin = str2va(argv[2]);
        if (!strncmp(option, "-dp", 3)) {
            if (PGNUM(va_begin) >= npages) {
                cprintf("Err: dump physical addr too high\n");
                return 0;
            }
            va_begin = (uintptr_t)KADDR(va_begin);
        } else if (!strncmp(option, "-dv", 3)) {
            ;
        } else {
            cprintf("Input error: can't read after -d\n");
            return 0;
        }

        int num_to_disp = sizeof(uint32_t);
        if (option[3] == '/') {
            num_to_disp *= strtol(&option[4], 0, 0);
            if (num_to_disp <= 0) {
                cprintf("Input error: N <= 0\n");
                return 0;
            }
        } else if (option[3]) {
            cprintf("Input error: can't read after -d\n");
            return 0;
        }
        memory_dump(pgdir, va_begin, num_to_disp);
        return 0;
    } 

    // Setting perm
    if (!strcmp(option, "-s")) {
        if (argc <= 3) 
            goto incorrect;
        va_begin = str2va(argv[2]);
        pt_entry = pgdir_walk(pgdir, (void *)va_begin, false);
        if (!pt_entry || !((*pt_entry) & PTE_P)) {
            cprintf("Not mapped\n");
            return 0;
        }
        int perm, new_perm;
        int i, j;
        perm = (*pt_entry) & 0xFFF;

        if (!strcmp(argv[3], "--clear") && argc == 4) {
            new_perm = PTE_P;
        } else if (!strcmp(argv[3], "--add") && argc > 4) {
            i = 4;
            new_perm = perm;
            for (i = 3; i != argc; i++)
                for (j = 0; j != strlen(flags); j++)
                    if (strchr(argv[i], flags[j]) )
                        new_perm |= (0x1 << j);
        } else {
            i = 3;
            new_perm = PTE_P;
            for (i = 3; i != argc; i++)
                for (j = 0; j != strlen(flags); j++) 
                    if (strchr(argv[i], flags[j]) )
                        new_perm |= (0x1 << j);
        }         
        cprintf("New permissions:\n");
        *pt_entry = PTE_ADDR(*pt_entry) | new_perm;
        display_entry(pgdir, va_begin, va_begin);
        return 0;
    }

incorrect:
    
    cprintf("Format: pg [options] va_begin [va_end] [param]\n"
            "[options]: \n"
            "   -dp: (-dp/N) dump physical address, default N=1\n"
            "   -dv: (-dv/N) dump virtual address, default N=1\n"
            "   -s: set permissions\n"
            "       [params]:\n"
            "       --add   add to existing\n"
            "       --clear clear all permission bits\n"
            "Examples:\n"
            "   pg 0x3000\n"
            "   pg -dv/4 0xef000000\n"
            "   pg -s 0xef000000 --add D A U P\n"
            );
    return 0;
}
