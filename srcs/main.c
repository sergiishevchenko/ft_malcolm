#include "ft_malcolm.h"

volatile sig_atomic_t	g_running = 1;

int	main(int argc, char **argv)
{
	t_malcolm	ctx;

	if (getuid() != 0)
	{
		fprintf(stderr, "%s: must be run as root\n", PROGRAM_NAME);
		return (1);
	}
	ft_bzero(&ctx, sizeof(ctx));
	ctx.sockfd = -1;
	if (parse_args(&ctx, argc, argv) != 0)
		return (1);
	setup_signals();
	if (find_interface(&ctx) != 0)
		return (1);
	if (open_raw_socket(&ctx) != 0)
		return (1);
	if (listen_arp_request(&ctx) != 0)
	{
		close(ctx.sockfd);
		if (!g_running)
			printf("\n");
		return (!g_running ? 0 : 1);
	}
	if (send_arp_reply(&ctx) != 0)
	{
		close(ctx.sockfd);
		return (1);
	}
	close(ctx.sockfd);
	return (0);
}
