add_subdirectory(sample_application)

set( PLATFORM_SPEC_SRC networkhandler.c opener_error.c networkconfig.c)

#######################################
# OpENer RT patch	                    #
#######################################
set( OpENer_RT OFF CACHE BOOL "Activate OpENer RT" )
if(OpENer_RT)
  set( OpENer_RT_Additional_Stacksize "10240" CACHE STRING "Additional stack size above the defined minimum")
  add_definitions( -DOPENER_RT )
  add_definitions(-DOPENER_RT_THREAD_SIZE=${OpENer_RT_Additional_Stacksize})
endif(OpENer_RT)

#######################################
# AFL Fuzzing                         #
#######################################
if(USE_FUZZ_AFL)
  add_definitions( -DFUZZING_AFL )
endif(USE_FUZZ_AFL)

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

add_library( ${OpENer_PLATFORM}PLATFORM ${PLATFORM_SPEC_SRC})
# Mark executables and/or libraries for installation

if( OPENER_INSTALL_AS_LIB )
  install(TARGETS ${OpENer_PLATFORM}PLATFORM
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(DIRECTORY ${PORTS_SRC_DIR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    USE_SOURCE_PERMISSIONS
    FILES_MATCHING
      PATTERN "*.h"
      PATTERN "sample_application" EXCLUDE
   )
endif()

if( NOT OpENer_TESTS)
  add_executable(OpENer main.c)
  target_link_libraries( OpENer PLATFORM_GENERIC ${OpENer_PLATFORM}PLATFORM ${PLATFORM_SPEC_LIBS} CIP Utils SAMPLE_APP ENET_ENCAP NVDATA rt cap pthread)

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
endif()

if(NOT ${CMAKE_INSTALL_BINDIR} STREQUAL "")
  install (PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/OpENer DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
endif()
