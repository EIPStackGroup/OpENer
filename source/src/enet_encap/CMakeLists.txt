#######################################
# Ethernet encapsulation library      #
#######################################

set( ENET_ENCAP_SRC cpf.c encap.c endianconv.c )

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

add_library( ENET_ENCAP ${ENET_ENCAP_SRC} )

target_link_libraries( ENET_ENCAP Utils)