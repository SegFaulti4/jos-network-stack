#include <kern/ip.h>
#include <inc/string.h>
#include <kern/inet.h>
#include <inc/error.h>
#include <kern/ethernet.h>
#include <kern/icmp.h>
#include <inc/stdio.h>

int 
icmp_echo_reply(struct ip_pkt* pkt) {
    struct icmp_pkt icmp_packet;
    int size = JNTOHS(pkt->hdr.ip_total_length - IP_HEADER_LEN);
    memcpy((void*)&icmp_packet, (void*)pkt->data, size);
    struct icmp_hdr* hdr = &icmp_packet.hdr;
    if (hdr->msg_type != ECHO_REQUEST)
        return -E_UNS_ICMP_TYPE;

    if (hdr->msg_code != 0)
        return -E_INV_ICMP_CODE;

    hdr->msg_type = ECHO_REPLY;
    hdr->sequence_number = JNTOHS(hdr->sequence_number);
    hdr->sequence_number += 1;
    hdr->sequence_number = JHTONS(hdr->sequence_number);
    
    struct ip_pkt result;
    result.hdr.ip_protocol = IP_PROTO_ICMP;
    memcpy((void*)result.data, (void*)&icmp_packet, size);
    return ip_send(&result, size);
    return 0;
}