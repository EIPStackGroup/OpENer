#!/bin/bash

echo 'Linux main Script started...'
# Exit with nonzero exit code if anything fails
set -e

cd $TRAVIS_BUILD_DIR/source
cmake -DOpENer_PLATFORM:STRING="POSIX" -DCMAKE_BUILD_TYPE:STRING="Debug" \
  -DOpENer_TESTS:BOOL=ON -DCPPUTEST_HOME:PATH=$TRAVIS_BUILD_DIR/source/dependencies/cpputest \
  -DCPPUTEST_LIBRARY:FILEPATH=$TRAVIS_BUILD_DIR/source/dependencies/cpputest/src/CppUTest/libCppUTest.a \
  -DCPPUTESTEXT_LIBRARY:FILEPATH=$TRAVIS_BUILD_DIR/source/dependencies/cpputest/src/CppUTestExt/libCppUTestExt.a .
build-wrapper-linux-x86-64 --out-dir bw-output make all
ctest --output-on-failure
make OpENer_coverage
chmod +x $TRAVIS_BUILD_DIR/travis_scripts/compileGcovResults.sh
$TRAVIS_BUILD_DIR/travis_scripts/compileGcovResults.sh
sonar-scanner -Dproject.settings=$TRAVIS_BUILD_DIR/sonar-project.properties -Dsonar.sources=. -Dsonar.exclusions=OpENer_coverage/**,dependencies/**,CMakeFiles/** -Dsonar.cfamily.gcov.reportsPath=./gcov_results -Dsonar.coverage.exclusions=tests/*,src/ports/**/sample_application/*,tests/**/*
