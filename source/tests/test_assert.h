/*
 * This header defines an implementation of the OPENER_ASSERT macro that
 * can be used with Cpputest unit tests to confirm an assertion fails
 * under given conditions. It is conditionally included in the application
 * code when unit tests are enabled through the Cmake configuration; it should
 * not be included in the Cpputest unit test code.
 *
 * The intent is to create an assertion implementation that both immediately
 * stops execution after an assertion failure, as should normally be the
 * case, and is detectable from the unit test code. This is accomplished via
 * setjmp() and longjmp(), approximating the behavior of a C++ exception,
 * where an OPENER_ASSERTION failure results in a longjmp() back to the
 * unit test code, which then verifies that an assertion failure occurred.
 */
#ifndef OPENER_TEST_ASSERT_H
#define OPENER_TEST_ASSERT_H


/*
 * Define the OPENER_ASSERT macro to call the unit test assertion verification
 * function. The surrounding do/while loop serves to insulate the if statement
 * from any surrounding if statements.
 */
#define OPENER_ASSERT(assertion) \
  do {if ( !(assertion) ) test_assert_fail(__FILE__, __LINE__);} while (0)


/* Function Prototypes */
extern void test_assert_fail(const char *const file,
                             const unsigned int line);


#endif /* OPENER_TEST_ASSERT_H */
