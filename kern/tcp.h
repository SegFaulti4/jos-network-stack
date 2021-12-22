#ifndef JOS_KERN_TCP_H
#define JOS_KERN_TCP_H

#include <inc/types.h>
#include <kern/ip.h>

struct tcp_hdr {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset : 4;
    uint8_t reserved : 3;
    uint8_t ns : 1,
            cwr : 1,
            ece : 1,
            urg : 1,
            ack : 1,
            psh : 1,
            rst : 1,
            syn : 1,
            fin : 1;
    uint16_t win_size;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed));

#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20

#define TCP_HEADER_LEN sizeof(struct tcp_hdr)
#define TCP_DATA_LEN (IP_DATA_LEN - TCP_HEADER_LEN)

struct tcp_pkt {
    struct tcp_hdr hdr;
    uint8_t data[TCP_DATA_LEN];
} __attribute__((packed));

enum tcp_state {
    CLOSED,
    LISTEN,
    SYN_SENT,
    SYN_RECEIVED,
    ESTABLISHED,
    FIN_WAIT_1,
    CLOSING,
    FIN_WAIT_2,
    TIME_WAIT,
    CLOSE_WAIT,
    LAST_ACK
};

struct tcp_endpoint {
    uint32_t ip;
    uint16_t port;
};

struct tcp_ack_seq {
    uint32_t seq_num;
    uint32_t ack_num;
};

struct tcp_virtual_channel {
    enum tcp_state state;
    struct tcp_endpoint host_side;
    struct tcp_endpoint guest_side;
    struct tcp_ack_seq ack_seq;
    uint8_t buffer[TCP_DATA_LEN * 10];
    uint32_t data_len;
};

#define TCP_VC_NUM 64

int tcp_send(struct tcp_virtual_channel* channel, struct tcp_pkt* pkt, size_t length);
int tcp_recv(struct ip_pkt* pkt);

#endif