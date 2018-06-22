#######################################
# Add subdirectories                  #
#######################################
add_subdirectory( cip )
add_subdirectory( cip_objects )
add_subdirectory( enet_encap )
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
