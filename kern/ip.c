#include <kern/ip.h>
#include <inc/string.h>
#include <kern/inet.h>
#include <inc/error.h>
#include <kern/ethernet.h>
#include <kern/icmp.h>
#include <inc/stdio.h>
#include <kern/udp.h>

uint16_t packet_id = 0;

uint16_t
ip_checksum(void* vdata, size_t length) {
    char* data = vdata;
    uint32_t acc = 0xffff;
    for (size_t i = 0; i + 1 < length; i += 2) {
        uint16_t word;
        memcpy(&word, data + i, 2);
        acc += ntohs(word);
        if (acc > 0xffff) {
            acc -= 0xffff;
        }
    }
    // Handle any partial block at the end of the data.
    if (length & 1) {
        uint16_t word = 0;
        memcpy(&word, data + length - 1, 1);
        acc += ntohs(word);
        if (acc > 0xffff) {
            acc -= 0xffff;
        }
    }
    // Return the checksum in network byte order.
    return htons(~acc);
}


int
ip_send(struct ip_pkt* pkt, uint16_t length) {
    uint16_t id = packet_id++;

    struct ip_hdr* hdr = &pkt->hdr;
    hdr->ip_verlen = IP_VER_LEN;
    hdr->ip_tos = 0;
    hdr->ip_total_length = JHTONS(length + IP_HEADER_LEN);
    hdr->ip_id = id;
    hdr->ip_flags_offset = 0;
    hdr->ip_ttl = IP_TTL;
    hdr->ip_header_checksum = ip_checksum((void*)pkt, IP_HEADER_LEN);
    struct eth_hdr e_hdr;
    e_hdr.eth_type = ETH_TYPE_IP;
    // length - data length
    return eth_send(&e_hdr, (void*)pkt, sizeof(struct ip_hdr) + length);
}

int
ip_recv(struct ip_pkt* pkt) {
    int res = eth_recv((void*)pkt);
    if (res < 0) {
        return res;
    }
    struct ip_hdr* hdr = &pkt->hdr;
    if (hdr->ip_verlen != IP_VER_LEN) {
        return -E_UNS_VER;
    }
    uint16_t checksum = hdr->ip_header_checksum;
    hdr->ip_header_checksum = 0;
    if (checksum != ip_checksum((void*)pkt, IP_HEADER_LEN)) {
        return -E_INV_CHS;
    }
    if (hdr->ip_protocol == IP_PROTO_TCP) {
        // some tcp reciever
    } else  if (hdr->ip_protocol == IP_PROTO_UDP) {
        return udp_recv(pkt);
    } else if (hdr->ip_protocol == IP_PROTO_ICMP) {
        return icmp_echo_reply(pkt);
    }

    return 0;
}