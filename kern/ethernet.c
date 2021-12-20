#include <kern/e1000.h>
#include <kern/ethernet.h>
#include <inc/string.h>
#include <kern/inet.h>
#include <inc/error.h>
#include <inc/assert.h>
#include <kern/arp.h>
#include <kern/ip.h>

int
eth_send(struct eth_hdr* hdr, void* data, size_t len) {
    assert(len <= ETH_MAX_PACKET_SIZE - sizeof(struct eth_hdr));
    memcpy((void*)hdr->eth_source_mac, qemu_mac, sizeof(hdr->eth_source_mac));
    memcpy((void*)hdr->eth_destination_mac, hard_code_destination_mac, sizeof(hdr->eth_destination_mac));

    char buf[ETH_MAX_PACKET_SIZE + 1];

    hdr->eth_type = htons(hdr->eth_type);
    memcpy((void*)buf, (void*)hdr, sizeof(struct eth_hdr));

    memcpy((void*)buf + sizeof(struct eth_hdr), data, len);
    // len - data length
    return e1000_transmit(buf, len + sizeof(struct eth_hdr));
}


int
eth_recv(void* data) {
    char buf[ETH_MAX_PACKET_SIZE + 1];
    struct eth_hdr hdr;

    int size = e1000_receive(buf);
    if (size < 0) return size;

    memcpy((void*)&hdr, (void*)buf, sizeof(struct eth_hdr));
    hdr.eth_type = ntohs(hdr.eth_type);
    memcpy(data, (void*)buf + sizeof(struct eth_hdr), size);
    if (hdr.eth_type == ETH_TYPE_IP) {
        return ip_recv(data);
    } else if (hdr.eth_type == ETH_TYPE_ARP) {
        arp_resolve(data);
    } else {
        return -E_BAD_ETH_TYPE;
    }

    return size;
}
