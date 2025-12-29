#######################################
# Add test subdirectories             #
#######################################

#######################################
# Add test includes                   #
#######################################
add_test_includes()


#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

###################################################
# Copy custom test output file to binary location #
###################################################
#configure_file( CTestCustom.cmake ${PROJECT_BINARY_DIR}/CTestCustom.cmake )

add_subdirectory( cip )
add_subdirectory( ports )
add_subdirectory( enet_encap )
add_subdirectory( utils )

add_executable( OpENer_Tests OpENerTests.cpp callback_mock.cpp security_tests.cpp)

find_library ( CPPUTEST_LIBRARY CppUTest ${CPPUTEST_HOME}/cpputest_build/lib )
find_library ( CPPUTESTEXT_LIBRARY CppUTestExt ${CPPUTEST_HOME}/cpputest_build/lib )

# When using AddressSanitizer, CppUTest's memory leak detection is not needed
# since ASAN provides more comprehensive memory safety checking
if(NOT (ENABLE_ADDRESS_SANITIZER OR ENABLE_UNDEFINED_SANITIZER))
  # CppUTest memory leak detection is enabled when sanitizers are OFF
else()
  # Suppress CppUTest's built-in memory checking to avoid conflicts
  add_compile_definitions(CPPUTEST_MEMORY_LEAK_DETECTION_DISABLED)
  message(STATUS "CppUTest memory leak detection disabled (using AddressSanitizer)")
endif()

target_link_libraries( OpENer_Tests rt )

target_link_libraries( OpENer_Tests gcov ${CPPUTEST_LIBRARY} ${CPPUTESTEXT_LIBRARY} )
target_link_libraries( OpENer_Tests UtilsTest Utils )
target_link_libraries( OpENer_Tests EthernetEncapsulationTest ENET_ENCAP )
target_link_libraries( OpENer_Tests CipTest CIP )
target_link_libraries( OpENer_Tests PortsTest PLATFORM_GENERIC )
target_link_libraries( OpENer_Tests NVDATA )

########################################
# Adds test to CTest environment       #
########################################
add_test(OpENer_Tests ${EXECUTABLE_OUTPUT_PATH}/OpENer_Tests -v -c)
