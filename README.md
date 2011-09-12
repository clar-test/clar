Come out and Clay
=================

Did you know that the word "test" has its roots in the latin word *"testum"*, meaning
"earthen pot", and *"testa"*,  meaning "piece of burned clay"?

This is because historically, testing implied melting metal in a pot to check its quality.
Clay is what tests are made of.

## Usage Overview

Clay is a minimal C unit testing framework. It's been written to replace the old
framework in [libgit2][libgit2], but it's both very versatile
and straightforward to use.

Can you count to funk?

- **One: Write some tests**

    ~~~~ c
    /* adding.c for the "Adding" suite */
    #include "clay.h"

    static int *answer;

    void test_adding__initialize(void)
    {
        answer = malloc(sizeof(int));
        clay_assert(answer != NULL, "No memory left?");
        *answer = 42;
    }

    void test_adding__cleanup(void)
    {
        free(answer);
    }

    void test_adding__make_sure_math_still_works(void)
    {
        clay_assert(5 > 3, "Five should probably be greater than three");
        clay_assert(-5 < 2, "Negative numbers are small, I think");
        clay_assert(*answer == 42, "The universe is doing OK. And the initializer too.");
    }
    ~~~~~

- **Two: Mix them with Clay**

        $ ./clay.py .

        Loading test suites...
          adding.c: 1 tests
        Written test suite to "./clay_main.c"
        Written header to "./clay.h"

- **Three: Build the test executable**

        $ gcc clay_main.c adding.c -o testit

- **Funk: Funk it.**

        $ ./testit

## The Clay Test Suite

Writing a test suite is pretty straighforward. Each test suite is a `*.c` file
with a descriptive name: this encourages modularity.

Each test suite has an optional initializator and cleanup method. These methods
will be called before and after running **each** test in the suite, even if such
test fails. As a rule of thumb, if a test needs a different initializer or cleanup
method than another test in the same module, that means it doesn't belong in that
module. Keep that in mind when grouping tests together.

The `initialize` and `cleanup` methods have the following syntax, with `suitename`
being the current suite name, e.g. `adding` for the `adding.c` suite.

~~~~ c
void test_suitename__initialize(void)
{
    /* init */
}

void test_suitename__cleanup(void)
{
    /* cleanup */
}
~~~~

These methods are encouraged to use static, global variables to store the state
that will be used by all tests inside the suite.

~~~~ c
static git_repository *_repository;

void test_status__initialize(void)
{
    create_tmp_repo(STATUS_REPO);
    git_repository_open(_repository, STATUS_REPO);
}

void test_status__cleanup(void)
{
    git_repository_close(_repository);
    git_path_rm(STATUS_REPO);
}

void test_status__simple_test(void)
{
    /* do something with _repository */
}
~~~~

Writing the actual tests is just as straightforward. Tests have the
`void test_suitename__test_name(void)` signature, and they should **not**
be static. Clay will automatically detect and list them.

Tests are run as they appear on their original suites: they have no return
value. A test is considered "passed" if it doesn't raise any errors. Check
the "Clay API" section to see the various helper functions to check and raise
errors during test execution.


## The Clay Mixer

The Clay mixer, also known as `clay.py` is the only file needed to use Clay.

The mixer is a Python script with embedded resources, which automatically generates
all the files required to build a test suite. No external files need to be copied
to your test folder.

The mixer can be run with **Python 2.5, 2.6, 2.7, 3.0, 3.1, 3.2 and PyPy 1.6**.

Commandline usage is as follows:

    $ ./clay.py .

Where `.` is the folder where all the test suites can be found. The mixer will
automatically locate all the relevant source files and build the testing boilerplate.

The testing boilerplate will be written to `clay_main.c`, in the same folder as all
the test suites. This boilerplate has no dependencies whatsoever: building together
this file with the rest of the suites will generate a fully functional test executable.

    $ gcc -I. clay_main.c suite1.c test2.c -o run_tests

**Do note that the Clay mixer only needs to be ran when adding new tests to a suite,
in order to regenerate the boilerplate**. Consequently, the `clay_main.c` boilerplate can
be checked in version control to allow building the test suite without any prior processing.

This is handy when e.g. generating tests in a local computer, and then building and testing
them on an embedded device or a platform where Python is not available.


## The Clay API

Clay makes the following methods available from all functions in a
test suite.

-   `clay_must_pass(call, message)`: Verify that the given function call
    passes, in the POSIX sense (returns a value greater or equal to 0).

-   `clay_must_fail(call, message)`: Verify that the given function call
    fails, in the POSIX sense (returns a value less than 0). 

-   `clay_assert(expr, message)`: Verify that `expr` is true. 

-   `clay_check_pass(call, message)`: Verify that the given function call
    passes, in the POSIX sense (returns a value greater or equal to 0). If
    the function call doesn't succeed, a test failure will be logged but the
    test's execution will continue.

-   `clay_check_fail(call, message)`: Verify that the given function call
    fails, in the POSIX sense (returns a value less than 0). If the function
    call doesn't fail, a test failure will be logged but the test's execution
    will continue.

-   `clay_check(expr, message)`: Verify that `expr` is true. If `expr` is not
    true, a test failure will be logged but the test's execution will continue.

-   `clay_fail(message)`: Fail the current test with the given message.

-   `clay_warning(message)`: Issue a warning. This warning will be
    logged as a test failure but the test's execution will continue.

-   `clay_set_cleanup(void (*cleanup)(void *), void *opaque)`: Set the cleanup
    method for a single test. This method will be called with `opaque` as its
    argument before the test returns (even if the test has failed).
    If a global cleanup method is also available, the local cleanup will be
    called first, and then the global.

Please do note that these methods are *always* available whilst running a test,
even when calling auxiliary/static functions inside the same file. It's strongly
encouraged to perform test assertions in auxiliary methods, instead of returning
error values.

Example:

~~~~ c
/* Bad style: auxiliary functions return an error code */
static int check_string(const char *str)
{
    const char *aux = process_string(str);

    if (aux == NULL)
        return -1;

    return strcmp(my_function(aux), str) == 0 ? 0 : -1;
}

void test_example__a_test_with_auxiliary_methods(void)
{
    clay_must_pass(
        check_string("foo"),
        "String differs after processing"
    );

    clay_must_pass(
        check_string("bar"),
        "String differs after processing"
    );
}
~~~~

~~~~ c
/* Good style: auxiliary functions perform assertions */
static void check_string(const char *str)
{
    const char *aux = process_string(str);

    clay_assert(
        aux != NULL,
        "String processing failed"
    );

    clay_assert(
        strcmp(my_function(aux), str) == 0,
        "String differs after processing"
    );
}

void test_example__a_test_with_auxiliary_methods(void)
{
    check_string("foo");
    check_string("bar");
}
~~~~

About
=====

Clay has been written from scratch by Vicent Martí, to replace the old testing
framework in [libgit2][libgit2]. 

Do you know what languages are *in* on the SF startup scene? Node.js *and* Latin.
Follow [@tanoku](https://www.twitter.com/tanoku) on Twitter to receive more lessons
on word etymology. You can be hip too.


[libgit2]: https://github.com/libgit2/libgit2
