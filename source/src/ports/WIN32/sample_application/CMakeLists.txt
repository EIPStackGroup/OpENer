#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform specific things        #
#######################################

opener_platform_support("INCLUDES")

opener_platform_support("INCLUDES")

add_library(SAMPLE_APP sampleapplication.c)
