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
	MTRandomParams sRandomParams;
	mtsSetupRandomParams(10, &sRandomParams);
	LONGS_EQUAL(10, sRandomParams.nBlockSize);
	if(0 == sRandomParams.panSeedBlocks) FAIL("Malloc failed to allocate memory!")
}

/*Test not correct*/
IGNORE_TEST(MTRandomTests, SeedTest)
{
	MTRandomParams sRandomParams; /*Parameters for the Maressene Twister algorithm*/
	mtsSetupRandomParams(10, &sRandomParams);
	mtsRand(0, &sRandomParams);
	LONGS_EQUAL(0, sRandomParams.panSeedBlocks[0]);
	/*LONGS_EQUAL(1, naSeedBlocks[1]);*/
}

