#include <kern/e1000.h>
#include <kern/pci.h>
#include <kern/pmap.h>
#include <inc/types.h>
#include <inc/string.h>
#include <inc/error.h>

#define E1000_REG(offset) (phy_mmio_addr[offset >> 2])

void dump_tx_desc(uint32_t tx_idx) {
    cprintf("\nTX Desc %u:\n", tx_idx);
    cprintf("\tbuf_addr: %lu\n", tx_desc_table[tx_idx].buf_addr);
    cprintf("\tlength:   %u\n", tx_desc_table[tx_idx].length);
    cprintf("\tcso:      %02x\n", tx_desc_table[tx_idx].cso);
    cprintf("\tcmd:      %02x\n", tx_desc_table[tx_idx].cmd);
    cprintf("\tstatus:   %02x\n", tx_desc_table[tx_idx].status);
    cprintf("\tcss:      %02x\n", tx_desc_table[tx_idx].css);
    cprintf("\tspecial:  %04x\n", tx_desc_table[tx_idx].special);
    cprintf("\n");
}

void dump_rx_desc(uint32_t rx_idx) {
    cprintf("\nRX Desc %u:\n", rx_idx);
    cprintf("\tbuf_addr: %lu\n", rx_desc_table[rx_idx].buf_addr);
    cprintf("\tlength:   %u\n", rx_desc_table[rx_idx].length);
    cprintf("\tchecksum: %04x\n", rx_desc_table[rx_idx].checksum);
    cprintf("\tstatus:   %02x\n", rx_desc_table[rx_idx].status);
    cprintf("\terrors:   %02x\n", rx_desc_table[rx_idx].errors);
    cprintf("\tspecial:  %04x\n", rx_desc_table[rx_idx].special);
    cprintf("\n");
}

void e1000_transmit_init() {
    // Set TX Base Address Low
    E1000_REG(E1000_TDBAL) = (uint32_t)PADDR(tx_desc_table);

    // Set TX Base Address High
    E1000_REG(E1000_TDBAH) = 0;

    // Set TX Length
    E1000_REG(E1000_TDLEN) = sizeof(tx_desc_table);

    // Set TX Head
    E1000_REG(E1000_TDH) = 0;

    // Set TX Tail
    E1000_REG(E1000_TDT) = 0;

    // Set TX Control Register
    E1000_REG(E1000_TCTL) = E1000_TCTL_EN |
                            E1000_TCTL_PSP |
                            (E1000_TCTL_CT & (0x10 << 4)) |
                            (E1000_TCTL_COLD & (0x40 << 12));

    // Set TX Inter-packet Gap
    E1000_REG(E1000_TIPG) = 0x60200a;

    // Set TX Descriptors
    for (int i = 0; i < E1000_NU_DESC; i++) {
        // Set TX status as Descriptor Done
        tx_desc_table[i].status |= E1000_TXD_STAT_DD;
        // Set TX buffer address
        tx_desc_table[i].buf_addr = PADDR(tx_buf[i]);
        // Set TX cmd
        tx_desc_table[i].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
    }

    dump_tx_desc(0);
}

void e1000_receive_init() {
    // Set RX Base Address Low
    E1000_REG(E1000_RDBAL) = PADDR(rx_desc_table);

    // Set RX Base Address High
    E1000_REG(E1000_RDBAH) = 0;

    // Set RX Length
    E1000_REG(E1000_RDLEN) = sizeof(rx_desc_table);

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

    dump_rx_desc(0);
}

int e1000_attach(struct pci_func * pciFunction) {
    // Get phy_mmio address and size
    pci_get_bar_info(pciFunction);

    // Map phy_mmio
    phy_mmio_addr = mmio_map_region(pciFunction->reg_base[0], pciFunction->reg_size[0]);

    // Set RX Address Low and High as
    // MAC address 52:54:00:12:34:56
    E1000_REG(E1000_RAL) = 0x12005452;
    *(uint16_t *)&E1000_REG(E1000_RAH) = 0x5634;

    // Set Multicast Table Array
    E1000_REG(E1000_MTA) = 0;

    e1000_transmit_init();
    e1000_receive_init();

    cprintf("E1000 status: %08x\n", E1000_REG(E1000_DEVICE_STATUS));

    return 1;
}


int e1000_transmit(const char* buf, uint16_t len) {
    // Trunk packet length
    len = len > E1000_BUFFER_SIZE ? E1000_BUFFER_SIZE : len;

    // Tail TX Descriptor Index
    uint32_t tail_tx = E1000_REG(E1000_TDT);

    dump_tx_desc(tail_tx);

    // Check status of tail TX Descriptor
    if (!(tx_desc_table[tail_tx].status & E1000_TXD_STAT_DD)) {
        cprintf("E1000 transmit queue is full\n");
        return -1;
    }

    // Move data to buffer
    memmove(tx_buf[tail_tx], buf, len);

    // Set packet length
    tx_desc_table[tail_tx].length = len;

    // Clear TX status Descriptor Done
    tx_desc_table[tail_tx].status &= ~E1000_TXD_STAT_DD;

    // Set TX cmd as Report Status and End of Packet
    // tx_desc_table[tail_tx].cmd |= (E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP);

    // Point to next TX Descriptor
    E1000_REG(E1000_TDT) = (tail_tx + 1) % E1000_NU_DESC;

    return 0;
}

int e1000_receive(char* buffer) {
    // Tail RX Descriptor Index
    uint32_t tail_rx = E1000_REG(E1000_RDT);
    tail_rx = (tail_rx + 1) % E1000_NU_DESC;

    dump_rx_desc(tail_rx);

    // Check status of tail RX Descriptor
    if (!(rx_desc_table[tail_rx].status & E1000_RXD_STAT_DD)) {
        cprintf("\nE1000 receive queue is empty\n");
        return 0;
    }
    if (!(rx_desc_table[tail_rx].status & E1000_RXD_STAT_EOP)){
        cprintf("E1000 receive status is not EOP\n");
        return 0;
    }

    // Clear RX status Descriptor Done
    rx_desc_table[tail_rx].status &= ~E1000_RXD_STAT_DD;

    // Get packet length
    int len = (int)rx_desc_table[tail_rx].length;

    // Get data from buffer
    memmove(buffer, rx_buf[tail_rx], len);

    // Point to next RX Descriptor
    E1000_REG(E1000_RDT) = tail_rx;

    return len;
}
