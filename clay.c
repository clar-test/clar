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

	void (*local_cleanup)(void *);
	void *local_cleanup_payload;

	jmp_buf trampoline;
	int trampoline_enabled;
} _clay;

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
	int argc, char **argv, const char *all_suites,
	const struct clay_suite *suites, size_t count);

static void clay_run_suite(
	const struct clay_suite *suite);

static void
clay_run_test(
	const struct clay_func *test,
	const struct clay_func *initialize,
	const struct clay_func *cleanup)
{
	int failed = 0;

	_clay.trampoline_enabled = 1;

	if (setjmp(_clay.trampoline) == 0) {
		if (initialize->ptr != NULL)
			initialize->ptr();

		test->ptr();
	} else
		failed = 1;

	_clay.trampoline_enabled = 0;

	if (_clay.local_cleanup != NULL)
		_clay.local_cleanup(_clay.local_cleanup_payload);

	if (cleanup->ptr != NULL)
		cleanup->ptr();

	_clay.test_count++;

	/* remove any local-set cleanup methods */
	_clay.local_cleanup = NULL;
	_clay.local_cleanup_payload = NULL;

	clay_print("%c", failed ? 'F' : '.');
}

static void
clay_print_error(int num, const struct clay_error *error)
{
	clay_print("  %d) Failure:\n", num);

	clay_print("\"%s\" (%s) [%s:%d] [-t%d]\n",
		error->test,
		error->suite,
		error->file,
		error->line_number,
		error->test_number);

	clay_print("  %s\n", error->error_msg);

	if (error->description != NULL)
		clay_print("  %s\n", error->description);

	clay_print("\n");
}

static void
clay_report_errors(void)
{
	int i = 1;
	struct clay_error *error, *next;

	_clay.total_errors += _clay.suite_errors;

	error = _clay.errors;
	while (error != NULL) {
		next = error->next;
		clay_print_error(i++, error);
		free(error->description);
		free(error);
		error = next;
	}
}

static void
clay_run_suite(const struct clay_suite *suite)
{
	const struct clay_func *test = suite->tests;
	size_t i;

	_clay.active_suite = suite->name;
	_clay.suite_errors = 0;

	for (i = 0; i < suite->test_count; ++i) {
		_clay.active_test = test[i].name;
		clay_run_test(&test[i], &suite->initialize, &suite->cleanup);
	}
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
		fprintf(stderr, "Test number %d does not exist.\n", test_id);
		exit(-1);
	}

	_clay.suite_errors = 0;
	_clay.active_suite = suite->name;
	_clay.active_test = suite->tests[test_id].name;

	clay_run_test(&suite->tests[test_id], &suite->initialize, &suite->cleanup);
}

static int
clay_usage(void)
{
	exit(-1);
}

static void
clay_parse_args(
	int argc, char **argv,
	const struct clay_suite *suites, size_t count)
{
	int i;

	for (i = 1; i < argc; ++i) {
		char *argument = argv[i];
		char action;
		int num;

		if (argument[0] != '-')
			clay_usage();

		action = argument[1];
		num = strtol(argument + 2, &argument, 10);

		if (*argument != '\0')
			clay_usage();

		switch (action) {
		case 't':
			clay_run_single(num, suites, count);
			break;

		case 's':
			if (num >= (int)count || num < 0) {
				fprintf(stderr, "Suite number %d does not exist.\n", num);
				exit(-1);
			}

			clay_run_suite(&suites[num]);
			break;

		default:
			clay_usage();
		}
	}
}

static int
clay_test(
	int argc, char **argv, const char *all_suites,
	const struct clay_suite *suites, size_t count)
{
	clay_print("Loaded %d suites: %s\n", (int)count, all_suites);

	if (!clay_sandbox())
		return -1;

	clay_print("Started\n");

	if (argc > 1) {
		clay_parse_args(argc - 1, argv + 1, suites, count);
	} else {
		size_t i;

		for (i = 0; i < count; ++i) {
			const struct clay_suite *s = &suites[i];
			clay_run_suite(s);
		}
	}

	clay_print("\n\n");
	clay_report_errors();

	clay_unsandbox();
	return _clay.total_errors;
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

	if (_clay.errors == NULL)
		_clay.errors = error;

	if (_clay.last_error != NULL)
		_clay.last_error->next = error;

	_clay.last_error = error;

	error->test = _clay.active_test;
	error->test_number = _clay.test_count;
	error->suite = _clay.active_suite;
	error->file = file;
	error->line_number = line;
	error->error_msg = error_msg;

	if (description != NULL)
		error->description = strdup(description);

	_clay.suite_errors++;

	if (!_clay.trampoline_enabled) {
		fprintf(stderr,
			"Unhandled exception: a cleanup method raised an exception.");
		exit(-1);
	}

	longjmp(_clay.trampoline, -1);
}

void clay_set_cleanup(void (*cleanup)(void *), void *opaque)
{
	_clay.local_cleanup = cleanup;
	_clay.local_cleanup_payload = opaque;
}
