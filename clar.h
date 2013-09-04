/*
 * Copyright (c) Vicent Marti. All rights reserved.
 *
 * This file is part of clar, distributed under the ISC license.
 * For full terms see the included COPYING file.
 */
#ifndef __CLAR_TEST_H__
#define __CLAR_TEST_H__

#include <stdlib.h>

int clar_test(int argc, char *argv[]);

void cl_set_cleanup(void (*cleanup)(void *), void *opaque);
void cl_fs_cleanup(void);

#ifdef CLAR_FIXTURE_PATH
const char *cl_fixture(const char *fixture_name);
void cl_fixture_sandbox(const char *fixture_name);
void cl_fixture_cleanup(const char *fixture_name);
#endif

/**
 * Assertion macros with explicit error message
 */
#define cl_must_pass_(expr, desc) clar__assert((expr) >= 0, __FILE__, __LINE__, "Function call failed: " #expr, desc, 1)
#define cl_must_fail_(expr, desc) clar__assert((expr) < 0, __FILE__, __LINE__, "Expected function call to fail: " #expr, desc, 1)
#define cl_assert_(expr, desc) clar__assert((expr) != 0, __FILE__, __LINE__, "Expression is not true: " #expr, desc, 1)

/**
 * Check macros with explicit error message
 */
#define cl_check_pass_(expr, desc) clar__assert((expr) >= 0, __FILE__, __LINE__, "Function call failed: " #expr, desc, 0)
#define cl_check_fail_(expr, desc) clar__assert((expr) < 0, __FILE__, __LINE__, "Expected function call to fail: " #expr, desc, 0)
#define cl_check_(expr, desc) clar__assert((expr) != 0, __FILE__, __LINE__, "Expression is not true: " #expr, desc, 0)

/**
 * Assertion macros with no error message
 */
#define cl_must_pass(expr) cl_must_pass_(expr, NULL)
#define cl_must_fail(expr) cl_must_fail_(expr, NULL)
#define cl_assert(expr) cl_assert_(expr, NULL)

/**
 * Check macros with no error message
 */
#define cl_check_pass(expr) cl_check_pass_(expr, NULL)
#define cl_check_fail(expr) cl_check_fail_(expr, NULL)
#define cl_check(expr) cl_check_(expr, NULL)

/**
 * Forced failure/warning
 */
#define cl_fail(desc) clar__fail(__FILE__, __LINE__, "Test failed.", desc, 1)
#define cl_warning(desc) clar__fail(__FILE__, __LINE__, "Warning during test execution:", desc, 0)

/**
 * Typed assertion macros
 */
#define cl_assert_equal_s(s1,s2) clar__assert_equal(__FILE__,__LINE__,"String mismatch: " #s1 " != " #s2, 1, "%s", (s1), (s2))
#define cl_assert_equal_s_(s1,s2,note) clar__assert_equal(__FILE__,__LINE__,"String mismatch: " #s1 " != " #s2 " (" #note ")", 1, "%s", (s1), (s2))

#define cl_assert_equal_i(i1,i2) clar__assert_equal(__FILE__,__LINE__,#i1 " != " #i2, 1, "%d", (int)(i1), (int)(i2))
#define cl_assert_equal_i_(i1,i2,note) clar__assert_equal(__FILE__,__LINE__,#i1 " != " #i2 " (" #note ")", 1, "%d", (i1), (i2))
#define cl_assert_equal_i_fmt(i1,i2,fmt) clar__assert_equal(__FILE__,__LINE__,#i1 " != " #i2, 1, (fmt), (int)(i1), (int)(i2))

#define cl_assert_equal_b(b1,b2) clar__assert_equal(__FILE__,__LINE__,#b1 " != " #b2, 1, "%d", (int)((b1) != 0),(int)((b2) != 0))

#define cl_assert_equal_p(p1,p2) clar__assert_equal(__FILE__,__LINE__,"Pointer mismatch: " #p1 " != " #p2, 1, "%p", (p1), (p2))


void clar__fail(
	const char *file,
	int line,
	const char *error,
	const char *description,
	int should_abort);

void clar__assert(
	int condition,
	const char *file,
	int line,
	const char *error,
	const char *description,
	int should_abort);

void clar__assert_equal(
	const char *file,
	int line,
	const char *err,
	int should_abort,
	const char *fmt,
	...);

#endif
