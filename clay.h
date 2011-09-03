#ifndef __CLAY_TEST_H__
#define __CLAY_TEST_H__

#include <stdlib.h>

void clay__assert(
	int condition,
	const char *file,
	int line,
	const char *error,
	const char *description);

void clay_set_cleanup(void (*cleanup)(void *), void *opaque);

#define clay_pass(expr, desc) clay__assert((expr) >= 0, __FILE__, __LINE__, "Function call failed: " #expr, desc)
#define clay_fail(expr, desc) clay__assert((expr) < 0, __FILE__, __LINE__, "Expected function call to fail: " #expr, desc)
#define clay_assert(expr, desc) clay__assert((expr) != 0, __FILE__, __LINE__, "Expression is not true: " #expr, desc)

#endif
