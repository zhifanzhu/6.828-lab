#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
    int r;
    void **rcvpg_list = (void *)0x0F000000;
    void *buf = (void *)REQVA;
    if ((r = sys_page_alloc(0, rcvpg_list, PTE_P|PTE_U|PTE_W)) < 0)
        panic("sys_page_alloc: %e", r);
    if ((r = sys_page_alloc(0, buf, PTE_P|PTE_U|PTE_W)) < 0)
        panic("sys_page_alloc: %e", r);

    rcvpg_list[0] = buf;
    if ((r = sys_e1000_receive(rcvpg_list, 1)) < 0)
        panic("net input: %e", r);

    struct jif_pkt *pkt = (struct jif_pkt *)REQVA;
    if ((r = sys_page_alloc(0, pkt, PTE_P|PTE_U|PTE_W)) < 0)
        panic("sys_page_alloc: %e", r);
    pkt->jp_len = PGSIZE - sizeof(pkt->jp_len);
    memcpy(pkt->jp_data, buf, pkt->jp_len);
    ipc_send(ns_envid, NSREQ_INPUT, pkt, PTE_P|PTE_U|PTE_W);
}
