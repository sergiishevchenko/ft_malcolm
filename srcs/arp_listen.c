#include "ft_malcolm.h"

static int	is_matching_request(t_arp_packet *pkt, t_malcolm *ctx)
{
	static const uint8_t	bcast[MAC_LEN] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};

	if (ft_memcmp(pkt->eth.dest, bcast, MAC_LEN) != 0)
		return (0);
	if (ntohs(pkt->arp.opcode) != ARP_OP_REQUEST)
		return (0);
	if (ft_memcmp(pkt->arp.target_ip, ctx->source_ip, IPV4_LEN) != 0)
		return (0);
	if (ft_memcmp(pkt->arp.sender_ip, ctx->target_ip, IPV4_LEN) != 0)
		return (0);
	return (1);
}

static void	print_request_info(t_arp_packet *pkt)
{
	printf("An ARP request has been broadcast.\n");
	printf("mac address of request: ");
	print_mac(pkt->arp.sender_mac);
	printf("\nIP address of request: ");
	print_ip(pkt->arp.sender_ip);
	printf("\n");
}

int	listen_arp_request(t_malcolm *ctx)
{
	unsigned char	buf[4096];
	ssize_t			len;
	t_arp_packet	*pkt;

	while (g_running)
	{
		len = recvfrom(ctx->sockfd, buf, sizeof(buf), 0, NULL, NULL);
		if (len < 0)
		{
			if (errno == EINTR)
				continue;
			fprintf(stderr, "%s: recvfrom: %s\n", PROGRAM_NAME,
				strerror(errno));
			return (-1);
		}
		if ((size_t)len < sizeof(t_arp_packet))
			continue;
		pkt = (t_arp_packet *)buf;
		if (ctx->verbose)
			print_verbose_pkt(pkt, (size_t)len, 0);
		if (!is_matching_request(pkt, ctx))
			continue;
		print_request_info(pkt);
		return (0);
	}
	return (-1);
}
