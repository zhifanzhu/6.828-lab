#include <kern/e1000.h>

// LAB 6: Your driver code here
#include <kern/pci.h>

int 
pci_82540em_attach(struct pci_func *f)
{
    pci_func_enable(f);
    return 1;
}
