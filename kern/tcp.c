#include <kern/ip.h>
#include <inc/string.h>
#include <kern/inet.h>
#include <inc/error.h>
#include <inc/stdio.h>
#include <kern/tcp.h>

struct tcp_virtual_channel tcp_vc[TCP_VC_NUM];

void dump_tcp_hdr(struct tcp_hdr *hdr) {
    /*cprintf("\nTCP header:\n");
    cprintf("\tdata_offset: %x\n", hdr->data_offset & 0xF);
    cprintf("\treserved:    %x\n", hdr->reserved & 0x7);
    cprintf("\tns:          %x\n", hdr->ns & 0x1);
    cprintf("\tflags:       %x\n", hdr->flags);
    cprintf("\n");*/
}

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
    pkt->hdr.seq_num = channel->ack_seq.seq_num;
    pkt->hdr.ack_num = channel->ack_seq.ack_num;
    pkt->hdr.src_port = JHTONS(channel->host_side.port);
    pkt->hdr.dst_port = JHTONS(channel->guest_side.port);
    pkt->hdr.win_size = JHTONS(sizeof(channel->buffer));

    // TODO chesksum

    struct ip_pkt result;
    struct ip_hdr* hdr = &result.hdr;

    hdr->ip_protocol = IP_PROTO_TCP;
    hdr->ip_source_address = JHTONL(channel->host_side.ip);
    hdr->ip_destination_address = JHTONL(channel->guest_side.ip);

    size_t data_length = TCP_HEADER_LEN + length;
    memcpy((void*)result.data, (void*)pkt, data_length);

    dump_tcp_hdr(&(pkt->hdr));

    return ip_send(&result, data_length);
}

struct tcp_virtual_channel * match_tcp_vc(struct tcp_pkt *pkt) {
    for (int i = 0; i < TCP_VC_NUM; i++) {
        cprintf("%u - %u\n", tcp_vc[i].host_side.port, JNTOHS(pkt->hdr.dst_port));
        if (tcp_vc[i].host_side.port == JNTOHS(pkt->hdr.dst_port)) {
            return &tcp_vc[i];
        }
    }
    return NULL;
}

int match_listen_ip(struct tcp_virtual_channel *vc, uint32_t src_ip) {
    // always match
    return 1;
}

int tcp_send_ack(struct tcp_virtual_channel *vc, uint8_t flags) {
    struct tcp_pkt ack_pkt = {};
    ack_pkt.hdr.DORNS |= ((uint8_t)(TCP_HEADER_LEN >> 2) & 0xF) << 4;
    ack_pkt.hdr.flags = flags | TH_ACK;
    int r = tcp_send(vc, &ack_pkt, 0);

    if (r == -1) {
        cprintf("tcp send error\n");
    }

    return r;
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
        if (pkt->hdr.flags & TH_SYN) {
            if (match_listen_ip(vc, src_ip)) {
                // trivial seq num
                vc->ack_seq.seq_num = JNTOHL(pkt->hdr.seq_num);
                vc->guest_side.ip = src_ip;
                vc->guest_side.port = JNTOHS(pkt->hdr.src_port);
                vc->ack_seq.ack_num = JNTOHL(pkt->hdr.seq_num) + 1;
                tcp_send_ack(vc, TH_SYN);

                vc->state = SYN_RECEIVED;
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
        if (pkt->hdr.flags & TH_ACK) {
            if (src_ip != vc->guest_side.ip) {
                cprintf("Wrong IP -");
                num2ip(src_ip);
                cprintf(" is not");
                num2ip(vc->guest_side.ip);
                cprintf("\n");
                goto error;
            }
            if (JNTOHL(pkt->hdr.seq_num) != vc->ack_seq.ack_num ||
                JNTOHL(pkt->hdr.ack_num) != vc->ack_seq.seq_num + 1) {
                cprintf("Wrond ack seq\n");
                goto error;
            }
            vc->state = ESTABLISHED;
        } else {
            cprintf("ACK flag is not provided\n");
            goto error;
        }
        break;
    case ESTABLISHED:
        // TODO Established
        cprintf("ESTABLISHED\n");
        while (1) {}
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

int tcp_recv(struct ip_pkt* pkt) {
    if (JNTOHS(pkt->hdr.ip_total_length) - IP_HEADER_LEN < TCP_HEADER_LEN) {
        cprintf("IP packet too short for TCP header\n");
        return -1;
    }
    struct tcp_pkt tcp_pkt;
    memcpy((void *)&tcp_pkt, (void *)pkt->data, JNTOHS(pkt->hdr.ip_total_length) - IP_HEADER_LEN);
    return tcp_process(&tcp_pkt, JNTOHL(pkt->hdr.ip_source_address));
}
