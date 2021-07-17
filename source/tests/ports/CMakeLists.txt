
#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

set( PortsTestSrc socket_timer_tests.cpp)

include_directories( ${SRC_DIR}/ports )

add_library( PortsTest ${PortsTestSrc} )
