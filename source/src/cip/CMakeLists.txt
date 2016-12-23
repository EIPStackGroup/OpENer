#######################################
# CIP library                         #
#######################################

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

add_subdirectory( connectionmanager )

set( CIP_SRC appcontype.c cipassembly.c cipclass3connection.c cipcommon.c cipconnectionmanager.c ciperror.h cipethernetlink.c cipidentity.c cipioconnection.c cipmessagerouter.c ciptcpipinterface.c ciptypes.h cipepath.h cipepath.c cipelectronickey.h cipelectronickey.c )

add_library( CIP ${CIP_SRC} )
target_link_libraries( CIP CIP_CONNECTION_MANAGER )
