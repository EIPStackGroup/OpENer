add_subdirectory(sample_application)

set( PLATFORM_SPEC_SRC networkhandler.c opener_error.c networkconfig.c)

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform specific things        #
#######################################
opener_platform_support("INCLUDES")

set (PLATFORMLIBNAME ${OpENer_PLATFORM}PLATFORM)

add_library( ${PLATFORMLIBNAME} ${PLATFORM_SPEC_SRC})

add_executable(OpENer main.c)

target_link_libraries( OpENer PLATFORM_GENERIC ${PLATFORMLIBNAME} CIP Utils  SAMPLE_APP ENET_ENCAP NVDATA ws2_32 Iphlpapi ${OpENer_CIP_OBJECTS} )

# Add additional CIP Objects
string(COMPARE NOTEQUAL "${OpENer_ADD_CIP_OBJECTS}" "" OpENer_HAS_ADDITIONAL_OBJECT )
if( OpENer_HAS_ADDITIONAL_OBJECT )
  message(STATUS "Additional activated objects: ${OpENer_ADD_CIP_OBJECTS}")
  string(REPLACE " " ";" OpENer_ADD_CIP_OBJECTS_LIST ${OpENer_ADD_CIP_OBJECTS} )
  foreach(CIP_OBJECT IN LISTS OpENer_ADD_CIP_OBJECTS_LIST)
    include_directories(${${CIP_OBJECT}_SOURCE_DIR})
    target_link_libraries( OpENer ${CIP_OBJECT} )
  endforeach()
else()
  message(STATUS "No additional activated objects")
endif()
