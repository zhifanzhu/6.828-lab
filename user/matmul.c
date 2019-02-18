// Challenge of Lab4
// An implementation of Concurrent Matrix Multiplication
// See C. A. R. Hoare "Communicating Sequential Processes", section 6.2

#include <inc/lib.h>

#define TEMP_ADDR	((char*)0xa00000)

int array_lock[1];
typedef int* plock;

unsigned east(void);
unsigned west(void);
unsigned north(void);
unsigned south(void);
unsigned center(void);


void
umain(int argc, char **argv)
{
    int i, j;
    int id[3][3];

    // fork SOUTH
    i = 2;
    j = 1; 
    if ((id[i][j] = fork()) == 0) {
        south();
        return;
    }
    // fork CENTER
    i = 1;
    j = 1;
    if ((id[i][j] = fork()) == 0) {
        center();
        return;
    }   
    // fork NORTH
    i = 0;
    j = 1;
    if ((id[i][j] = fork()) == 0) {
        north();
        return;
    } 
    // fork WEST
    i = 1;
    j = 0;
    if ((id[i][j] = fork()) == 0) {
        west();
        return;
    }     
    // fork EAST
    i = 1;
    j = 2;
    if ((id[i][j] = fork()) == 0) {
        east();
        return;
    }     
    // init locks
    array_lock[0] = 1;

    sys_page_alloc(thisenv->env_id, TEMP_ADDR, PTE_P|PTE_W|PTE_U); 
    memcpy(TEMP_ADDR, array_lock, 1 * sizeof(array_lock[0]));


    // tell east the sender
    i = 1;
    j = 2;
    ipc_send(id[i][j], id[i][j-1], 0, 0);

    // tell south the name of center, and send page of lock
    i = 2;
    j = 1;
    ipc_send(id[i][j], id[i-1][j], 0, 0);
    
    // tell north, after this ,north begins to send
    i = 0;
    j = 1;
    ipc_send(id[0][j], id[1][j], 0, 0);
    ipc_send(id[0][j], 3*0+(j-1), TEMP_ADDR, PTE_P|PTE_U|PTE_W); 
    
    // tell west 
    i = 1; 
    j = 0;
    ipc_send(id[i][0], id[i][1], 0, 0);
    // tell them which lock to look at
    ipc_send(id[i][0], 3*(i-1)+0, TEMP_ADDR, PTE_P|PTE_U|PTE_W); 

    //
    // tell center
    // tell them whom to recv from
    i = 1;
    j = 1;
    ipc_send(id[i][j], id[i-1][j], 0, 0);
    ipc_send(id[i][j], id[i][j-1], 0, 0);
    // tell them whom to send to
    ipc_send(id[i][j], id[i+1][j], 0, 0);
    ipc_send(id[i][j], id[i][j+1], 0, 0);
    // tell them which lock to set
    ipc_send(id[i][j], 3*(i-1)+(j-1), TEMP_ADDR, PTE_P|PTE_U|PTE_W); 
}

unsigned
west(void) 
{
    int idx, x;
    envid_t to_env, envid;

    to_env = ipc_recv(&envid, 0, 0);
    idx = ipc_recv(&envid, TEMP_ADDR, 0);
    cprintf("WEST init complete\n");
    plock lk = (plock)TEMP_ADDR + idx;
    
    x = sys_getenvid() % 10;
    x = 8;
    int i;
    for (i = 0; i != 4; i++) {
        if (ipc_recv(&envid , 0, 0) == to_env) {
            ipc_send(to_env, x, 0, 0);
            cprintf("WEST: I sent x(%d)\n", x);
        }
        /* if (*lk == 1) { */
        /*     ipc_send(to_env, x, 0, 0); */
        /*     cprintf("WEST: I sent x(%d)\n", x); */
        /* } */
    }
    return 0;
}

unsigned
north(void) 
{
    int idx, send_val = 0;
    envid_t to_env, envid;

    to_env = ipc_recv(&envid, 0, 0);
    idx = ipc_recv(&envid, TEMP_ADDR, 0);
    cprintf("NORTH init complete\n");
    plock lk = (plock)TEMP_ADDR + idx;
    while(1) {
        if (ipc_recv(&envid, 0, 0) == to_env) {
            ipc_send(to_env, send_val, 0, 0);
            cprintf("NORTH: I sent 0(%d)\n", send_val);
        }
        /* if (*lk== 2) { */
        /*     ipc_send(to_env, send_val, 0, 0); */
        /*     cprintf("NORTH: I sent 0(%d)\n", send_val); */
        /* } */
        /* sys_yield(); */
    }
}

unsigned
center(void)
{
    int x, sum, idx;
    envid_t down, right, up, left, envid;
    up = ipc_recv(&envid, 0, 0);
    left = ipc_recv(&envid, 0, 0);
    down = ipc_recv(&envid, 0, 0);
    right = ipc_recv(&envid, 0, 0);
    idx = ipc_recv(&envid, TEMP_ADDR, 0);
    cprintf("CENTER init complete\n");

    plock lk = (plock)TEMP_ADDR + idx;

    ipc_send(left, sys_getenvid(), 0, 0); // ack west
    while(1) {
        x = ipc_recv(&envid, 0, 0); 
        assert(envid == left);
        cprintf("CENTER: I got x(%d)\n", x);
        *lk = 2;
        ipc_send(right, x, 0, 0);
        ipc_send(up, sys_getenvid(), 0, 0); // ack north

        sum = ipc_recv(&envid, 0, 0);
        assert(envid == up);
        cprintf("CENTER: I got sum(%d)\n", sum);
        int a_ij = sys_getenvid() % 10;
        a_ij = 7;
        ipc_send(down, a_ij * x + sum, 0, 0);
        *lk = 1;
        ipc_send(left, sys_getenvid(), 0, 0); // ack west
    }
    return 0;
}

unsigned
south(void)
{
    int sum;
    envid_t envid, from;
    from = ipc_recv(&envid, 0, 0);
    cprintf("SOUTH init complete\n");
    while(1) {
        sum = ipc_recv(&envid, 0, 0);
        cprintf("[%x]Sum: %d\n", sys_getenvid(), sum);
        assert(envid == from);
        /* sys_yield(); */
    }
}

unsigned
east(void) 
{
    int x;
    envid_t envid, from;
    from = ipc_recv(&envid, 0, 0);
    cprintf("EAST init complete\n");
    while(1) {
        x = ipc_recv(&envid, 0, 0);
        cprintf("EAST: I got x(%d)\n", x);
        assert(envid == from);
        /* sys_yield(); */
    }
}
