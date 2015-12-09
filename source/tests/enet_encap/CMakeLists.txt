
opener_common_includes()

set( EthernetEncapsulationTestSrc endianconvtest.cpp )

include_directories( ${SRC_DIR}/enet_encap )

add_library( EthernetEncapsulationTest ${EthernetEncapsulationTestSrc} )
