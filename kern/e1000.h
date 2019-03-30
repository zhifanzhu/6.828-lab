#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#define PCI_VENDOR_DEFAULT          0x8086
#define PCI_DEVICE_82540EM_A        0x100e

struct pci_func;
int pci_82540em_attach(struct pci_func *);

#endif	// JOS_KERN_E1000_H
