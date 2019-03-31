#include <kern/e1000.h>
#include <inc/types.h>

#include <kern/pci.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

volatile uint32_t *e1000_bar0; // E1000 Base Address Register 0
struct e1000_tx_desc txdesc_list[NUM_TX_DESC]; // Transmit Descriptor Ring

static uint32_t
e1000_read(int index)
{
    return e1000_bar0[index/4];
}

static void
e1000_write(int index, int value)
{
    e1000_bar0[index/4] = value;
}

static void 
e1000_tx_init() 
{
    // Allocate a region of mem for tx desc list
    // mem should aligned on 16-byte bound
    // Program TBDAL/TDBAH with addr of the region
    e1000_write(E1000_TDBAL, PADDR(&txdesc_list[0]));
    e1000_write(E1000_TDBAH, 0);

    // Set (TDLEN) to size of desc ring, must be 128-byte aligned
    e1000_write(E1000_TDLEN, TXDESC_LEN);

    // TDH/TDT are init by h/w to 0b, s/w should write 0b to both to ensure this
    e1000_write(E1000_TDH, 0);
    e1000_write(E1000_TDT, 0);
    
    // Init Transmit Control Register(TCTL), include:
    //  Set TCTL.EN bit to 1b for normal operation
    //  Set Pad Short Packets (TCTL.PSP) to 1b
    //  Configure Collison Thresh (TCTL.CT) to desired value. Ethernet standard is 10h,
    //      this setting only has meaning in half dulplex mode
    //  Configure Collison Distance (TCTL.COLD) to expected value.
    //      Assume full duplex ops, set to 40h. 
    uint32_t tctl_reg = e1000_read(E1000_TCTL);
    assert(tctl_reg == 0); // Assuming tctl_reg empty
    tctl_reg = SET_TCTL_EN(1)|SET_TCTL_PSP(1)|SET_TCTL_CT(0x10)|SET_TCTL_COLD(0x40);
    e1000_write(E1000_TCTL, tctl_reg);
    
    // Program Transmit IPG (TIPG) to get minimum legal Inter Packet Gap
    // See Table 13-77 in Manual 13.4.34
    e1000_write(E1000_TIPG, (6<<20)|(8<<10)|10);
}

int 
pci_e1000_attach(struct pci_func *f)
{
    pci_func_enable(f);
    uint32_t bar0_pa = f->reg_base[0], bar0_size = f->reg_size[0];
    e1000_bar0 = mmio_map_region(bar0_pa, bar0_size);
    assert(e1000_read(E1000_STATUS) == 0x80080783); // full duplex link 1000 MB/s
    e1000_tx_init();
    return 1;
}

int
e1000_transmit(physaddr_t addr, uint16_t length) 
{
    if (length > TXDATA_MAX_LEN)
        return -E_TXD_LEN_OV;

    size_t tail = e1000_read(E1000_TDT);
    // Read the DD bit of TAIL Register
    if (!(txdesc_list[tail].lower.data & E1000_TXD_CMD_RS)
            || (txdesc_list[tail].lower.data & E1000_TXD_CMD_RS
                && txdesc_list[tail].upper.data & E1000_TXD_STAT_DD)
            ) {
        // Descriptor Done, we are safe
        txdesc_list[tail].buffer_addr = addr;
        txdesc_list[tail].lower.flags.length = length; // length must > 48 ?
        txdesc_list[tail].lower.data |= E1000_TXD_CMD_RS|E1000_TXD_CMD_EOP;
        tail = (++tail == NUM_TX_DESC) ? 0 : tail;
        e1000_write(E1000_TDT, tail);
        return 0;
    } else {
        // DD not set, tx queue is full
        return -E_TXD_FULL;
    }
}
