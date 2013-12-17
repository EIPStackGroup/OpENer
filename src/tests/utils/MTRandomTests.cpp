#include <CppUTest/TestHarness.h>
#include <stdint.h>

extern "C"  {
  #include "mtrandom.h"
}

TEST_GROUP(MTRandomTests)
{
};

TEST(MTRandomTests, InitParamsTest)
{
	mtSetupRandomParams(10);
	//LONGS_EQUAL(10,);
	//if(0 == sRandomParams.panSeedBlocks) FAIL("Malloc failed to allocate memory!")
}

/*Test not correct*/
IGNORE_TEST(MTRandomTests, SeedTest)
{
	mtSetupRandomParams(10);
	mtSetSeed(0);
	//LONGS_EQUAL(0, sRandomParams.panSeedBlocks[0]);
	/*LONGS_EQUAL(1, naSeedBlocks[1]);*/
}

