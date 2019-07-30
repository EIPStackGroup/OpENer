#!/bin/bash

echo 'Windows main Script started...'
# Exit with nonzero exit code if anything fails
set -e

cd $TRAVIS_BUILD_DIR/source
cmake -DOpENer_PLATFORM:STRING="WIN32" -DCMAKE_BUILD_TYPE:STRING="Debug" -DOpENer_64_BIT_DATA_TYPES_ENABLED:BOOL=ON .
"c:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\MSBuild\15.0\Bin\MSBuild.exe" OpENer.sln //p:Configuration=Release //p:Platform="Win32"