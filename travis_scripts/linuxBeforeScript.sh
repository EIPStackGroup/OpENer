#!/bin/bash

echo 'Linux Before Script started...'
# Exit with nonzero exit code if anything fails
set -e

cd $TRAVIS_BUILD_DIR/source
chmod +x $TRAVIS_BUILD_DIR/travis_scripts/installCppUTestDependency.sh
$TRAVIS_BUILD_DIR/travis_scripts/installCppUTestDependency.sh