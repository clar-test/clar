#include "clar_test.h"
#include <sys/stat.h>

static int file_size(const char *filename)
{
	struct stat st;

	if (stat(filename, &st) == 0)
		return (int)st.st_size;
	return -1;
}

void test_sample__initialize(void)
{
	global_test_counter++;
}

void test_sample__cleanup(void)
{
	cl_fixture_cleanup("test");

	cl_assert(file_size("test/file") == -1);
}

void test_sample__1(void)
{
	cl_assert(1);
	cl_must_pass(0);  /* 0 == success */
	cl_must_fail(-1); /* <0 == failure */
}

void test_sample__2(void)
{
	cl_fixture_sandbox("test");

	cl_assert(file_size("test/nonexistent") == -1);
	cl_assert(file_size("test/file") > 0);
}

void test_sample__3(void)
{
	const char *actual = "expected";
	int value = 100;

	cl_assert_equal_s("expected", actual);
	cl_assert_equal_i(100, value);
	cl_assert_equal_b(1, value);       /* equal as booleans */
	cl_assert_equal_p(actual, actual); /* pointers to same object */
}
