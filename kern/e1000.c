#include <kern/e1000.h>
#include <inc/types.h>

#include <kern/pci.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

// TODO add static for all
static volatile uint32_t *e1000_bar0; // E1000 Base Address Register 0
struct e1000_tx_desc txdesc_list[NUM_TX_DESC]; // Transmit Descriptor Ring TODO static
struct e1000_rx_desc rxdesc_list[NUM_RX_DESC]; // Receive Descriptor Ring TODO static
char recv_buf[RXBUF_SIZE];

uint32_t
e1000_read(int index)
{
    return e1000_bar0[index/4];
}

void
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

void 
e1000_rx_init()
{
    // Program the RAL/RAH with desired Ethernet Address.
    // RAL[0]/RAH[0] should always be used to store the Individual Ethernet MAC addr
    // of the Ethernet controller. This can com from the EEPROM.
    e1000_write(E1000_RA, 0x12005452); // RAL[0]
    e1000_write(E1000_RA + 4, E1000_RAH_AV | 0x5634); // RAH[0]

    // Initialize the MTA to 0b.
    e1000_write(E1000_MTA, 0);

    // Program the Interrupt Mask Set/Read(IMS) register to enable any interrupt
    // the software driver wants to be notified of when the event occurs.
    // Suggested bits: RXT, RXO, RXDMT, RXSEQ, and LSC.
    // There's no immediate reason to enable the transmit interrupt
    //
    // If s/w uses the Receive Descriptor Minimum Threshold Interrupt, 
    // the Receive Delay Timer(RDTR) register should be initialized with desire delay time
    // Don't Use interrupt for now.(Do this in Challenge problem)

    // Allocate a region of memory for the receive descriptor list (16-bytes aligned),
    // Program the Receive Descriptor Base Address(RDBAL/RDBAH) with addr of the region
    // RDBAL is used for 32-bit address.
    int tail = 0;
    for (; tail < NUM_RX_DESC; ++tail) {
        rxdesc_list[tail].buffer_addr = (uint32_t)&recv_buf[tail*RXBUF_SIZE];
    }
    e1000_write(E1000_RDBAL, PADDR(&rxdesc_list[0]));
    e1000_write(E1000_RDBAH, 0);

    // Set the RDLEN to the size (in bytes) of the descriptor ring (128-btye) aligned.
    e1000_write(E1000_RDLEN, RXDESC_LEN);

    // RDH/RDT are init to 0b. 
    // Receive buffers of appropriate size should be alloced and pointers to these
    // buffers should be stored in the receive desc ring.
    //
    // S/w ini the RDH and RDT with appropriate head and tail addr.
    // Head should point to first valid receive desc in ring and tail should point
    // to one desc beyond the last valid desc in ring.
    e1000_write(E1000_RDT, 0);  
    e1000_write(E1000_RDH, 1); // Can't set RDH to be equal to RDT

    // Program the RCTL with :
    //  Set RCTL.EN bit to 1b for normal op. However, it's best to leave RCTL.EN = 0b 
    //      until desc ring has been init and s/w is ready to process received packets.
    //  Set RCTL.LPE to 1b when processing packet lenth > standard Ethernet packet
    //      e.g set to 1b when processing Jumbo Frames.
    //  RCTL.LBM should be set to 00b.
    //  Config RCTL.RDMTS to desired value.
    //  Config RCTL.MO to desired value.
    //  Set RCTL.BAM to 1b allowing h/w to accept broadcast packets.
    //  Config RCTL.BSIZE bits to reflect size of receive bufs s/w provides to h/w
    //      also config RCTL.BSEX bit if receive buf needs to be larger than 2048 bytes.
    //  Set the Strip Ethernet CRC (RCTL.SECRC) for h/w to strip CRC prior to DMA-ing
    //      the receive packet to host Memory
    uint32_t rctl_reg = e1000_read(E1000_RCTL);
    assert(rctl_reg == 0);
    // Currently enable Long Packet; mem buf size 4096 bytes(BSEX==1b, SZ_256)
    rctl_reg = SET_RCTL_EN(1)|SET_RCTL_LPE(1)|E1000_RCTL_LBM_NO|E1000_RCTL_RDMTS_HALF 
                |E1000_RCTL_MO_0|SET_RCTL_BAM(1)|E1000_RCTL_SZ_256
                |SET_RCTL_BSEX(1)|SET_RCTL_SECRC(1);
    e1000_write(E1000_RCTL, rctl_reg);

}

int 
pci_e1000_attach(struct pci_func *f)
{
    pci_func_enable(f);
    uint32_t bar0_pa = f->reg_base[0], bar0_size = f->reg_size[0];
    e1000_bar0 = mmio_map_region(bar0_pa, bar0_size);
    assert(e1000_read(E1000_STATUS) == 0x80080783); // full duplex link 1000 MB/s
    e1000_rx_init();
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

int e1000_receive()
{
    return 0;
}
