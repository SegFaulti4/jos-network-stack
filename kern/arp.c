#include <inc/stdio.h>
#include <inc/string.h>
#include <kern/arp.h>
#include <kern/ethernet.h>
#include <inc/error.h>
#include <kern/inet.h>

void arp_reply(struct arp_hdr *arp_header) {
    arp_header->opcode = ARP_REPLY;
    memcpy(arp_header->target_mac, arp_header->source_mac, 6);
    arp_header->target_ip = arp_header->source_ip;
    memcpy(arp_header->source_mac, qemu_mac, 6);
    arp_header->source_ip = JHTONL(MY_IP);

    arp_header->opcode = htons(arp_header->opcode);
    arp_header->hardware_type = htons(arp_header->hardware_type);
    arp_header->protocol_type = htons(arp_header->protocol_type);

    struct eth_hdr reply_header;
    memcpy(reply_header.eth_destination_mac, arp_header->target_mac, 6);
    reply_header.eth_type = JHTONS(ETH_TYPE_ARP);
    int status = eth_send(&reply_header, arp_header, sizeof(struct arp_hdr));
    if (status < 0) {
        cprintf("Error attempting arp response.");
    }
}


void arp_resolve(void* data) {
    struct arp_hdr *arp_header;

    arp_header = (struct arp_hdr *)data;

    arp_header->hardware_type = ntohs(arp_header->hardware_type);
    arp_header->protocol_type = ntohs(arp_header->protocol_type);
    arp_header->opcode = ntohs(arp_header->opcode);

    if (arp_header->hardware_type != ARP_ETHERNET) {
        cprintf("Error! Only ethernet is supporting.");
        return;
    }
    if (arp_header->protocol_type != ARP_IPV4) {
        cprintf("Error! Only IPv4 is supported.");
        return;
    }
    if (arp_header->target_ip != MY_IP) {
        cprintf("This is not for me!");
        return;
    }
    if (arp_header->opcode != ARP_REQUEST) {
        cprintf("Error! Only arp requests are supported");
        return;
    }

    arp_reply(arp_header);
}
