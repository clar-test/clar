Clay
====

Clay is what tests are made of.

## Can you count to "funk"?

- **One: Write some tests!**

~~~~ c
/* adding.c for the "Adding" suite */
#include "clay.h"

static int *answer;

void test_adding__initialize()
{
    answer = malloc(sizeof(int));
    clay_assert(answer != NULL);
    *answer = 42;
}

void test_adding__cleanup()
{
    free(answer);
}

void test_adding__make_sure_math_still_works()
{
    clay_assert2(5 > 3, "Five should probably be greater than three");
    clay_assert2(-5 < 2, "Negative numbers are small, I think");
    clay_assert2(*answer == 42, "The universe is doing OK. And the initializer too.");
}
~~~~~

- **Two: Mix them with Clay!**

        $ clay .

- **Three: Build the test suite!**

    Where are the dependencies?? Somebody stole them??
    THERE ARE NO DEPENDENCIES JOHNNY THIS SHIT IS MAGICAL

        $ gcc -I. clay_main.c adding.c -o test_suite

- **Funk: Funk it.**
