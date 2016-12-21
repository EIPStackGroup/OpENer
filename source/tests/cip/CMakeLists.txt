
opener_common_includes()

add_subdirectory( connectionmanager )

set( CipTestSrc cipepathtest.cpp cipelectronickeytest.cpp cipconnectionobjecttest.cpp )

include_directories( ${SRC_DIR}/cip )

add_library( CipTest ${CipTestSrc} )

target_link_libraries( CipTest gcov ${CPPUTEST_LIBRARY} ${CPPUTESTEXT_LIBRARY} )
