#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "clay.h"

struct clay_error {
	const char *test;
	int test_number;
	const char *suite;
	const char *file;
	int line_number;
	const char *error_msg;
	char *description;

	struct clay_error *next;
};

static struct {
	const char *active_test;
	const char *active_suite;

	int suite_errors;
	int total_errors;

	int test_count;

	struct clay_error *errors;
	struct clay_error *last_error;

	jmp_buf trampoline;
	int trampoline_enabled;

} _clay_status;

static void
run_single_test(
	const struct clay_func *test,
	const struct clay_func *initialize,
	const struct clay_func *cleanup)
{
	_clay_status.trampoline_enabled = 1;

	if (setjmp(_clay_status.trampoline) == 0) {
		if (initialize->ptr != NULL)
			initialize->ptr();

		test->ptr();
	}

	_clay_status.trampoline_enabled = 0;
	if (cleanup->ptr != NULL)
		cleanup->ptr();

	_clay_status.test_count++;
}

void clay__assert(
	int condition,
	const char *file,
	int line,
	const char *error_msg,
	const char *description)
{
	struct clay_error *error;

	if (condition)
		return;

	error = calloc(1, sizeof(struct clay_error));

	if (_clay_status.errors == NULL)
		_clay_status.errors = error;

	if (_clay_status.last_error != NULL)
		_clay_status.last_error->next = error;

	_clay_status.last_error = error;

	error->test = _clay_status.active_test;
	error->test_number = _clay_status.test_count;
	error->suite = _clay_status.active_suite;
	error->file = file;
	error->line_number = line;
	error->error_msg = error_msg;

	if (description != NULL)
		error->description = strdup(description);

	_clay_status.suite_errors++;

	if (!_clay_status.trampoline_enabled) {
		fprintf(stderr,
			"Unhandled exception: a cleanup method raised an exception.");
		exit(-1);
	}

	longjmp(_clay_status.trampoline, -1);
}

static void
run_suite(const struct clay_suite *suite)
{
	const struct clay_func *test = suite->tests;
	size_t i;

	_clay_status.active_suite = suite->name;
	_clay_status.suite_errors = 0;

	for (i = 0; i < suite->test_count; ++i) {
		_clay_status.active_test = test[i].name;
		run_single_test(&test[i], &suite->initialize, &suite->cleanup);
		_clay_status.active_test = NULL;
	}

	_clay_status.active_suite = NULL;
	_clay_status.total_errors += _clay_status.suite_errors;
}

extern int clay_sandbox(void);
extern void clay_unsandbox(void);

int clay_test(
	int argc, char **argv,
	const struct clay_suite *suites, size_t count)
{
	size_t i;

	if (!clay_sandbox())
		return -1;

	for (i = 0; i < count; ++i) {
		const struct clay_suite *s = &suites[i];
		run_suite(s);
	}

	clay_unsandbox();

	return _clay_status.total_errors;
}
