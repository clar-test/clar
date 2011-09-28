#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* required for sandboxing */
#include <sys/types.h>
#include <sys/stat.h>

#define clay_print(...) ${clay_print}

#ifdef _WIN32
#	include <windows.h>
#	include <io.h>
#	include <shellapi.h>
#	include <direct.h>
#	pragma comment(lib, "shell32")

#	define _MAIN_CC __cdecl

#	define stat(path, st) _stat(path, st)
#	define mkdir(path, mode) _mkdir(path)
#	define chdir(path) _chdir(path)
#	define access(path, mode) _access(path, mode)
#	define strdup(str) _strdup(str)

#	ifndef __MINGW32__
#		define strncpy(to, from, to_size) strncpy_s(to, to_size, from, _TRUNCATE)
#		define W_OK 02
#		define S_ISDIR(x) ((x & _S_IFDIR) != 0)
#	endif
	typedef struct _stat STAT_T;
#else
#	include <sys/wait.h> /* waitpid(2) */
#	include <unistd.h>
#	define _MAIN_CC
	typedef struct stat STAT_T;
#endif

#include "clay.h"

static void fs_rm(const char *_source);
static void fs_copy(const char *_source, const char *dest);

static const char *
fixture_path(const char *base, const char *fixture_name);

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
	size_t suite_n;
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

static void
clay_run_test(
	const struct clay_func *test,
	const struct clay_func *initialize,
	const struct clay_func *cleanup)
{
	int error_st = _clay.suite_errors;

	_clay.trampoline_enabled = 1;

	if (setjmp(_clay.trampoline) == 0) {
		if (initialize->ptr != NULL)
			initialize->ptr();

		test->ptr();
	}

	_clay.trampoline_enabled = 0;

	if (_clay.local_cleanup != NULL)
		_clay.local_cleanup(_clay.local_cleanup_payload);

	if (cleanup->ptr != NULL)
		cleanup->ptr();

	_clay.test_count++;

	/* remove any local-set cleanup methods */
	_clay.local_cleanup = NULL;
	_clay.local_cleanup_payload = NULL;

	clay_print("%c", (_clay.suite_errors > error_st) ? 'F' : '.');
}

static void
clay_print_error(int num, const struct clay_error *error)
{
	clay_print("  %d) Failure:\n", num);

	clay_print("%s::%s (%s) [%s:%d] [-t%d]\n",
		error->suite,
		error->test,
		"no description",
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
clay_run_single(const struct clay_func *test,
	const struct clay_suite *suite)
{
	_clay.suite_errors = 0;
	_clay.active_suite = suite->name;
	_clay.active_test = test->name;

	clay_run_test(test, &suite->initialize, &suite->cleanup);
}

static void
clay_usage(const char *arg)
{
	printf("Usage: %s [options]\n\n", arg);
	printf("Options:\n");
	printf("  -tXX\t\tRun only the test number XX\n");
	printf("  -sXX\t\tRun only the suite number XX\n");
	exit(-1);
}

static void
clay_parse_args(
	int argc, char **argv,
	const struct clay_func *callbacks,
	size_t cb_count,
	const struct clay_suite *suites,
	size_t suite_count)
{
	int i;

	for (i = 1; i < argc; ++i) {
		char *argument = argv[i];
		char action;
		int num;

		if (argument[0] != '-')
			clay_usage(argv[0]);

		action = argument[1];
		num = strtol(argument + 2, &argument, 10);

		if (*argument != '\0' || num < 0)
			clay_usage(argv[0]);

		switch (action) {
		case 't':
			if ((size_t)num >= cb_count) {
				fprintf(stderr, "Test number %d does not exist.\n", num);
				exit(-1);
			}

			clay_print("Started (%s::%s)\n",
				suites[callbacks[num].suite_n].name,
				callbacks[num].name);

			clay_run_single(&callbacks[num], &suites[callbacks[num].suite_n]);
			break;

		case 's':
			if ((size_t)num >= suite_count) {
				fprintf(stderr, "Suite number %d does not exist.\n", num);
				exit(-1);
			}

			clay_print("Started (%s::*)\n", suites[num].name);
			clay_run_suite(&suites[num]);
			break;

		default:
			clay_usage(argv[0]);
		}
	}
}

static int
clay_test(
	int argc, char **argv,
	const char *suites_str,
	const struct clay_func *callbacks,
	size_t cb_count,
	const struct clay_suite *suites,
	size_t suite_count)
{
	clay_print("Loaded %d suites: %s\n", (int)suite_count, suites_str);

	if (clay_sandbox() < 0) {
		fprintf(stderr,
			"Failed to sandbox the test runner.\n"
			"Testing will proceed without sandboxing.\n");
	}

	if (argc > 1) {
		clay_parse_args(argc, argv,
			callbacks, cb_count, suites, suite_count);

	} else {
		size_t i;
		clay_print("Started\n");

		for (i = 0; i < suite_count; ++i) {
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
	const char *description,
	int should_abort)
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
	_clay.total_errors++;

	if (should_abort) {
		if (!_clay.trampoline_enabled) {
			fprintf(stderr,
				"Fatal error: a cleanup method raised an exception.");
			exit(-1);
		}

		longjmp(_clay.trampoline, -1);
	}
}

void cl_set_cleanup(void (*cleanup)(void *), void *opaque)
{
	_clay.local_cleanup = cleanup;
	_clay.local_cleanup_payload = opaque;
}

${clay_modules}

static const struct clay_func _all_callbacks[] = {
    ${test_callbacks}
};

static const struct clay_suite _all_suites[] = {
    ${test_suites}
};

static const char _suites_str[] = "${suites_str}";

int _MAIN_CC main(int argc, char *argv[])
{
    return clay_test(
        argc, argv, _suites_str,
        _all_callbacks, ${cb_count},
        _all_suites, ${suite_count}
    );
}
