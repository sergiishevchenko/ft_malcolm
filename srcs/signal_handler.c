#include "ft_malcolm.h"

static void	sig_handler(int sig)
{
	(void)sig;
	g_running = 0;
}

int	setup_signals(void)
{
	struct sigaction	sa;

	ft_bzero(&sa, sizeof(sa));
	sa.sa_handler = sig_handler;
	sa.sa_flags = 0;
	if (sigaction(SIGINT, &sa, NULL) < 0)
	{
		fprintf(stderr, "%s: sigaction: %s\n", PROGRAM_NAME,
			strerror(errno));
		return (-1);
	}
	if (sigaction(SIGTERM, &sa, NULL) < 0)
	{
		fprintf(stderr, "%s: sigaction: %s\n", PROGRAM_NAME,
			strerror(errno));
		return (-1);
	}
	return (0);
}
