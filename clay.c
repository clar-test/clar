#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* required for sandboxing */
#include <sys/stat.h>
#include <unistd.h>

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

/* From clay_sandbox.c */
static void clay_unsandbox(void);
static int clay_sandbox(void);

/* From clay.c */
static void clay_run_test(
	const struct clay_func *test,
	const struct clay_func *initialize,
	const struct clay_func *cleanup);

static int clay_test(
	int argc, char **argv,
	const struct clay_suite *suites, size_t count);

static void clay_run_suite(
	const struct clay_suite *suite);


static void
clay_run_test(
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

static void
clay_run_suite(const struct clay_suite *suite)
{
	const struct clay_func *test = suite->tests;
	size_t i;

	_clay_status.active_suite = suite->name;
	_clay_status.suite_errors = 0;

	for (i = 0; i < suite->test_count; ++i) {
		_clay_status.active_test = test[i].name;
		clay_run_test(&test[i], &suite->initialize, &suite->cleanup);
	}

	_clay_status.total_errors += _clay_status.suite_errors;
}

static void
clay_run_single(int test_id,
	const struct clay_suite *suites, size_t count)
{
	size_t i;
	const struct clay_suite *suite;

	for (i = 0; i < count; ++i) {
		suite = &suites[i];

		if (suite->test_count > test_id)
			break;

		test_id -= suite->test_count;
	}

	if (i == count || test_id < 0 || test_id >= suite->test_count) {
		fprintf(stderr, "Test number %d does not exist on the suite.\n", test_id);
		exit(-1);
	}

	_clay_status.suite_errors = 0;
	_clay_status.active_suite = suite->name;
	_clay_status.active_test = suite->tests[test_id].name;

	clay_run_test(&suite->tests[test_id], &suite->initialize, &suite->cleanup);

	_clay_status.total_errors += _clay_status.suite_errors;
}


static int
clay_test(
	int argc, char **argv,
	const struct clay_suite *suites, size_t count)
{

	if (!clay_sandbox())
		return -1;

	if (argc > 1) {
		int i;

		for (i = 1; i < argc; ++i) {
			char *argument = argv[i];
			int test_id;

			if (argument[0] == '-' && argument[1] == 't')
				test_id = strtol(argument + 2, &argument, 10);

			if (*argument != '\0')
				return -1;

			clay_run_single(test_id, suites, count);
		}
	} else {
		size_t i;

		for (i = 0; i < count; ++i) {
			const struct clay_suite *s = &suites[i];
			clay_run_suite(s);
		}
	}

	clay_unsandbox();

	return _clay_status.total_errors;
}

void
clay__assert(
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
