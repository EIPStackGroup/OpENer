#######################################
# CIP connection manager library      #
#######################################

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

set( CIP_CONNECTION_MANAGER_SRC cipforwardopen.c )

add_library( CIP_CONNECTION_MANAGER ${CIP_CONNECTION_MANAGER_SRC} )