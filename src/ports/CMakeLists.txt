#######################################
# OpENer ports					      #
#######################################

if(PLATFORM_PC)
  add_subdirectory(platform-pc)
  include_directories(platform-pc)
else(PLATFORM_PC)
  message(FATAL_ERROR "No platform selected!")
endif(PLATFORM_PC)