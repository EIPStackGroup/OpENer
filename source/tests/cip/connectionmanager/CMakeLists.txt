
opener_common_includes()

set( CipConnectionManagerTestSrc cipforwardopentest.cpp )

include_directories( ${SRC_DIR}/cip ${SRC_DIR}/cip/connectionmanager )

add_library( CipConnectionManagerTest ${CipConnectionManagerTestSrc} )

target_link_libraries( CipConnectionManagerTest gcov ${CPPUTEST_LIBRARY} ${CPPUTESTEXT_LIBRARY} )
