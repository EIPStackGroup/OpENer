#######################################
# Non Volatile data storage library   #
#######################################

set( NVDATA_SRC nvdata.c conffile.c nvqos.c nvtcpip.c )

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

add_library( NVDATA ${NVDATA_SRC} )

if( OPENER_INSTALL_AS_LIB )
  install(TARGETS NVDATA
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(DIRECTORY ${NVDATA_SRC_DIR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
   )
endif()

