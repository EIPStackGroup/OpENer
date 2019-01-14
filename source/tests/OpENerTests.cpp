#include "OpENerTests.h"

extern "C" {
#include "endianconv.h"
}

int main(int argc,
         char **argv) {
  /* These checks are here to make sure assertions outside test runs don't crash */
  CHECK(true);
  LONGS_EQUAL(1, 1);

  DetermineEndianess();

  return CommandLineTestRunner::RunAllTests(argc, argv);
}
