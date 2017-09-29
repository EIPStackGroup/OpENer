#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

set( CipTestSrc cipepathtest.cpp cipelectronickeytest.cpp cipconnectionmanagertest.cpp )

include_directories( ${SRC_DIR}/cip )

add_library( CipTest ${CipTestSrc} )

message("${PLATFORMLIBNAME}")
target_link_libraries( CipTest CIP ENET_ENCAP PLATFORM_GENERIC SAMPLE_APP ${PLATFORMLIBNAME} ${PLATFORM_SPEC_LIBS} )

target_link_libraries( CipTest gcov ${CPPUTEST_LIBRARY} ${CPPUTESTEXT_LIBRARY} )
