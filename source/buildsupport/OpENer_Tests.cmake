#######################################
# Adds test includes                  #
#######################################
macro( add_test_includes )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage -include ${CPPUTEST_HOME}/include/CppUTest/MemoryLeakDetectorNewMacros.h" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage -include ${CPPUTEST_HOME}/include/CppUTest/MemoryLeakDetectorMallocMacros.h" )
  include_directories( ${CPPUTEST_HOME}/include )
endmacro( add_test_includes )
