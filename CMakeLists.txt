######################################################
#                                                    #
# Copyright (c) 2020, Rockwell Automation, Inc.      #
# All rights reserved.                               #
#                                                    #
######################################################

project(CIP_SECURITY_OBJECTS LANGUAGES C VERSION 3.0)
#############################################################
# CIP Security objects (class 0x5D, class 0x5E, class 0x5F) #
#############################################################
opener_add_cip_object( ${PROJECT_NAME} "CIP Security Object (class 0x5D), EtherNet/IP Security Object (class 0x5E), Certificate Management Object (class 0x5F)")

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

##################################
# Set source for library         #
##################################
set( ${PROJECT_NAME}_SOURCE CertificateManagementObject/certificatemanagement.c CipSecurityObject/cipsecurity.c EtherNetIPSecurityObject/ethernetipsecurity.c)

include_directories(${${PROJECT_NAME}_SOURCE_DIR})

##################################
# Create library		         #
##################################
add_library( ${PROJECT_NAME} ${${PROJECT_NAME}_SOURCE})

##################################
# Add object to includes         #
##################################
string(STRIP "${OpENer_ADD_CIP_OBJECTS} ${PROJECT_NAME}" OpENer_TRIMMED_ADD_CIP_OBJECTS)
set(OpENer_ADD_CIP_OBJECTS "${OpENer_TRIMMED_ADD_CIP_OBJECTS}" CACHE INTERNAL STRING )

target_link_libraries( ${PROJECT_NAME} CIP )
