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
    uintptr_t va = strtol(s, NULL, 16);
    return (va >= 0 && va <= 0xFFFFFFFF) ? va : -1;
}

static void
display_entry(pte_t *pt_entry) 
{
    if (!pt_entry) {
       cprintf("Not mapped\n");
       return;
    }
    int perm;
    int ind;
    perm = (*pt_entry) & 0xFFF;
    for (ind = strlen(flags) - 1; ind >= 0; ind--) {
        cprintf("%c", perm & (0x1 << ind) ? flags[ind] : '-');
    }
    cprintf("  %08x\n",PTE_ADDR(*pt_entry));
}

static void
memory_dump(pde_t *pgdir, uintptr_t va_begin, int N)
{
    // boundary is page aligned.
    uintptr_t va_end = va_begin + N;
    uintptr_t boundary = ROUNDUP(va_begin, PGSIZE);
    pte_t *pt_entry = pgdir_walk(pgdir, (void *)va_begin, false);
    uint32_t buf;

    for(; va_begin < va_end; ) {
        if (va_begin >= boundary) {
            pt_entry = pgdir_walk(pgdir, (void *)va_begin, false);
            boundary += PGSIZE;
        }
        while (!pt_entry) {
            cprintf("Can't access mem here.\n");
            va_begin += PGSIZE;
            va_begin = ROUNDDOWN(va_begin, PGSIZE);
            boundary += PGSIZE;
            if (va_begin >= va_end) {
                return;
            }
            pt_entry = pgdir_walk(pgdir, (void *)va_begin, false);
        }

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
            if (!pt_entry) {
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
        cprintf("\n");
    }
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
    // Has defect in setting AVL bits
    if (argc == 1) {
        cprintf("Fault: no arguments\n");
        goto incorrect;
    }

    uintptr_t va_begin, va_end;
    pte_t *pt_entry;
    pde_t *pgdir = (pde_t *)KADDR(rcr3());

    if (argc == 2) {
        va_begin = str2va(argv[1]);
        pt_entry = pgdir_walk(pgdir, (void *)va_begin, false);
        cprintf("0x%08x  ", va_begin);
        display_entry(pt_entry);
        return 0;
    }

    char *option = argv[1];
    // [va_begin, va_end] easy to read
    if (option[0] != '-') {
        va_begin = str2va(argv[1]);
        va_end = str2va(argv[2]);   
        for (;;) {
            pt_entry = pgdir_walk(pgdir, (void *)va_begin, false);
            cprintf("0x%08x  ", va_begin);        
            display_entry(pt_entry);
            va_begin += PGSIZE;
            if (va_begin > va_end)
                break;
        }
        return 0;
    }

    //Dump 
    // Case 2: N >= 4, easy to read
    if (option[1] == 'd') {
        va_begin = str2va(argv[2]);
        if (!strncmp(option, "-dp", 3)) {
            if (PGNUM(va_begin) >= npages) {
                cprintf("Err: physical addr too high\n");
                return 0;
            }
            va_begin = (uintptr_t)KADDR(va_begin);
        } else if (!strncmp(option, "-dv", 3)) {
            ;
        }
        else {
            cprintf("Err: cannot read after -d\n");
            goto incorrect;
        }

        int num_to_disp = sizeof(uint32_t);
        if (option[3] == '/') {
            num_to_disp *= strtol(&option[4], 0, 0);
            if (num_to_disp <= 0) {
                cprintf("Err: N <= 0\n");
                goto incorrect;
            }
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
        if (!pt_entry) {
            cprintf("Not mapped\n");
            return 0;
        }
        int perm, new_perm;
        int i, j;
        perm = (*pt_entry) & 0xFFF;

        if (!strcmp(argv[3], "--clear") && argc == 4) {
            new_perm = 0;
        } else if (!strcmp(argv[3], "--add") && argc > 4) {
            i = 4;
            new_perm = perm;
            for (i = 3; i != argc; i++)
                for (j = 0; j != strlen(flags); j++)
                    if (strchr(argv[i], flags[j]) )
                        new_perm |= (0x1 << j);
        } else {
            i = 3;
            new_perm = 0;
            for (i = 3; i != argc; i++)
                for (j = 0; j != strlen(flags); j++) 
                    if (strchr(argv[i], flags[j]) )
                        new_perm |= (0x1 << j);
        }         
        cprintf("New permissions:\n");
        cprintf("0x%08x  ", va_begin);
        *pt_entry = PTE_ADDR(*pt_entry) | new_perm;
        display_entry(pt_entry);
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
            );
    return 0;
}
