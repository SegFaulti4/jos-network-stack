#include <kern/e1000.h>
#include <kern/pci.h>
#include <kern/pmap.h>
#include <inc/types.h>
#include <inc/string.h>
#include <inc/error.h>

// Base mmio address
volatile uint32_t *phy_mmio_addr;
#define E1000_REG(offset) (phy_mmio_addr[offset >> 2])

struct tx_desc tx_desc_table[E1000_NU_DESC];  // Literally
struct rx_desc rx_desc_table[E1000_NU_DESC];  // Literally
char tx_buf[E1000_NU_DESC][E1000_BUFFER_SIZE];      // TX buffers
char rx_buf[E1000_NU_DESC][E1000_BUFFER_SIZE];      // RX buffers

int e1000_attach(struct pci_func * pciFunction) {
    // Get phy_mmio address and size
    pci_get_bar_info(pciFunction);

    // Map phy_mmio
    phy_mmio_addr = mmio_map_region(pciFunction->reg_base[0], pciFunction->reg_size[0]);

    cprintf("Mapping E1000 TX area at %16lx.\n", PADDR(tx_desc_table));
    // Set TX Base Address Low
    E1000_REG(E1000_TDBAL) = (uint32_t)((uint64_t)PADDR(tx_desc_table) & 0xFFFFFFFF);

    // Set TX Base Address High
    E1000_REG(E1000_TDBAH) = (uint32_t)((uint64_t)PADDR(tx_desc_table) >> 32);

    // Set TX Length
    E1000_REG(E1000_TDLEN) = sizeof(tx_desc_table);

    // Set TX Head
    E1000_REG(E1000_TDH) = 0;

    // Set TX Tail
    E1000_REG(E1000_TDT) = 0;

    // Set TX Control Register
    //E1000_REG(E1000_TCTL) = E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT | E1000_TCTL_COLD;
    E1000_REG(E1000_TCTL) = E1000_TCTL_EN | E1000_TCTL_PSP | (15 << 4) | (64 << 12);
    //E1000_REG(E1000_TCTL) = 0b0110000000000111111000011111010;

    // Set TX Inter-packet Gap
    E1000_REG(E1000_TIPG) = 0x60200a;

    // Set TX Descriptors
    for (int i = 0; i < E1000_NU_DESC; i++) {
        // Set TX status as Descriptor Done
        tx_desc_table[i].status |= E1000_TXD_STAT_DD;
        // Set TX buffer address
        tx_desc_table[i].buf_addr = PADDR(tx_buf[i]);
    }

    // Set RX Address Low and High as
    // MAC address 52:54:00:12:34:56
    E1000_REG(E1000_RAL) = 0x12005452;
    *(uint16_t *)&E1000_REG(E1000_RAH) = 0x5634;

    // Set Multicast Table Array
    E1000_REG(E1000_MTA) = 0;

    // Set RX Base Address Low
    E1000_REG(E1000_RDBAL) = PADDR(rx_desc_table);

    // Set RX Base Address High
    E1000_REG(E1000_RDBAH) = 0;

    // Set RX Length
    E1000_REG(E1000_RDLEN) = E1000_NU_DESC;

    // Set RX Head
    E1000_REG(E1000_RDH) = 0;

    // Set RX Tail
    E1000_REG(E1000_RDT) = E1000_NU_DESC - 1;

    // Set RX Control Register
    E1000_REG(E1000_RCTL) = E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_CRC;

    // Set RX Descriptors
    for (int i = 0; i < E1000_NU_DESC; i++) {
        // Clear RX status Descriptor Done
        rx_desc_table[i].status &= ~E1000_RXD_STAT_DD;
        // Set RX buffer address
        rx_desc_table[i].buf_addr = PADDR(rx_buf[i]);
    }

    return 1;
}


int e1000_transmit(const char* buf, unsigned len) {
    // Trunk packet length
    len = len > E1000_BUFFER_SIZE ? E1000_BUFFER_SIZE : len;

    // Tail TX Descriptor Index
    uint32_t tail_tx = E1000_REG(E1000_TDT);

    // Check status of tail TX Descriptor
    if (!(tx_desc_table[tail_tx].status & E1000_TXD_STAT_DD)) {
        cprintf("E1000 transmit queue is full\n");
        return -1;
    }

    // Clear TX status Descriptor Done
    tx_desc_table[tail_tx].status &= ~E1000_TXD_STAT_DD;

    // Move data to buffer
    memmove(tx_buf[tail_tx], buf, len);

    // Set packet length
    tx_desc_table[tail_tx].length = len;

    // Set TX cmd as Report Status and End of Packet
    //tx_desc_table[tail_tx].cmd |= (E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP);
    tx_desc_table[tail_tx].cmd = ((1 << 1) | (1 << 0) | (1 << 3));
    tx_desc_table[tail_tx].status = 0;

    // Point to next TX Descriptor
    uint32_t old_tail = tail_tx;
    tail_tx = (tail_tx + 1) % E1000_NU_DESC;
    E1000_REG(E1000_TDT) = tail_tx;
    while(!(tx_desc_table[old_tail].status & 0xff) && !(tx_desc_table[old_tail].status & E1000_TXD_STAT_DD)) {}

    return 0;
}

int e1000_receive(char* buffer) {
    // Tail RX Descriptor Index
    uint32_t tail_rx = E1000_REG(E1000_RDT);
    tail_rx = (tail_rx + 1) % E1000_NU_DESC;

    // Check status of tail RX Descriptor
    if (!(rx_desc_table[tail_rx].status & E1000_RXD_STAT_DD)) {
        cprintf("E1000 receive queue is empty\n");
        return -1;
    }

    // Clear RX status Descriptor Done
    rx_desc_table[tail_rx].status &= ~E1000_RXD_STAT_DD;

    // Get packet length
    int len = rx_desc_table[tail_rx].length;

    // Get data from buffer
    memmove(buffer, rx_buf[tail_rx], len);

    // Point to next RX Descriptor
    E1000_REG(E1000_RDT) = tail_rx;

    return len;
}
