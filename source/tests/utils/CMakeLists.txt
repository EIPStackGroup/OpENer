
opener_common_includes()

set( UtilsTestSrc randomTests.cpp xorshiftrandomtests.cpp doublylinkedlistTests.cpp)

include_directories( ${SRC_DIR}/utils )

add_library( UtilsTest ${UtilsTestSrc} )

target_link_libraries( UtilsTest gcov ${CPPUTEST_LIBRARY} ${CPPUTESTEXT_LIBRARY} )
