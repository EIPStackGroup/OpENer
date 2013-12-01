#######################################
# Add test subdirectories             #
#######################################

add_test_includes()

add_subdirectory( utils )
add_executable( OpENer_Tests main.cpp )

set( CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CPPUTEST_HOME}/lib )

target_link_libraries( OpENer_Tests CppUTest CppUTestExt UtilsTest Utils )
