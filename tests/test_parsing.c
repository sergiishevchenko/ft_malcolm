#include "test.h"
#include "ft_malcolm.h"

volatile sig_atomic_t	g_running = 1;

TEST(parse_basic_args)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "10.0.0.1", "aa:bb:cc:dd:ee:ff",
		"10.0.0.2", "11:22:33:44:55:66"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT_EQ(parse_args(&ctx, 5, argv), 0);
	ASSERT_EQ(ctx.source_ip[0], 10);
	ASSERT_EQ(ctx.source_ip[1], 0);
	ASSERT_EQ(ctx.source_ip[2], 0);
	ASSERT_EQ(ctx.source_ip[3], 1);
	ASSERT_EQ(ctx.source_mac[0], 0xaa);
	ASSERT_EQ(ctx.target_ip[3], 2);
	ASSERT_EQ(ctx.target_mac[0], 0x11);
	PASS();
}

TEST(parse_verbose_flag)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "-v", "10.0.0.1",
		"aa:bb:cc:dd:ee:ff", "10.0.0.2", "11:22:33:44:55:66"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT_EQ(parse_args(&ctx, 6, argv), 0);
	ASSERT_EQ(ctx.verbose, 1);
	PASS();
}

TEST(parse_verbose_long)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "--verbose", "10.0.0.1",
		"aa:bb:cc:dd:ee:ff", "10.0.0.2", "11:22:33:44:55:66"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT_EQ(parse_args(&ctx, 6, argv), 0);
	ASSERT_EQ(ctx.verbose, 1);
	PASS();
}

TEST(parse_continuous_flag)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "-c", "10.0.0.1",
		"aa:bb:cc:dd:ee:ff", "10.0.0.2", "11:22:33:44:55:66"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT_EQ(parse_args(&ctx, 6, argv), 0);
	ASSERT_EQ(ctx.continuous, 1);
	PASS();
}

TEST(parse_gratuitous_flag)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "-g", "10.0.0.1",
		"aa:bb:cc:dd:ee:ff", "10.0.0.2", "11:22:33:44:55:66"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT_EQ(parse_args(&ctx, 6, argv), 0);
	ASSERT_EQ(ctx.gratuitous, 1);
	PASS();
}

TEST(parse_interface_option)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "-i", "eth0", "10.0.0.1",
		"aa:bb:cc:dd:ee:ff", "10.0.0.2", "11:22:33:44:55:66"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT_EQ(parse_args(&ctx, 7, argv), 0);
	ASSERT_EQ(ctx.iface_set, 1);
	ASSERT(strcmp(ctx.iface_name, "eth0") == 0);
	PASS();
}

TEST(parse_all_flags)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "-v", "-c", "-g", "-i", "lo",
		"10.0.0.1", "aa:bb:cc:dd:ee:ff", "10.0.0.2", "11:22:33:44:55:66"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT_EQ(parse_args(&ctx, 10, argv), 0);
	ASSERT_EQ(ctx.verbose, 1);
	ASSERT_EQ(ctx.continuous, 1);
	ASSERT_EQ(ctx.gratuitous, 1);
	ASSERT_EQ(ctx.iface_set, 1);
	PASS();
}

TEST(parse_too_few_args)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "10.0.0.1", "aa:bb:cc:dd:ee:ff"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT(parse_args(&ctx, 3, argv) != 0);
	PASS();
}

TEST(parse_too_many_args)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "10.0.0.1", "aa:bb:cc:dd:ee:ff",
		"10.0.0.2", "11:22:33:44:55:66", "extra"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT(parse_args(&ctx, 6, argv) != 0);
	PASS();
}

TEST(parse_unknown_option)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "-x", "10.0.0.1",
		"aa:bb:cc:dd:ee:ff", "10.0.0.2", "11:22:33:44:55:66"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT(parse_args(&ctx, 6, argv) != 0);
	PASS();
}

TEST(parse_interface_missing_arg)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "-i"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT(parse_args(&ctx, 2, argv) != 0);
	PASS();
}

TEST(parse_invalid_source_ip)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "999.999.999.999",
		"aa:bb:cc:dd:ee:ff", "10.0.0.2", "11:22:33:44:55:66"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT(parse_args(&ctx, 5, argv) != 0);
	PASS();
}

TEST(parse_invalid_source_mac)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "10.0.0.1",
		"invalid_mac", "10.0.0.2", "11:22:33:44:55:66"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT(parse_args(&ctx, 5, argv) != 0);
	PASS();
}

TEST(parse_invalid_target_mac)
{
	t_malcolm	ctx;
	char		*argv[] = {"ft_malcolm", "10.0.0.1",
		"aa:bb:cc:dd:ee:ff", "10.0.0.2", "bad"};

	ft_bzero(&ctx, sizeof(ctx));
	ASSERT(parse_args(&ctx, 5, argv) != 0);
	PASS();
}

void	run_parsing_tests(void)
{
	TEST_SUITE("parse_args");
	RUN(parse_basic_args);
	RUN(parse_verbose_flag);
	RUN(parse_verbose_long);
	RUN(parse_continuous_flag);
	RUN(parse_gratuitous_flag);
	RUN(parse_interface_option);
	RUN(parse_all_flags);
	RUN(parse_too_few_args);
	RUN(parse_too_many_args);
	RUN(parse_unknown_option);
	RUN(parse_interface_missing_arg);
	RUN(parse_invalid_source_ip);
	RUN(parse_invalid_source_mac);
	RUN(parse_invalid_target_mac);
}
