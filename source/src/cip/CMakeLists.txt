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

set( CIP_SRC appcontype.c cipassembly.c cipclass3connection.c cipcommon.c cipconnectionobject.c cipconnectionmanager.c ciperror.h cipethernetlink.c cipidentity.c cipioconnection.c cipmessagerouter.c ciptcpipinterface.c ciptypes.h cipepath.h cipepath.c cipelectronickey.h cipelectronickey.c cipqos.c )

add_library( CIP ${CIP_SRC} )
