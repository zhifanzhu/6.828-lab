// Challenge of Lab4
// An implementation of Concurrent Matrix Multiplication
// See C. A. R. Hoare "Communicating Sequential Processes", section 6.2

#include <inc/lib.h>

#define TEMP_ADDR	((char*)0xa00000)

unsigned east(void);
unsigned west(void);
unsigned north(void);
unsigned south(void);
unsigned center(void);

void
umain(int argc, char **argv)
{
    int i, j;
    int id[5][5];

    // fork SOUTH
    i = 4;
    for (j = 1; j != 4; ++j) {
        if ((id[i][j] = fork()) == 0) {
            south();
            return;
        }
    }
    // fork CENTER
    for (i = 1; i != 4; ++i) {
        for (j = 1; j != 4; ++j) {
            if ((id[i][j] = fork()) == 0) {
                center();
                return;
            }   
        }
    }
    // fork NORTH
    i = 0;
    for (j = 1; j != 4; ++j) {
        if ((id[i][j] = fork()) == 0) {
            north();
            return;
        } 
    }
    // fork WEST
    j = 0;
    for (i = 1; i != 4; ++i) {
        if ((id[i][j] = fork()) == 0) {
            west();
            return;
        }     
    }
    // fork EAST
    j = 4;
    for (i = 1; i != 4; ++i) {
        if ((id[i][j] = fork()) == 0) {
            east();
            return;
        }     
    }

    // tell east the sender
    for (i = 1; i != 4; ++i) {
        /* sys_env_set_status(id[4][j], ENV_RUNNING); */
        ipc_send(id[i][4], id[i][3], 0, 0);
    }
    
    // tell south the name of center
    for (j = 1; j != 4; ++j) {
        /* sys_env_set_status(id[4][j], ENV_RUNNING); */
        ipc_send(id[4][j], id[3][j], 0, 0);
    }
    
    // tell north, after this ,north begins to send
    for (j = 1; j != 4; ++j) {
        /* sys_env_set_status(id[4][j], ENV_RUNNING); */
        ipc_send(id[0][j], id[1][j], 0, 0);
    }
    
    // tell west 
    for (i = 1; i != 4; ++i) {
        /* sys_env_set_status(id[4][j], ENV_RUNNING); */
        ipc_send(id[i][0], id[i][1], 0, 0);
    }

    // tell center
    for (i = 3; i >= 1; --i) {
        for (j = 1; j != 3; ++j) {
            /* sys_env_set_status(id[4][j], ENV_RUNNING); */
            // tell them whom to recv from
            ipc_send(id[i][j], id[i-1][j], 0, 0);
            ipc_send(id[i][j], id[i][j-1], 0, 0);
            // tell them whom to send to
            ipc_send(id[i][j], id[i+1][j], 0, 0);
            ipc_send(id[i][j], id[i][j+1], 0, 0);
        }
    }
}

unsigned
west(void) 
{
    /* sys_env_set_status(thisenv->env_id, ENV_NOT_RUNNABLE); */
    /* sys_yield(); */
    int idx, x;
    envid_t to_env, envid;

    to_env = ipc_recv(&envid, 0, 0);
    cprintf("WEST init complete\n");
    
    x = sys_getenvid() % 10;
    int i;
    for (i = 0; i != 5; i++) {
        if (ipc_recv(&envid , 0, 0) == to_env) {
            ipc_send(to_env, x, 0, 0);
            cprintf("WEST: I sent x(%d)\n", x);
        }
    }
    return 0;
}

unsigned
north(void) 
{
    /* sys_env_set_status(thisenv->env_id, ENV_NOT_RUNNABLE); */
    /* sys_yield(); */
    int idx, send_val = 0;
    envid_t to_env, envid;

    to_env = ipc_recv(&envid, 0, 0);
    cprintf("NORTH init complete\n");
    while(1) {
        if (ipc_recv(&envid, 0, 0) == to_env) {
            ipc_send(to_env, send_val, 0, 0);
            cprintf("NORTH: I sent 0(%d)\n", send_val);
        }
    }
}

unsigned
center(void)
{
    /* sys_env_set_status(thisenv->env_id, ENV_NOT_RUNNABLE); */
    /* sys_yield(); */
    int x, sum, idx;
    envid_t down, right, up, left, envid;
    up = ipc_recv(&envid, 0, 0);
    left = ipc_recv(&envid, 0, 0);
    down = ipc_recv(&envid, 0, 0);
    right = ipc_recv(&envid, 0, 0);
    cprintf("CENTER init complete\n");


    ipc_send(left, sys_getenvid(), 0, 0); // ack west
    while(1) {
        x = ipc_recv(&envid, 0, 0); 
        assert(envid == left);
        cprintf("CENTER: I got x(%d)\n", x);
        ipc_send(right, x, 0, 0);
        ipc_send(up, sys_getenvid(), 0, 0); // ack north

        sum = ipc_recv(&envid, 0, 0);
        assert(envid == up);
        cprintf("CENTER: I got sum(%d)\n", sum);
        int a_ij = sys_getenvid() % 10;
        a_ij = 7;
        ipc_send(down, a_ij * x + sum, 0, 0);
        ipc_send(left, sys_getenvid(), 0, 0); // ack west
    }
}

unsigned
south(void)
{
    /* sys_env_set_status(thisenv->env_id, ENV_NOT_RUNNABLE); */
    /* sys_yield(); */
    int sum;
    envid_t envid, from;
    from = ipc_recv(&envid, 0, 0);
    cprintf("SOUTH init complete\n");
    while(1) {
        sum = ipc_recv(&envid, 0, 0);
        cprintf("[%x]Sum: %d\n", sys_getenvid(), sum);
        // assert(envid == from)
    }
}

unsigned
east(void) 
{
    /* sys_env_set_status(thisenv->env_id, ENV_NOT_RUNNABLE); */
    /* sys_yield(); */
    int x;
    envid_t envid, from;
    from = ipc_recv(&envid, 0, 0);
    cprintf("EAST init complete\n");
    while(1) {
        sys_yield();
        x = ipc_recv(&envid, 0, 0);
        cprintf("EASE: I got x(%d)\n", x);
        // assert(envid == from)
    }
}
