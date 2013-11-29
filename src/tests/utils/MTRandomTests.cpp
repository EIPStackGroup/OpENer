#include <CppUTest/TestHarness.h>

TEST_GROUP(MTRandomTests)
{
};

TEST(MTRandomTests, SeedTest)
{
  mtsrand(0);
}

