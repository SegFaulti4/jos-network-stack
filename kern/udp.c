#include <kern/udp.h>
#include <kern/inet.h>
#include <inc/string.h>
#include <inc/stdio.h>

int
udp_send(void* data, int length) {
    struct udp_pkt pkt;
    struct udp_hdr* hdr = &pkt.hdr;
    hdr->source_port = JHTONS(8081);
    hdr->destination_port = JHTONS(1234);
    hdr->length = JHTONS(length + sizeof(struct udp_hdr));
    hdr->checksum = 0;
    memcpy((void*)pkt.data, data, length);
    struct ip_pkt result;
    result.hdr.ip_protocol = IP_PROTO_UDP;
    int8_t src_ip[] = {192, 168, 123, 2};
    int8_t dst_ip[] = {192, 168, 123, 1};
    result.hdr.ip_source_address = JHTONL(ip2num(src_ip));
    result.hdr.ip_destination_address = JHTONL(ip2num(dst_ip));
    memcpy((void*)result.data, (void*)&pkt, length + sizeof(struct udp_hdr));
    return ip_send(&result, length + sizeof(struct udp_hdr));
}

int
udp_recv(struct ip_pkt* pkt) {
    cprintf("Processing UDP\n");
    struct udp_pkt upkt;
    int size = JNTOHS(pkt->hdr.ip_total_length - IP_HEADER_LEN);
    memcpy((void*)&upkt, (void*)pkt->data, size);
    struct udp_hdr* hdr = &upkt.hdr;
    cprintf("port: %d\n", hdr->destination_port);
    for (size_t i = 0; i < hdr->length; i++) {
        cprintf("%c", upkt.data[i]);
    }
    cprintf("\n");
    return 0;
}
