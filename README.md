Come out and Clar
=================

In Catalan, "clar" means clear, easy to perceive.  Using clar will make it
easy to test and make clear the quality of your code.

(_Historical note_:

Originally the clar project was named "clay" because the word "test" has its
roots in the latin word *"testum"*, meaning "earthen pot", and *"testa"*,
meaning "piece of burned clay"?

This is because historically, testing implied melting metal in a pot to
check its quality.  Clay is what tests are made of.)

## Usage Overview

Clar is a minimal C unit testing framework. It's been written to replace the
old framework in [libgit2][libgit2], but it's both very versatile and
straightforward to use.

Can you count to funk?

- **One: Write some tests**

    ~~~~ c
    /* adding.c for the "Adding" suite */
    #include "clar.h"

    static int *answer;

    void test_adding__initialize(void)
    {
        answer = malloc(sizeof(int));
        cl_assert_(answer != NULL, "No memory left?");
        *answer = 42;
    }

    void test_adding__cleanup(void)
    {
        free(answer);
    }

    void test_adding__make_sure_math_still_works(void)
    {
        cl_assert_(5 > 3, "Five should probably be greater than three");
        cl_assert_(-5 < 2, "Negative numbers are small, I think");
        cl_assert_(*answer == 42, "The universe is doing OK. And the initializer too.");
    }
    ~~~~~

- **Two: Mix them with Clar**

        $ ./clar.py .

        Loading test suites...
          adding.c: 1 tests
        Written test suite to "./clar_main.c"
        Written header to "./clar.h"

- **Three: Build the test executable**

        $ gcc clar_main.c adding.c -o testit

- **Funk: Funk it.**

        $ ./testit

## The Clar Test Suite

Writing a test suite is pretty straighforward. Each test suite is a `*.c`
file with a descriptive name: this encourages modularity.

Each test suite has optional initialize and cleanup methods. These methods
will be called before and after running **each** test in the suite, even if
such test fails. As a rule of thumb, if a test needs a different initializer
or cleanup method than another test in the same module, that means it
doesn't belong in that module. Keep that in mind when grouping tests
together.

The `initialize` and `cleanup` methods have the following syntax, with
`suitename` being the current suite name, e.g. `adding` for the `adding.c`
suite.

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
be static. Clar will automatically detect and list them.

Tests are run as they appear on their original suites: they have no return
value. A test is considered "passed" if it doesn't raise any errors. Check
the "Clar API" section to see the various helper functions to check and raise
errors during test execution.

#### Categorizing Tests

As your test suite grows, you may wish to create categories of tests that
can be run selectively.  To do this, you just put one or more `CL_IN_CATEGORY`
declarations into your test file, like so:

~~~~ c
CL_IN_CATEGORY("status")
CL_IN_CATEGORY("quick")
~~~~

When you go to run the test executable, you can pass use `-i=<category>`
to enable tests in a category.  Uncategorized tests are put in category
"default" which is enabled if you don't give any other category to run.
You can run multiple categories by using the `-i` argument more than once.
Also, you can use the special "all" category to run everything.

## The Clar Mixer

The Clar mixer, also known as `clar.py` is the only file needed to use Clar.

The mixer is a Python script with embedded resources, which automatically
generates all the files required to build a test suite. No external files
need to be copied to your test folder.

The mixer can be run with **Python 2.5, 2.6, 2.7, 3.0, 3.1, 3.2 and PyPy 1.6**.

Commandline usage is as follows:

    $ ./clar.py .

Where `.` is the folder where all the test suites can be found. The mixer
will automatically locate all the relevant source files and build the
testing boilerplate.

The testing boilerplate will be written to `clar_main.c`, in the same folder
as all the test suites. This boilerplate has no dependencies whatsoever:
building together this file with the rest of the suites will generate a
fully functional test executable.

    $ gcc -I. clar_main.c suite1.c test2.c -o run_tests

**Do note that the Clar mixer only needs to be ran when adding new tests to
a suite, in order to regenerate the boilerplate**. Consequently, the
`clar_main.c` boilerplate can be checked in version control to allow
building the test suite without any prior processing.

This is handy when e.g. generating tests in a local computer, and then
building and testing them on an embedded device or a platform where Python
is not available.

## The Clar API

Clar makes the following methods available from all functions in a
test suite.

-   `cl_must_pass(call)`, `cl_must_pass_(call, message)`: Verify that the given
    function call passes, in the POSIX sense (returns a value greater or equal
    to 0).

-   `cl_must_fail(call)`, `cl_must_fail_(call, message)`: Verify that the given
    function call fails, in the POSIX sense (returns a value less than 0).

-   `cl_assert(expr)`, `cl_assert_(expr, message)`: Verify that `expr` is true.

-   `cl_check_pass(call)`, `cl_check_pass_(call, message)`: Verify that the
    given function call passes, in the POSIX sense (returns a value greater or
    equal to 0). If the function call doesn't succeed, a test failure will be
    logged but the test's execution will continue.

-   `cl_check_fail(call)`, `cl_check_fail_(call, message)`: Verify that the
    given function call fails, in the POSIX sense (returns a value less than
    0). If the function call doesn't fail, a test failure will be logged but
    the test's execution will continue.

-   `cl_check(expr)`: Verify that `expr` is true. If `expr` is not
    true, a test failure will be logged but the test's execution will continue.

-   `cl_fail(message)`: Fail the current test with the given message.

-   `cl_warning(message)`: Issue a warning. This warning will be
    logged as a test failure but the test's execution will continue.

-   `cl_set_cleanup(void (*cleanup)(void *), void *opaque)`: Set the cleanup
    method for a single test. This method will be called with `opaque` as its
    argument before the test returns (even if the test has failed).
    If a global cleanup method is also available, the local cleanup will be
    called first, and then the global.

-   `cl_assert_equal_i(int,int)`: Verify that two integer values are equal.
    The advantage of this over a simple `cl_assert` is that it will format
	a much nicer error report if the values are not equal.

-   `cl_assert_equal_s(const char *,const char *)`: Verify that two strings
    are equal.  The expected value can also be NULL and this will correctly
    test for that.

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
    cl_must_pass_(
        check_string("foo"),
        "String differs after processing"
    );

    cl_must_pass_(
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

    cl_assert_(
        aux != NULL,
        "String processing failed"
    );

    cl_assert_(
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

Global Events
=============

If Clar detects any of the following functions declared in any of the
parsed suite files, they will be treated as global event callbacks and
issued on the corresponding events.

- `void clar_on_init()`: will be called as soon as Clar is initialized
- `void clar_on_shutdown()`: will be called beofre Clar shutdowns
- `void clar_on_test()`: will be called right before each test
- `void clar_on_suite()`: will be called right before each suite

These are useful when you are testing a library that has a global initializer
or the likes.

About
=====

Clar has been written from scratch by Vicent Martí, to replace the old
testing framework in [libgit2][libgit2].

Do you know what languages are *in* on the SF startup scene? Node.js *and*
Latin.  Follow [@vmg](https://www.twitter.com/vmg) on Twitter to
receive more lessons on word etymology. You can be hip too.


[libgit2]: https://github.com/libgit2/libgit2
