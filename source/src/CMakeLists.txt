#######################################
# Add subdirectories                  #
#######################################
add_subdirectory( enet_encap )
add_subdirectory( cip )
add_subdirectory( cip_objects )
add_subdirectory( ports )
add_subdirectory( utils )

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform specific things        #
#######################################

opener_platform_support( "INCLUDES" )

if( OPENER_INSTALL_AS_LIB )
  install(DIRECTORY .
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
   )
endif()
