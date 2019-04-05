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
    int i, r;
    struct jif_pkt *pkt = (struct jif_pkt *)REQVA;
    /* for (;;) { */
        if ((r = sys_page_alloc(0, pkt, PTE_P|PTE_U|PTE_W)) < 0)
            panic("sys_page_alloc: %e", r);

        pkt->jp_len = PGSIZE - sizeof(pkt->jp_len);
        while ((r = sys_e1000_receive(pkt->jp_data, pkt->jp_len)) < 0) {
            sys_yield();
        }
            /* panic("net input: %e", r); */

        nsipcbuf.pkt = *pkt;
        ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P|PTE_U|PTE_W);
        sys_yield();
    /* } */
        if ((r = sys_e1000_receive(pkt->jp_data, pkt->jp_len)) < 0)
            panic("net input: %e", r);

        nsipcbuf.pkt = *pkt;
        ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P|PTE_U|PTE_W);
        sys_yield();
}
