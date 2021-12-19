#ifndef JOS_KERN_TCP_H
#define JOS_KERN_TCP_H

#include <inc/types.h>
#include <kern/ip.h>

struct tcp_hdr {
    uint16_t source_port;
    uint16_t destination_port;
    uint32_t sequence_number;
    uint32_t acknowledgement_number;
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
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} __attribute__((packed));

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
    CLOSE_WAIT,
    FIN_WAIT_2,
    LAST_ACK,
    TIME_WAIT,
    CLOSING,
};

struct tcp_endpoint {
    uint32_t ip;
    uint16_t port;
};

struct tcp_ack_seq {
    uint32_t sequence_number;
    uint32_t acknowledgement_number;
};

struct tcp_virtual_channel {
    int socket_id;
    enum tcp_state state;
    struct tcp_endpoint my_side;
    struct tcp_endpoint client_side;
    struct tcp_ack_seq channel_situation;
    uint8_t buffer[TCP_DATA_LEN * 10];
    uint32_t data_len;
};

int tcp_send(struct tcp_virtual_channel* channel, struct tcp_pkt* pkt, size_t length);
int tcp_recv(struct ip_pkt* pkt);

#endif