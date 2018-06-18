#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

set( EthernetEncapsulationTestSrc endianconvtest.cpp encaptest.cpp)

include_directories( ${SRC_DIR}/enet_encap )

add_library( EthernetEncapsulationTest ${EthernetEncapsulationTestSrc} )
