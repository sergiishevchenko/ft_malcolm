#include "test.h"
#include "ft_malcolm.h"

TEST(ip_dotted_valid)
{
	uint8_t ip[4];

	ASSERT_EQ(validate_ip("192.168.1.1", ip), 0);
	ASSERT_EQ(ip[0], 192);
	ASSERT_EQ(ip[1], 168);
	ASSERT_EQ(ip[2], 1);
	ASSERT_EQ(ip[3], 1);
	PASS();
}

TEST(ip_zero)
{
	uint8_t ip[4];

	ASSERT_EQ(validate_ip("0.0.0.0", ip), 0);
	ASSERT_EQ(ip[0], 0);
	ASSERT_EQ(ip[1], 0);
	ASSERT_EQ(ip[2], 0);
	ASSERT_EQ(ip[3], 0);
	PASS();
}

TEST(ip_broadcast)
{
	uint8_t ip[4];

	ASSERT_EQ(validate_ip("255.255.255.255", ip), 0);
	ASSERT_EQ(ip[0], 255);
	ASSERT_EQ(ip[1], 255);
	ASSERT_EQ(ip[2], 255);
	ASSERT_EQ(ip[3], 255);
	PASS();
}

TEST(ip_loopback)
{
	uint8_t ip[4];

	ASSERT_EQ(validate_ip("127.0.0.1", ip), 0);
	ASSERT_EQ(ip[0], 127);
	ASSERT_EQ(ip[1], 0);
	ASSERT_EQ(ip[2], 0);
	ASSERT_EQ(ip[3], 1);
	PASS();
}

TEST(ip_decimal_format)
{
	uint8_t ip[4];

	ASSERT_EQ(validate_ip("3232235777", ip), 0);
	ASSERT_EQ(ip[0], 192);
	ASSERT_EQ(ip[1], 168);
	ASSERT_EQ(ip[2], 1);
	ASSERT_EQ(ip[3], 1);
	PASS();
}

TEST(ip_decimal_zero)
{
	uint8_t ip[4];

	ASSERT_EQ(validate_ip("0", ip), 0);
	ASSERT_EQ(ip[0], 0);
	ASSERT_EQ(ip[1], 0);
	ASSERT_EQ(ip[2], 0);
	ASSERT_EQ(ip[3], 0);
	PASS();
}

TEST(ip_hostname_localhost)
{
	uint8_t ip[4];

	ASSERT_EQ(validate_ip("localhost", ip), 0);
	ASSERT_EQ(ip[0], 127);
	ASSERT_EQ(ip[1], 0);
	ASSERT_EQ(ip[2], 0);
	ASSERT_EQ(ip[3], 1);
	PASS();
}

TEST(ip_invalid_format)
{
	uint8_t ip[4];

	ASSERT(validate_ip("256.1.1.1", ip) != 0);
	PASS();
}

TEST(ip_invalid_chars)
{
	uint8_t ip[4];

	ASSERT(validate_ip("abc.def.ghi.jkl", ip) != 0);
	PASS();
}

TEST(ip_empty_string)
{
	uint8_t ip[4];

	ASSERT(validate_ip("", ip) != 0);
	PASS();
}

TEST(ip_too_many_octets)
{
	uint8_t ip[4];

	ASSERT(validate_ip("1.2.3.4.5", ip) != 0);
	PASS();
}

TEST(ip_trailing_dot)
{
	uint8_t ip[4];

	ASSERT(validate_ip("1.2.3.", ip) != 0);
	PASS();
}

void	run_validate_ip_tests(void)
{
	TEST_SUITE("validate_ip");
	RUN(ip_dotted_valid);
	RUN(ip_zero);
	RUN(ip_broadcast);
	RUN(ip_loopback);
	RUN(ip_decimal_format);
	RUN(ip_decimal_zero);
	RUN(ip_hostname_localhost);
	RUN(ip_invalid_format);
	RUN(ip_invalid_chars);
	RUN(ip_empty_string);
	RUN(ip_too_many_octets);
	RUN(ip_trailing_dot);
}
