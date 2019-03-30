#include <kern/e1000.h>
#include <inc/types.h>

#include <kern/pci.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

volatile uint32_t *e1000_bar0;

int 
pci_e1000_attach(struct pci_func *f)
{
    pci_func_enable(f);
    uint32_t bar0_pa = f->reg_base[0], bar0_size = f->reg_size[0];
    e1000_bar0 = mmio_map_region(bar0_pa, bar0_size);
    assert(e1000_bar0[E1000_STATUS] == 0x80080784); // full duplex link 1000 MB/s
    return 1;
}
