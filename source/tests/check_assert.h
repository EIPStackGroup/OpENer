/*
 * This header contains definitions implementing a method for writing test
 * cases to confirm an OPENER_ASSERTION failure using the CHECK_ASSERT macro.
 * Only code implementing Cpputest test cases should include this header; it
 * should not be included by application code.
 */
#include <setjmp.h>


/* See OpENerTests.cpp for descriptions. */
extern jmp_buf assert_jump;
extern jmp_buf *assert_jump_enabled;


/*
 * This macro is intended to be used in the unit test code, not the
 * application code, to verify a given expression, typically a call to a
 * function being tested, generates an assertion via a failed OPENER_ASSERT
 * condition. For example:
 *
 * CHECK_ASSERT(func());
 *
 * The above statement will pass if an OPENER_ASSERT fails during func(),
 * or cause the test to fail if func() returns normally.
 *
 * These statements are enclosed within a do/while block to keep the if
 * statement isolated from surrounding if statements.
 */
#define CHECK_ASSERT(exp)                                               \
  do {                                                                  \
    /* Enable an expected assertion by storing a non-NULL pointer. */   \
    UT_PTR_SET(assert_jump_enabled, &assert_jump);                      \
                                                                        \
    /* Store the assertion jump location. */                            \
    if (setjmp(assert_jump) == 0) {                                     \
                                                                        \
      /* Code under test, which should longjmp() instead of return. */  \
      exp;                                                              \
                                                                        \
      /* Fail if the above expression did not generate an assertion. */ \
      FAIL("Did not assert as expected.");                              \
    }                                                                   \
  } while (0)
