#include "ft_malcolm.h"

static void	print_usage(void)
{
	printf("Usage: %s [-v] [-c] [-g] [-i interface] "
		"<source_ip> <source_mac> <target_ip> <target_mac>\n",
		PROGRAM_NAME);
}

static int	parse_options(t_malcolm *ctx, int argc, char **argv)
{
	int	i;

	i = 1;
	while (i < argc && argv[i][0] == '-')
	{
		if (ft_strcmp(argv[i], "-v") == 0
			|| ft_strcmp(argv[i], "--verbose") == 0)
		{
			ctx->verbose = 1;
			i++;
		}
		else if (ft_strcmp(argv[i], "-c") == 0
			|| ft_strcmp(argv[i], "--continuous") == 0)
		{
			ctx->continuous = 1;
			i++;
		}
		else if (ft_strcmp(argv[i], "-g") == 0
			|| ft_strcmp(argv[i], "--gratuitous") == 0)
		{
			ctx->gratuitous = 1;
			i++;
		}
		else if (ft_strcmp(argv[i], "-i") == 0)
		{
			if (i + 1 >= argc)
			{
				fprintf(stderr, "%s: -i requires an argument\n",
					PROGRAM_NAME);
				return (-1);
			}
			ft_strlcpy(ctx->iface_name, argv[i + 1], IFNAMSIZ);
			ctx->iface_set = 1;
			i += 2;
		}
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n",
				PROGRAM_NAME, argv[i]);
			return (-1);
		}
	}
	return (i);
}

static int	parse_positional(t_malcolm *ctx, int argc, char **argv, int idx)
{
	if (argc - idx != 4)
	{
		print_usage();
		return (1);
	}
	if (validate_ip(argv[idx], ctx->source_ip) != 0)
	{
		fprintf(stderr, "%s: unknown host or invalid IP address: (%s).\n",
			PROGRAM_NAME, argv[idx]);
		return (1);
	}
	if (validate_mac(argv[idx + 1], ctx->source_mac) != 0)
	{
		fprintf(stderr, "%s: invalid mac address: (%s)\n",
			PROGRAM_NAME, argv[idx + 1]);
		return (1);
	}
	if (validate_ip(argv[idx + 2], ctx->target_ip) != 0)
	{
		fprintf(stderr, "%s: unknown host or invalid IP address: (%s).\n",
			PROGRAM_NAME, argv[idx + 2]);
		return (1);
	}
	if (validate_mac(argv[idx + 3], ctx->target_mac) != 0)
	{
		fprintf(stderr, "%s: invalid mac address: (%s)\n",
			PROGRAM_NAME, argv[idx + 3]);
		return (1);
	}
	return (0);
}

int	parse_args(t_malcolm *ctx, int argc, char **argv)
{
	int	idx;

	idx = parse_options(ctx, argc, argv);
	if (idx < 0)
		return (1);
	return (parse_positional(ctx, argc, argv, idx));
}
