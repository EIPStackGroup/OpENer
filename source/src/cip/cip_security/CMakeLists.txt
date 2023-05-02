#######################################
# CIP Security library                         #
#######################################

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

set( CIP_SECURITY_SRC cipsecurity.c certificatemanagement.c ethernetipsecurity.c)

add_library( CIP_SECURITY ${CIP_SECURITY_SRC})
target_link_libraries(CIP_SECURITY CIP_FILE_OBJECT)