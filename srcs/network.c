#include "ft_malcolm.h"

static int	get_iface_mac(t_malcolm *ctx, struct ifaddrs *ifap)
{
	struct ifaddrs		*ifa;
	struct sockaddr_ll	*sll;

	ifa = ifap;
	while (ifa)
	{
		if (ifa->ifa_addr && ft_strcmp(ifa->ifa_name, ctx->iface_name) == 0
			&& ifa->ifa_addr->sa_family == AF_PACKET)
		{
			sll = (struct sockaddr_ll *)ifa->ifa_addr;
			ft_memcpy(ctx->iface_mac, sll->sll_addr, MAC_LEN);
			ctx->iface_index = sll->sll_ifindex;
			return (0);
		}
		ifa = ifa->ifa_next;
	}
	return (-1);
}

static int	auto_detect_iface(t_malcolm *ctx, struct ifaddrs *ifap)
{
	struct ifaddrs	*ifa;

	ifa = ifap;
	while (ifa)
	{
		if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET
			&& !(ifa->ifa_flags & IFF_LOOPBACK)
			&& (ifa->ifa_flags & IFF_UP))
		{
			ft_strlcpy(ctx->iface_name, ifa->ifa_name, IFNAMSIZ);
			return (0);
		}
		ifa = ifa->ifa_next;
	}
	fprintf(stderr, "%s: no suitable network interface found\n",
		PROGRAM_NAME);
	return (-1);
}

int	find_interface(t_malcolm *ctx)
{
	struct ifaddrs	*ifap;

	if (getifaddrs(&ifap) == -1)
	{
		fprintf(stderr, "%s: getifaddrs: %s\n", PROGRAM_NAME,
			strerror(errno));
		return (-1);
	}
	if (!ctx->iface_set && auto_detect_iface(ctx, ifap) != 0)
	{
		freeifaddrs(ifap);
		return (-1);
	}
	if (get_iface_mac(ctx, ifap) != 0)
	{
		freeifaddrs(ifap);
		fprintf(stderr, "%s: could not get interface info for %s\n",
			PROGRAM_NAME, ctx->iface_name);
		return (-1);
	}
	freeifaddrs(ifap);
	printf("Found available interface: %s\n", ctx->iface_name);
	return (0);
}

int	open_raw_socket(t_malcolm *ctx)
{
	struct sockaddr_ll	sll;

	ctx->sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (ctx->sockfd < 0)
	{
		fprintf(stderr, "%s: socket: %s\n", PROGRAM_NAME, strerror(errno));
		return (-1);
	}
	ft_bzero(&sll, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ctx->iface_index;
	sll.sll_protocol = htons(ETH_P_ARP);
	if (bind(ctx->sockfd, (struct sockaddr *)&sll, sizeof(sll)) < 0)
	{
		fprintf(stderr, "%s: bind: %s\n", PROGRAM_NAME, strerror(errno));
		close(ctx->sockfd);
		return (-1);
	}
	return (0);
}
