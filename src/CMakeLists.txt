#######################################
# Required CMake version              #
#######################################
cmake_minimum_required( VERSION 2.8.9 )

#######################################
# Project name                        #
#######################################
project( OpENer C )

#######################################
# Project version                     #
#######################################
set( OpENer_VERSION_MAJOR 1 )
set( OpENer_VERSION_MINOR 2 )

find_path( OpENer_BUILDSUPPORT_DIR OpENer.cmake ${PROJECT_SOURCE_DIR}/../buildsupport)
 
INCLUDE( ${OpENer_BUILDSUPPORT_DIR}/OpENer.cmake)

#######################################
# Platform switches                   #
#######################################
set( OpENer_KNOWN_PLATFORMS "POSIX" "WIN32" "LM3S8962" )

set( OpENer_PLATFORM CACHE STRINGS "Platform OpENer will be built for" )
set_property(CACHE OpENer_PLATFORM PROPERTY STRINGS ${OpENer_KNOWN_PLATFORMS} )

#######################################
# OpENer tracer switches              #
#######################################
set( OpENer_TRACES OFF CACHE BOOL "Activate OpENer traces" )
if(OpENer_TRACES)
  add_definitions( -DOPENER_WITH_TRACES )
  set( TRACE_LEVEL 0 )
  set( OpENer_TRACE_LEVEL_ERROR ON CACHE BOOL "Error trace level" )
  set( OpENer_TRACE_LEVEL_WARNING ON CACHE BOOL "Warning trace level" )
  set( OpENer_TRACE_LEVEL_STATE ON CACHE BOOL "State trace level" )
  set( OpENer_TRACE_LEVEL_INFO ON CACHE BOOL "Info trace level" )
  
  if(OpENer_TRACE_LEVEL_ERROR)
    math( EXPR TRACE_LEVEL "${TRACE_LEVEL} + 1" )
  endif(OpENer_TRACE_LEVEL_ERROR)
  if(OpENer_TRACE_LEVEL_WARNING)
    math( EXPR TRACE_LEVEL "${TRACE_LEVEL} + 2" )
  endif(OpENer_TRACE_LEVEL_WARNING)
  if(OpENer_TRACE_LEVEL_STATE)
    math( EXPR TRACE_LEVEL "${TRACE_LEVEL} + 4" )
  endif(OpENer_TRACE_LEVEL_STATE)
  if(OpENer_TRACE_LEVEL_INFO)
    math( EXPR TRACE_LEVEL "${TRACE_LEVEL} + 8" )
  endif(OpENer_TRACE_LEVEL_INFO)
  
  add_definitions(-DOPENER_TRACE_LEVEL=${TRACE_LEVEL})
  
endif(OpENer_TRACES)

#######################################
# Test switch                         #
#######################################
set( OpENer_TESTS OFF CACHE BOOL "Enable tests to be built" )
if( OpENer_TESTS )
  enable_language( CXX )
  set( CPPUTEST_HOME "" CACHE PATH "Path to CppUTest directory" )
  INCLUDE( ${OpENer_BUILDSUPPORT_DIR}/OpENer_Tests.cmake )
  add_subdirectory(tests)
endif( OpENer_TESTS )

#######################################
# OpENer C flags                      #
#######################################
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -Wall" )

####################################################
# Internal cache holding the available CIP objects #
####################################################
set( OpENer_ADD_CIP_OBJECTS "" CACHE INTERNAL STRING )

#######################################
# Add custom CIP objects              #
#######################################
set( OpENer_CIP_OBJECTS_DIR ${PROJECT_SOURCE_DIR}/cip_objects )
include(${OpENer_BUILDSUPPORT_DIR}/OpENer_CIP_Object_generator.cmake)

#######################################
# Add subdirectories                  #
#######################################
add_subdirectory(cip)
add_subdirectory(cip_objects)
add_subdirectory(enet_encap)
add_subdirectory(ports)

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platfrom specific things        #
#######################################

opener_platform_support("INCLUDES")
