#!/bin/sh

echo 'Setting up the CppUTest script...'
# Exit with nonzero exit code if anything fails
set -e

mkdir dependencies
cd dependencies

git clone https://github.com/cpputest/cpputest.git

cd cpputest
git checkout v3.8
cmake CMakeLists.txt
make
cd ../..
