#include <kern/ip.h>
#include <inc/string.h>
#include <kern/inet.h>
#include <inc/error.h>
#include <inc/stdio.h>
#include <kern/tcp.h>

int
tcp_send(struct tcp_virtual_channel* channel ,struct tcp_pkt* pkt, size_t length) {
    struct ip_pkt result;
    struct ip_hdr* hdr = &result.hdr;

    hdr->ip_protocol = IP_PROTO_TCP;
    hdr->ip_source_address = channel->my_side.ip;
    hdr->ip_destination_address = channel->client_side.ip;

    size_t data_length = TCP_HEADER_LEN + length;
    memcpy((void*)result.data, (void*)pkt, data_length);

    return ip_send(&result, data_length);
}

int
tcp_recv(struct ip_pkt* pkt) {
    // some code here
}
