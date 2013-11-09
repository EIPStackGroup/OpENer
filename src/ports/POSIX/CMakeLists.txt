add_subdirectory(sample_application)

set( PLATFORM_SPEC_SRC networkhandler.c)

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

add_library( PCPLATFORM ${PLATFORM_SPEC_SRC}) 

add_executable(OpENer main.c)

target_link_libraries( OpENer PCPLATFORM CIP SAMPLE_APP ENET_ENCAP ${PLATFORM_SPEC_LIBS} )
