#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform specific things        #
#######################################

opener_platform_support("INCLUDES")

opener_platform_support("INCLUDES")

if (OPENER_IS_DLR_DEVICE)
  set( SAMPLE_APP_DLR_ONLY_SRC ethlinkcbs.c )
endif()

add_library(SAMPLE_APP sampleapplication.c ${SAMPLE_APP_DLR_ONLY_SRC})
