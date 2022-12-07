[![Linux Build Status (travis-ci.com)](https://img.shields.io/travis/mity/acutest/master.svg?logo=linux&label=linux%20build)](https://travis-ci.com/mity/acutest)
[![Windows Build Status (appveyor.com)](https://img.shields.io/appveyor/ci/mity/acutest/master.svg?logo=windows&label=windows%20build)](https://ci.appveyor.com/project/mity/acutest/branch/master)


# Acutest Readme

Home: https://github.com/mity/acutest


## What Is Acutest

Acutest is C/C++ unit testing facility aiming to be as simple as possible, not
to stand in the developer's way and to minimize any external dependencies.

To achieve that, the complete implementation resides in a single C header file,
and its core depends only on few standard C library functions.


## Overview

**Main features:**
* Unit tests in C or C++ are supported.
* No need to install/setup/configure any testing framework. Acutest is just
  a single header file, `acutest.h`.
* The header provides the program entry point (function `main()`).
* Minimal dependencies: Core features only depend on few standard C headers,
  optional features may use more if available on the particular system.
* Trivial interface for writing unit tests: Few preprocessor macros described
  further below.
* Rudimentary support for [Test Anything Protocol](https://testanything.org/)
  (use `--tap` option).
* Rudimentary support for xUnit-compatible XML output (use `--xml-output=FILE`).

**C++ specific features:**
* Acutest catches any C++ exception thrown from any unit test function. When
  that happens, the given test is considered to fail.
* If the exception is derived from `std::exception`, `what()` is written out
  in the error message.

**Unix/Posix specific features:**
* By default, every unit test is executed as a child process.
* By default, if the output is directed to a terminal, the output is colorized.
* If the system offers Posix timer (`clock_gettime()`), user can measure test
  execution times with `--time=real` (same as `--time`) and `--time=cpu`.

**Linux specific features:**
* If a debugger is detected, the default execution of tests as child processes
  is suppressed in order to make the debugging easier.

**Windows specific features:**
* By default, every unit test is executed as a child process.
* If a debugger is detected, the default execution of tests as child processes
  is suppressed in order to make the debugging easier.
* By default, if the output is directed to a terminal, the output is colorized.
* Acutest installs a SEH filter to print out uncaught SEH exceptions.
* User can measure test execution times with `--time`.

**macOS specific features:**
* If a debugger is detected, the default execution of tests as child processes
  is suppressed in order to make the debugging easier.

Any C/C++ module implementing one or more unit tests and including `acutest.h`,
can be built as a standalone program. We call the resulted binary as a "test
suite" for purposes of this document. The suite is then executed to run the
tests, as specified with its command line options.

We say any unit test succeeds if and only if:
1. all condition checks (as described below) called throughout its execution
   pass;
2. the test does not throw any exception (C++ only); and
3. (on Windows or Unix) the unit test subprocess is not interrupted/terminated
   (e.g. by a signal on Unix or SEH on Windows).


## Writing Unit Tests

### Basic Use

To use Acutest, simply include the header file `acutest.h` on the beginning of
the C/C++ source file implementing one or more unit tests. Note the header
provides implementation of the `main()` function.

```C
#include "acutest.h"
```

Every test is supposed to be implemented as a function with the following
prototype:

```C
void test_example(void);
```

The tests can use some preprocessor macros to validate the test conditions.
They can be used multiple times, and if any of those conditions fails, the
particular test is considered to fail.

`TEST_CHECK` is the most commonly used testing macro which simply tests a
boolean condition and fails if the condition evaluates to false (or zero).

For example:

```C
void test_example(void)
{
    void* mem;
    int a, b;

    mem = malloc(10);
    TEST_CHECK(mem != NULL);

    mem = realloc(mem, 20);
    TEST_CHECK(mem != NULL);
}
```

Note that the tests should be completely independent on each other. Whenever
the test suite is invoked, the user may run any number of tests in the suite,
in any order. Furthermore by default, on platforms where supported, each unit
test is executed as a standalone (sub)process.

Finally, the test suite source file has to list the unit tests, using the
macro `TEST_LIST`. The list specifies name of each test (it has to be unique)
and pointer to a function implementing the test. I recommend names which are
easy to use on command line: especially avoid space and other special
characters which might require escaping in shell; also avoid dash (`-`) as a
first character of the name, as it could then be interpreted as a command line
option and not as a test name.

```C
TEST_LIST = {
   { "example", test_example },
   ...
   { NULL, NULL }     /* zeroed record marking the end of the list */
};
```

Note the test list has to be ended with zeroed record.

For a basic test suites this is more or less all you need to know. However
Acutest provides some more macros which can be useful in some specific
situations. We cover them in the following sub-sections.

### Aborting on a Check Failure

There is a macro `TEST_ASSERT` which is very similar to `TEST_CHECK` but, if it
fails, it aborts execution of the current unit test instantly.

For example:

```C
void test_example(void)
{
    void* mem;
    int a, b;

    mem = malloc(10);
    TEST_ASSERT(mem != NULL);

    mem = realloc(mem, 20);
    TEST_ASSERT(mem != NULL);
}
```

The abortion in the case of failure is performed either by calling `abort()`
(if the test is executed as a child process) or via `longjmp()` (if it is not).

Therefore it should be used only if you understand the costs connected with
such a brutal abortion of the test. Depending on what your unit test does,
it may include unflushed file descriptors, memory leaks, C++ objects destructed
without their destructors being called and more.

In general, `TEST_CHECK` should be preferred over `TEST_ASSERT`, unless you
know exactly what you do and why you chose `TEST_ASSERT` in some particular
situation.

### Testing C++ Exceptions

For C++, there is an additional macro `TEST_EXCEPTION` for verifying the given
code (typically just a function or a method call) throws the expected type of
exception.

The check fails if the function does not throw any exception or if it throws
anything incompatible.

For example:

```C++
void test_example(void)
{
    TEST_EXCEPTION(CallSomeFunction(), std::exception);
}
```

### Richer Failure Diagnosis

If a condition check fails, it is often useful to provide some additional
information about the situation so the problem is easier to debug. Acutest
provides the macros `TEST_MSG` and `TEST_DUMP` for this purpose.

The former one outputs any `printf`-like message, the other one outputs a
hexadecimal dump of a provided memory block.

So for example:

```C
void test_example(void)
{
    char produced[100];
    char expected[] = "Hello World!";

    SomeSprintfLikeFunction(produced, "Hello %s!", "world");
    TEST_CHECK(strcmp(produced, expected) == 0);
    TEST_MSG("Expected: %s", expected);
    TEST_MSG("Produced: %s", produced);

    /* Or, if the function could provide some binary stuff, we might rather
     * use TEST_DUMP instead in order to output a hexadecimal dump of the data.
     */
    TEST_DUMP("Expected:", expected, strlen(expected));
    TEST_DUMP("Produced:", produced, strlen(produced));
}
```

Note that both macros output anything only when the most recently checking
macro has failed. In other words, these two are equivalent:

```C
if(!TEST_CHECK(some_condition != 0))
    TEST_MSG("some message");
```

```C
TEST_CHECK(some_condition != 0);
TEST_MSG("some message");
```

(Note that `TEST_MSG` requires the compiler with variadic macros support.)

### Loops over Test Vectors

Sometimes, it is useful to design your testing function as a loop over data
providing a collection of test vectors and their respective expected outputs.
For example imagine our unit test is supposed to verify some kind of a hashing
function implementation and we've got a collection of test vectors for it in
the hash specification.

In such cases, it is very useful to get some name associated with every test
vector and output the name in the output log so that if any check fails, it is
easy to identify the guilty test vector. However, the loop body may execute
dozens of checking macros and so it may be impractical to add such name to
customize every check message in the loop.

To solve this, Acutest provides the macro `TEST_CASE`. The macro specifies
a string serving as the test vector name. When used, Acutest makes sure that
in the output log the provided name precedes any message from subsequent
condition checks.

For example, lets assume we are testing `SomeFunction()` which is supposed,
for a given byte array of some size, return another array of bytes in a newly
`malloc`-ed buffer. Then we can do something like this:

```C
struct TestVector {
    const char* name;
    const uint8_t* input;
    size_t input_size;
    const uint8_t* expected_output;
    size_t expected_output_size;
};

struct TestVector test_vectors[] = {
    /* some data */
};

void test_example(void)
{
    int i;
    const uint8_t* output;
    size_t output_size;

    for(i = 0; i < sizeof(test_vectors) / sizeof(test_vectors[0]); i++) {
        struct TestVector* vec = &test_vectors[i];

        /* Output the name of the tested test vector. */
        TEST_CASE(vec.name);

        /* Now, we can check the function produces what it should for the
         * current test vector. If any of the following checking macros
         * produces any output (either because the check fails, or because
         * high `--verbose` level is used), Acutest also outputs the  currently
         * tested vector's name. */
        output = SomeFunction(vec->input, vec->input_size, &output_size);
        if(TEST_CHECK(output != NULL)) {
            TEST_CHECK(output_size == vec->expected_output_size);
            TEST_CHECK(memcmp(output, vec->expected_output, output_size) == 0);
            free(output);
        }
    }
}
```

The specified name applies to all checks executed after the use of `TEST_CASE`
* until the unit test ends; or
* until `TEST_CASE` is used again to specify another name; or
* until the name is explicitly reset by using `TEST_CASE` with the `NULL`
  as its argument.

### Custom Log Messages

Many of the macros mentioned in the earlier sections have a counterpart which
allows to output a custom messages instead of some default ones.

All of these have the same name as the aforementioned macros, just with the
underscore suffix added. With the suffix, they then expect `printf`-like string
format and corresponding additional arguments.

So, for example, instead of the simple checking macros
```C++
TEST_CHECK(a == b);
TEST_ASSERT(x < y);
TEST_EXCEPTION(SomeFunction(), std::exception);
```
we can use their respective counterparts with a custom messages:
```C++
TEST_CHECK_(a == b, "%d is equal to %d", a, b);
TEST_ASSERT_(x < y, "%d is lower than %d", x, y);
TEST_EXCEPTION_(SomeFunction(), std::exception, "SomeFunction() throws std::exception");
```

You should use some neutral wording for them because, with the command line
option `--verbose`, the messages are logged out even if the respective check
passes successfully.

(If you need to output some diagnostic information just in the case the check
fails, use the macro `TEST_MSG`. That's exactly its purpose.)

Similarly, instead of
```C
TEST_CASE("name");
```
we can use richer
```C
TEST_CASE_("iteration #%d", 42);
```

However note all of these can only be used if your compiler supports variadic
preprocessor macros. Variadic macros became a standard part of the C language
with C99.


## Building the Test Suite

When we are done with implementing the tests, we can simply compile it as
a simple C/C++ program. For example, assuming `cc` is your C compiler:

```sh
$ cc test_example.c -o test_example
```


## Running Unit Tests

When the test suite is compiled, the resulted testing binary can be used to run
the tests.

Exit code of the test suite is 0 if all the executed unit tests pass, 1 if any
of them fails, or any other number if an internal error occurs.

By default (without any command line options), it runs all implemented unit
tests. It can also run only subset of the unit tests as specified on the
command line:

```sh
$ ./test_example                # Runs all tests in the suite
$ ./test_example test1 test2    # Runs only tests specified
$ ./test_example --skip test3   # Runs all tests but those specified
```

Note that a single command line argument can select a whole group of test units
because Acutest implements several levels of unit test selection (the 1st one
matching at least one test unit is used):

1. *Exact match*: When the argument matches exactly the whole name of a unit
   test then just the given test is selected.

2. *Word match*: When the argument does not match any complete test name, but
   it does match whole word in one or more test names, then all such tests are
   selected.

   The following characters are recognized as word delimiters: space ` `,
   tabulator `\t`, dash `-`, underscore `_`, slash `/`, dot `.`, comma `,`,
   colon `:`, semicolon `;`.

3. *Substring match*: If even the word match failed to select any test, then
   all tests with a name which contains the argument as its substring are
   selected.

By adopting an appropriate test naming strategy, this allows user to run (or
to skip if `--skip` is used) easily whole family of related tests with a single
command line argument.

For example consider test suite `test_example` which implements tests `foo-1`,
`foo-2`, `foomatic`, `bar-1` and `bar-10`:

```sh
$ ./test_example bar-1   # Runs only the test 'bar-1' (exact match)
$ ./test_example foo     # Runs 'foo-1' and 'foo-2' (word match)
$ ./test_example oo      # Runs 'foo-1', 'foo-2' and 'foomatic' (substring match)
$ ./test_example 1       # Runs 'foo-1' and 'bar-1' (word match)
```

You may use `--list` or `-l` to just list all unit tests implemented by the
given test suite:

```sh
$ ./test_example --list
```

To see description for all the supported command line options, run the binary
with the option `--help`:

```sh
$ ./test_example --help
```


## FAQ

**Q: Wasn't this project known as "CUTest"?**

**A:** Yes. It has been renamed as the original name was found to be
[too much overloaded](https://github.com/mity/cutest/issues/6).


**Q: Do I need to distribute file `README.md` and/or `LICENSE.md`?**

**A:** No. The header `acutest.h` includes URL to our repo, copyright note and
the MIT license terms inside of it. As long as you leave those intact, we are
completely fine if you only add the header into your project. After all,
the simple use and all-in-one-header nature of it is our primary aim.


## License

Acutest is covered with MIT license, see the file `LICENSE.md` or beginning of
`acutest.h` for its full text.


## More Information

The project resides on github:

* https://github.com/mity/acutest

You can find the latest version of Acutest there, contribute with enhancements
or report bugs.
