#######################################
# OpENer ports			              #
#######################################

add_subdirectory( ${OpENer_PLATFORM} )
add_subdirectory( nvdata )

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

set( PLATFORM_GENERIC_SRC generic_networkhandler.c socket_timer.c )

add_library( PLATFORM_GENERIC ${PLATFORM_GENERIC_SRC} )

if( OPENER_INSTALL_AS_LIB )
  install(TARGETS PLATFORM_GENERIC
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(DIRECTORY ${PORTS_SRC_DIR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    USE_SOURCE_PERMISSIONS
    FILES_MATCHING
      PATTERN "*.h"
      PATTERN "sample_application" EXCLUDE
   )
endif()

