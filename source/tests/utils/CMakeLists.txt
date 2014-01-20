
opener_common_includes()

set( UtilsTestSrc randomTests.cpp xorshiftrandomtests.cpp)

include_directories( ${SRC_DIR}/utils )

add_library( UtilsTest ${UtilsTestSrc} )
