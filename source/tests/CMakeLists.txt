#######################################
# Add test subdirectories             #
#######################################

#######################################
# Add test includes                   #
#######################################
add_test_includes()


###################################################
# Copy custom test output file to binary location #
###################################################
configure_file( CTestCustom.cmake ${PROJECT_BINARY_DIR}/CTestCustom.cmake )

add_subdirectory( utils )
add_executable( OpENer_Tests OpENerTests.cpp )

set( CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CPPUTEST_HOME}/lib )

target_link_libraries( OpENer_Tests CppUTest CppUTestExt )
target_link_libraries( OpENer_Tests UtilsTest Utils )

########################################
# Adds test to CTest environment       #
########################################
add_test(OpENer_Tests ${EXECUTABLE_OUTPUT_PATH}/OpENer_Tests)
