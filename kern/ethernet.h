#ifndef JOS_KERN_ETH_H
#define JOS_KERN_ETH_H

#include <inc/types.h>
#include <kern/e1000.h>

struct eth_hdr {
    uint8_t eth_destination_mac[6];
    uint8_t eth_source_mac[6];
    uint16_t eth_type;
} __attribute__((packed));

int eth_send(struct eth_hdr* hdr, void* data, size_t len);
int eth_recv(void* data);

//52:54:00:12:34:56
static const char qemu_mac[6] = {0x52, 0x54, 0x0, 0x12, 0x34, 0x56};
//static const char hard_code_destination_mac[6] = {0x3a, 0xbe, 0x6d, 0xa0, 0xaf, 0x00};
static const char hard_code_destination_mac[6] = {0xde, 0x1f, 0x77, 0xb8, 0x9f, 0x91};

#define ETH_MAX_PACKET_SIZE 1500
#define ETH_HEADER_LEN sizeof(struct eth_hdr)
#define ETH_TYPE_IP 0x0800
#define ETH_TYPE_ARP 0x0806

#endif
