#include "test.h"
#include "ft_malcolm.h"

TEST(mac_valid_lowercase)
{
	uint8_t mac[6];

	ASSERT_EQ(validate_mac("aa:bb:cc:dd:ee:ff", mac), 0);
	ASSERT_EQ(mac[0], 0xaa);
	ASSERT_EQ(mac[1], 0xbb);
	ASSERT_EQ(mac[2], 0xcc);
	ASSERT_EQ(mac[3], 0xdd);
	ASSERT_EQ(mac[4], 0xee);
	ASSERT_EQ(mac[5], 0xff);
	PASS();
}

TEST(mac_valid_uppercase)
{
	uint8_t mac[6];

	ASSERT_EQ(validate_mac("AA:BB:CC:DD:EE:FF", mac), 0);
	ASSERT_EQ(mac[0], 0xaa);
	ASSERT_EQ(mac[1], 0xbb);
	ASSERT_EQ(mac[2], 0xcc);
	ASSERT_EQ(mac[3], 0xdd);
	ASSERT_EQ(mac[4], 0xee);
	ASSERT_EQ(mac[5], 0xff);
	PASS();
}

TEST(mac_valid_mixed_case)
{
	uint8_t mac[6];

	ASSERT_EQ(validate_mac("aA:Bb:cC:Dd:eE:Ff", mac), 0);
	ASSERT_EQ(mac[0], 0xaa);
	ASSERT_EQ(mac[1], 0xbb);
	ASSERT_EQ(mac[2], 0xcc);
	ASSERT_EQ(mac[3], 0xdd);
	ASSERT_EQ(mac[4], 0xee);
	ASSERT_EQ(mac[5], 0xff);
	PASS();
}

TEST(mac_all_zeros)
{
	uint8_t mac[6];

	ASSERT_EQ(validate_mac("00:00:00:00:00:00", mac), 0);
	ASSERT_EQ(mac[0], 0);
	ASSERT_EQ(mac[1], 0);
	ASSERT_EQ(mac[2], 0);
	ASSERT_EQ(mac[3], 0);
	ASSERT_EQ(mac[4], 0);
	ASSERT_EQ(mac[5], 0);
	PASS();
}

TEST(mac_broadcast)
{
	uint8_t mac[6];

	ASSERT_EQ(validate_mac("ff:ff:ff:ff:ff:ff", mac), 0);
	ASSERT_EQ(mac[0], 0xff);
	ASSERT_EQ(mac[1], 0xff);
	ASSERT_EQ(mac[2], 0xff);
	ASSERT_EQ(mac[3], 0xff);
	ASSERT_EQ(mac[4], 0xff);
	ASSERT_EQ(mac[5], 0xff);
	PASS();
}

TEST(mac_too_short)
{
	uint8_t mac[6];

	ASSERT(validate_mac("aa:bb:cc:dd:ee", mac) != 0);
	PASS();
}

TEST(mac_too_long)
{
	uint8_t mac[6];

	ASSERT(validate_mac("aa:bb:cc:dd:ee:ff:00", mac) != 0);
	PASS();
}

TEST(mac_wrong_separator)
{
	uint8_t mac[6];

	ASSERT(validate_mac("aa-bb-cc-dd-ee-ff", mac) != 0);
	PASS();
}

TEST(mac_invalid_hex)
{
	uint8_t mac[6];

	ASSERT(validate_mac("gg:hh:ii:jj:kk:ll", mac) != 0);
	PASS();
}

TEST(mac_empty_string)
{
	uint8_t mac[6];

	ASSERT(validate_mac("", mac) != 0);
	PASS();
}

TEST(mac_no_separators)
{
	uint8_t mac[6];

	ASSERT(validate_mac("aabbccddeeff", mac) != 0);
	PASS();
}

TEST(mac_partial_invalid)
{
	uint8_t mac[6];

	ASSERT(validate_mac("aa:bb:cc:dd:ee:zz", mac) != 0);
	PASS();
}

void	run_validate_mac_tests(void)
{
	TEST_SUITE("validate_mac");
	RUN(mac_valid_lowercase);
	RUN(mac_valid_uppercase);
	RUN(mac_valid_mixed_case);
	RUN(mac_all_zeros);
	RUN(mac_broadcast);
	RUN(mac_too_short);
	RUN(mac_too_long);
	RUN(mac_wrong_separator);
	RUN(mac_invalid_hex);
	RUN(mac_empty_string);
	RUN(mac_no_separators);
	RUN(mac_partial_invalid);
}
