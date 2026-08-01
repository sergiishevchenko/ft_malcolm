#include "test.h"

int	g_tests_run;
int	g_tests_passed;
int	g_tests_failed;

void	run_validate_ip_tests(void);
void	run_validate_mac_tests(void);
void	run_parsing_tests(void);

int	main(void)
{
	printf("\n═══════════════════════════════════════════════\n");
	printf("         ft_malcolm test suite\n");
	printf("═══════════════════════════════════════════════\n");
	run_validate_ip_tests();
	run_validate_mac_tests();
	run_parsing_tests();
	TEST_SUMMARY();
	return (g_tests_failed != 0);
}
