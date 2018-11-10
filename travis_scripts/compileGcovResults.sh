#!/usr/bin/env sh

# Delete old gcov folder
rm -rf gcov_results

# Create new gcov folder
mkdir gcov_results
# Change into results folder
cd gcov_results
# Get all object files
OBJECTS=$(find ../src -iname "*.o")
# Process all files
for o in $OBJECTS; 
do 
  gcov -abcfu $o; 
done
