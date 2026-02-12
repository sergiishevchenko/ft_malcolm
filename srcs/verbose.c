#include "ft_malcolm.h"

static void	print_hex_line(const unsigned char *data, size_t off, size_t len)
{
	size_t	i;

	printf("  %04zx: ", off);
	i = 0;
	while (i < 16)
	{
		if (off + i < len)
			printf("%02x ", data[off + i]);
		else
			printf("   ");
		if (i == 7)
			printf(" ");
		i++;
	}
	printf(" ");
	i = 0;
	while (i < 16 && off + i < len)
	{
		if (data[off + i] >= 0x20 && data[off + i] <= 0x7e)
			printf("%c", data[off + i]);
		else
			printf(".");
		i++;
	}
	printf("\n");
}

void	print_hex_dump(const unsigned char *data, size_t len)
{
	size_t	off;

	off = 0;
	while (off < len)
	{
		print_hex_line(data, off, len);
		off += 16;
	}
}

static const char	*opcode_str(uint16_t opcode)
{
	if (opcode == ARP_OP_REQUEST)
		return ("REQUEST");
	if (opcode == ARP_OP_REPLY)
		return ("REPLY");
	return ("OTHER");
}

void	print_verbose_pkt(t_arp_packet *pkt, size_t len, int out)
{
	uint16_t	op;

	if (out)
		printf("[VERBOSE] Sending ARP packet (%zu bytes):\n", len);
	else
		printf("[VERBOSE] Received ARP packet (%zu bytes):\n", len);
	printf("  Eth: src=");
	print_mac(pkt->eth.src);
	printf(" dst=");
	print_mac(pkt->eth.dest);
	printf(" type=0x%04x\n", ntohs(pkt->eth.ethertype));
	op = ntohs(pkt->arp.opcode);
	printf("  ARP: op=%s(%d) hw=%d proto=0x%04x\n",
		opcode_str(op), op,
		ntohs(pkt->arp.hw_type), ntohs(pkt->arp.proto_type));
	printf("    sender: ");
	print_mac(pkt->arp.sender_mac);
	printf(" / ");
	print_ip(pkt->arp.sender_ip);
	printf("\n    target: ");
	print_mac(pkt->arp.target_mac);
	printf(" / ");
	print_ip(pkt->arp.target_ip);
	printf("\n");
	print_hex_dump((unsigned char *)pkt, len);
}
