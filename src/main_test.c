#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ab_test.c"
#include "pt_test.c"

typedef bool(*test_func)(void);

static const test_func TESTS[] = {
    test_ab,
    test_pt,
};

int
main(int argc, char* argv[])
{
    long num_tests = sizeof(TESTS) / sizeof(*TESTS);

    long successful_tests = 0;
    long failed_tests = 0;

    for (long i = 0; i < num_tests; i++) {
        test_func test = TESTS[i];
        if (test()) {
            successful_tests++;
        } else {
            failed_tests++;
        }
    }

    printf("%ld successful %s, %ld failed %s\n",
        successful_tests,
        successful_tests == 1 ? "test" : "tests",
        failed_tests,
        failed_tests == 1 ? "test" : "tests");

    return failed_tests == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
