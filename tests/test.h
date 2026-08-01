#ifndef TEST_H
# define TEST_H

# include <stdio.h>
# include <string.h>

extern int g_tests_run;
extern int g_tests_passed;
extern int g_tests_failed;

# define TEST(name) \
	static void test_##name(void); \
	static void run_##name(void) { \
		g_tests_run++; \
		printf("  %-50s", #name); \
		test_##name(); \
	} \
	static void test_##name(void)

# define ASSERT(cond) do { \
	if (!(cond)) { \
		printf("\033[31mFAIL\033[0m\n"); \
		printf("    assertion failed: %s\n", #cond); \
		printf("    at %s:%d\n", __FILE__, __LINE__); \
		g_tests_failed++; \
		return; \
	} \
} while (0)

# define ASSERT_EQ(a, b) do { \
	if ((a) != (b)) { \
		printf("\033[31mFAIL\033[0m\n"); \
		printf("    expected %d, got %d\n", (int)(b), (int)(a)); \
		printf("    at %s:%d\n", __FILE__, __LINE__); \
		g_tests_failed++; \
		return; \
	} \
} while (0)

# define ASSERT_MEM_EQ(a, b, n) do { \
	if (memcmp((a), (b), (n)) != 0) { \
		printf("\033[31mFAIL\033[0m\n"); \
		printf("    memory mismatch (%zu bytes)\n", (size_t)(n)); \
		printf("    at %s:%d\n", __FILE__, __LINE__); \
		g_tests_failed++; \
		return; \
	} \
} while (0)

# define PASS() do { \
	printf("\033[32mOK\033[0m\n"); \
	g_tests_passed++; \
} while (0)

# define RUN(name) run_##name()

# define TEST_SUITE(name) printf("\n\033[1m[%s]\033[0m\n", name)

# define TEST_SUMMARY() do { \
	printf("\n─────────────────────────────────────────────\n"); \
	printf("Results: %d passed, %d failed, %d total\n", \
		g_tests_passed, g_tests_failed, g_tests_run); \
	if (g_tests_failed == 0) \
		printf("\033[32m✔ All tests passed!\033[0m\n\n"); \
	else \
		printf("\033[31m✖ Some tests failed!\033[0m\n\n"); \
} while (0)

#endif
