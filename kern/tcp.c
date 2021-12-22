#include <kern/ip.h>
#include <inc/string.h>
#include <kern/inet.h>
#include <inc/error.h>
#include <inc/stdio.h>
#include <kern/tcp.h>


struct tcp_virtual_channel[TCP_VC_NUM] tcp_vc;

void tcp_init_vc() {
    for (int i = 0; i < TCP_VC_NUM; i++) {
        tcp_vc[i].state = LISTEN;
        tcp_vc[i].host_side.ip = MY_IP;
        tcp_vc[i].host_side.port = i + 1;
        // tcp connection from host OS is expected
        tcp_vc[i].guest_side.ip = HOST_IP;
        tcp_vc[i].guest_side.port = 8080;
    }
}

int tcp_send(struct tcp_virtual_channel* channel, struct tcp_pkt* pkt, size_t length) {
    struct ip_pkt result;
    struct ip_hdr* hdr = &result.hdr;

    hdr->ip_protocol = IP_PROTO_TCP;
    hdr->ip_source_address = channel->my_side.ip;
    hdr->ip_destination_address = channel->client_side.ip;

    size_t data_length = TCP_HEADER_LEN + length;
    memcpy((void*)result.data, (void*)pkt, data_length);

    return ip_send(&result, data_length);
}

int tcp_recv(struct ip_pkt* pkt) {
    if (pkt->hdr.ip_total_length - IP_HEADER_LEN < TCP_HEADER_LEN) {
        cprintf("IP packet too short for TCP header\n");
        return -1;
    }
    struct tcp_pkt tcp_pkt;
    memcpy((void *)&tcp_pkt, (void *)pkt->data, pkt->hdr.ip_total_length - IP_HEADER_LEN);
    tcp_process(&tcp_pkt, pkt->ip_source_address);
}

struct tcp_virtual_channel * match_tcp_vc(struct tcp_pkt *pkt) {
    for (int i = 0; i < TCP_VC_NUM; i++) {
        if (tcp_vc[i].host_side.port == pkt->hdr.dst_port) {
            return &tcp_vc[i];
        }
    }
    return NULL;
}

int match_listen_ip(struct tcp_virtual_channel *vc, uint32_t src_ip) {
    // always match
    return 1;
}

int tcp_process(struct tcp_pkt *pkt, uint32_t src_ip) {
    struct tcp_virtual_channel * vc = match_tcp_vc(pkt);
    if (vc == NULL) {
        cprintf("No TCP VC match for packet\n");
        goto error;
    }
    switch(vc->state) {
    case CLOSED:
        // not implemented
        break;
    case LISTEN:
        if (pkt->hdr.syn) {
            if (match_listen_ip(vc, src_ip)) {
                // trivial seq num
                vc->ack_seq.seq_num = pkt->hdr.seq_num;
                vc->guest_side.ip = src_ip;
                vc->guest_side.port = pkt->hdr.src_port;
                // TODO: send with ack = pkt->hdr.seq_num + 1
                // tcp_send_ack();
            } else {
                cprintf("Source IP");
                num2ip(src_ip);
                cprintf("didn't match listen IP");
                num2ip(vc->guest_side.ip);
                cprintf("\n");
                goto error;
            }
        } else {
            cprintf("SYN flag is not provided\n");
            goto error;
        }
        break;
    case SYN_SENT:
        break;
    case SYN_RECEIVED:
        break;
    case ESTABLISHED:
        break;
    case FIN_WAIT_1:
        break;
    case CLOSING:
        break;
    case FIN_WAIT_2:
        break;
    case TIME_WAIT:
        break;
    case CLOSE_WAIT:
        break;
    case LAST_ACK:
        break;
    default:
        cprintf("Impossible state - %d\n", vc->state);
        break;
    }

    return 0;

error:
    return -1;
}
