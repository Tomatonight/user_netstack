#include "utils.h"
u_int16_t utils::checksum(char *buff, int len, int start_sum)
{
	u_int32_t sum = start_sum;
	sum += sum_every_16bits(buff, len);
	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);
	return ~sum;
}
u_int16_t utils::checksum_tran(u_int32_t saddr, u_int32_t daddr, char proto, char *data, u_int16_t len)
{

	u_int32_t sum = 0;
	struct pseudo_head head;
	memset(&head, 0, sizeof(pseudo_head));
	/* 需要保证传入的daddr以及saddr是网络字节序 */
	head.daddr = daddr;
	head.saddr = saddr;
	head.proto = proto;
	head.zero = 0;
	head.len = htons(len);
	sum = sum_every_16bits((char *)&head, sizeof(pseudo_head));
	return checksum(data, len, sum);
}

u_int32_t utils::sum_every_16bits(char *data, int len)
{
	u_int32_t sum = 0;
	u_int16_t *ptr = (u_int16_t *)data;
	u_int16_t answer = 0;

	while (len > 1)
	{
		/*  This is the inner loop */
		sum += *ptr++;
		len -= 2;
	}

	if (len == 1)
	{
		*(char *)(&answer) = *(char *)ptr;
		sum += answer;
	}

	return sum;
}