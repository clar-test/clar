#ifndef __CLAY_TEST_H__
#define __CLAY_TEST_H__

#include <stdlib.h>

struct clay_func {
	const char *name;
	void (*ptr)(void);
};

struct clay_suite {
	const char *name;
	struct clay_func initialize;
	struct clay_func cleanup;
	const struct clay_func *tests;
	size_t test_count;
};

void clay__assert(
	int condition,
	const char *file,
	int line,
	const char *error,
	const char *description);

int clay_test(
	int argc, char **argv,
	const struct clay_suite *suites, size_t count);

#define clay_pass(expr, desc) clay__assert((expr) >= 0, __FILE__, __LINE__, "Function call failed: " #expr, desc)
#define clay_fail(expr, desc) clay__assert((expr) < 0, __FILE__, __LINE__, "Expected function call to fail: " #expr, desc)
#define clay_assert(expr, desc) clay__assert((expr) != 0, __FILE__, __LINE__, "Expression is not true: " #expr, desc)

#endif