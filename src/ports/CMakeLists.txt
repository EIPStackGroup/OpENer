#######################################
# OpENer ports			      #
#######################################

if(PLATFORM_POSIX)
  add_subdirectory(platform-pc)
else(PLATFORM_POSIX)
  message(FATAL_ERROR "No platform selected!")
endif(PLATFORM_POSIX)


