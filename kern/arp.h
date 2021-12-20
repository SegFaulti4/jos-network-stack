#ifndef JOS_KERN_ARP_H
#define JOS_KERN_ARP_H

#include <inc/types.h>
#include <kern/ip.h>
#define MY_IP ip2num((int8_t [4]){192, 168, 123, 2})
//0xA000004

#define ARP_ETHERNET 0x0001
#define ARP_IPV4     0x0800
#define ARP_REQUEST  0x0001
#define ARP_REPLY    0x0002

#define ARP_TABLE_MAX_SIZE 32

struct arp_hdr {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_address_length;
    uint8_t protocol_address_length;
    uint16_t opcode;
    uint8_t source_mac[6];
    uint32_t source_ip;
    uint8_t target_mac[6];
    uint32_t target_ip;
} __attribute__((packed));

void arp_resolve(void* data);
void arp_reply(struct arp_hdr *arp_header);

#endif
