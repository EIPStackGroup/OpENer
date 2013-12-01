
opener_common_includes()

set( UtilsTestSrc MTRandomTests.cpp xorshiftrandomtests.cpp)

include_directories( ${PROJECT_SOURCE_DIR}/utils )

add_library( UtilsTest ${UtilsTestSrc} )
