add_subdirectory(sample_application)

set( PLATFORM_SPEC_SRC networkhandler.c opener_error.c networkconfig.c)

#######################################
# OpENer RT patch	              #
#######################################
set( OpENer_RT OFF CACHE BOOL "Activate OpENer RT" )
if(OpENer_RT)
  set( OpENer_RT_Additional_Stacksize "10240" CACHE STRING "Additional stack size above the defined minimum")
  add_definitions( -DOPENER_RT )
  add_definitions(-DOPENER_RT_THREAD_SIZE=${OpENer_RT_Additional_Stacksize})
endif(OpENer_RT)

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

add_library( ${OpENer_PLATFORM}PLATFORM ${PLATFORM_SPEC_SRC})

add_executable(OpENer main.c)

target_link_libraries( OpENer CIP Utils SAMPLE_APP ENET_ENCAP PLATFORM_GENERIC ${OpENer_PLATFORM}PLATFORM ${PLATFORM_SPEC_LIBS} ${OpENer_ADD_CIP_OBJECTS} rt cap pthread)
