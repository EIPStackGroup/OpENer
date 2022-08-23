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

if( OPENER_INSTALL_AS_LIB )
  install(TARGETS ENET_ENCAP
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(DIRECTORY ${ENET_ENCAP_SRC_DIR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
   )
endif()

target_link_libraries( ENET_ENCAP Utils)
