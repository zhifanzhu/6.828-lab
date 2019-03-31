#include "ns.h"
#include <kern/e1000.h>

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver

    int val, perm, r;
    envid_t whom;

    struct jif_pkt *pkt = (struct jif_pkt *)REQVA;
    if ((r = sys_page_alloc(0, pkt, PTE_P|PTE_U|PTE_W)) < 0)
        panic("sys_page_alloc: %e", r);

    for (;;) {
        val = ipc_recv(&whom, pkt, &perm);
        if (val == NSREQ_OUTPUT) {
            if (!(perm & (PTE_P|PTE_U|PTE_W))) {
                cprintf("Bad perm: from %08x to output\n", whom);
            }

            while((r = sys_e1000_try_transmit(pkt->jp_data, pkt->jp_len))) {
                if (r != -E_TXD_FULL)
                    panic("net output: %e", r);
                sys_yield();
            }
        }     
    }
}
