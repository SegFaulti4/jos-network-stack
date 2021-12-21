#include <kern/ip.h>
#include <inc/string.h>
#include <kern/inet.h>
#include <inc/error.h>
#include <kern/ethernet.h>
#include <kern/icmp.h>
#include <inc/stdio.h>

int16_t get_checksum(int8_t data[], int32_t length)
{
  int i = 0;
  int32_t temp = 0;
  int16_t checksum = 0;

  while (i < length)
  {
    if ((i+1) < length)
    {
      temp = (int32_t)checksum + ((((int32_t)data[i])<<8) + data[i+1]);
      checksum = (temp & 0xffff) + (temp >> 16);
      i = i + 2;
    }
    else
    {
      temp = checksum  + (data[i] << 8);
      checksum = (temp & 0xffff) + (temp >> 16);
      i = i + 1;
    }
  }
  checksum = ~checksum;
  return checksum;
}

int
icmp_echo_reply(struct ip_pkt* pkt) {
    struct icmp_pkt icmp_packet;
    int size = JNTOHS(pkt->hdr.ip_total_length) - IP_HEADER_LEN;
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

    hdr->checksum = 0;
    hdr->checksum = get_checksum((void*)&icmp_packet, sizeof(icmp_packet));

    struct ip_pkt result;
    result.hdr.ip_protocol = IP_PROTO_ICMP;
    result.hdr.ip_source_address = JHTONL(MY_IP);
    result.hdr.ip_destination_address = JHTONL(HOST_IP);
    memcpy((void*)result.data, (void*)&icmp_packet, size);
    return ip_send(&result, size);
}
