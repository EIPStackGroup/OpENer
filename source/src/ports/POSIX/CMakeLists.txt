add_subdirectory(sample_application)

set( PLATFORM_SPEC_SRC networkhandler.c opener_error.c)

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

set (PLATFORMLIBNAME ${OpENer_PLATFORM}PLATFORM)

add_library( ${PLATFORMLIBNAME} ${PLATFORM_SPEC_SRC})

add_executable(OpENer main.c)

target_link_libraries( OpENer CIP SAMPLE_APP ENET_ENCAP PLATFORM_GENERIC ${PLATFORMLIBNAME} ${PLATFORM_SPEC_LIBS} ${OpENer_ADD_CIP_OBJECTS})
