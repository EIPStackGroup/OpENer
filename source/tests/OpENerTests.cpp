#include <setjmp.h>
#include <stdexcept>
#include <stdio.h>

#include "OpENerTests.h"
#include "CppUTest/TestRegistry.h"
#include "CppUTestExt/MockSupportPlugin.h"

extern "C" {
#include "endianconv.h"
}


/*
 * Stores the location in the unit test function where execution should jump
 * to upon a failed assertion.
 */
jmp_buf assert_jump;


/*
 * This pointer is used to indicate if an assertion is expected in the code
 * being tested. A non-NULL value means an assertion is expected, and the
 * resulting longjmp() target has been stored in the assert_jmp variable.
 * The actual address stored here is meaningless, aside from being NULL or not;
 * this pointer is never dereferenced. A pointer is used instead of a boolean
 * so the SetPointerPlugin can automatically reset it after every test.
 */
jmp_buf *assert_jump_enabled;


int main(int argc,
         char **argv) {
  /* These checks are here to make sure assertions outside test runs don't crash */
  CHECK(true);
  LONGS_EQUAL(1, 1);

  DetermineEndianess();

  TestRegistry* reg = TestRegistry::getCurrentRegistry();

  MockSupportPlugin mockPlugin;
  reg->installPlugin(&mockPlugin);

  /*
   * Enable the Cpputest SetPointerPlugin to automatically reset the
   * assert_jump_enabled pointer after each test.
   */
  SetPointerPlugin assert_restore("AssertJumpRestore");
  reg->installPlugin(&assert_restore);

  return CommandLineTestRunner::RunAllTests(argc, argv);
}


/*
 * This is the function called by the OPENER_ASSERT macro if the assertion
 * condition fails. It will interrupt the code under test in one of two ways
 * depending on if an assertion is expected from the CHECK_ASSERT test macro.
 *
 * Arguments:
 *
 * file - Path to the source file where the assertion failed.
 * line - Line number identifying the failed assertion.
 */
extern "C" void test_assert_fail(const char *const file,
                                 const unsigned int line) {
  /*
   * Throw an exception with the assertion location if an assertion is not
   * expected. Unfortunately, this will stop all further tests and does not
   * identify the test that failed.
   */
  if (assert_jump_enabled == NULL) {
    const char format[] = "Assertion failure: %s:%d";
    char dummy;

    /* Determine how long the exception message would be. */
    int len_no_null = snprintf(&dummy, 1, format, file, line);

    if (len_no_null > 0) {
      /*
       * Allocate memory for the exception message, including the NULL
       * terminator. This memory is not freed because the forthcoming
       * exception terminates everything anyway.
       */
      const size_t len_with_null = len_no_null + 1;
      char *msg = (char *)malloc(len_with_null);

      if (msg != NULL) {
        len_no_null = snprintf(msg, len_with_null, format, file, line);

        if (len_no_null > 0) {
          throw std::runtime_error(msg);
        }
      }
    }

    /* Throw a generic exception if string generation fails. */
    throw std::runtime_error("Assertion failure.");
  }

  /*
   * Execute the jump back to the unit test function if an assertion was
   * expected.
   */
  else {
    longjmp(assert_jump, 0);
  }
}
