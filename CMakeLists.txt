#######################################
# Required CMake version              #
#######################################
cmake_minimum_required(VERSION 2.8.9)

#######################################
# Project name                        #
#######################################
project(OpENer)

#######################################
# Project-wide switches               #
#######################################
set( PLATFORM_POSIX OFF CACHE BOOL "Platform for OpENer is POSIX" )
set( PLATFORM_WIN32 OFF CACHE BOOL "Platform for OpENer is WIN32" )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPENER_WITH_TRACES -DOPENER_TRACE_LEVEL=0xFF -w -Wall -ansi -std=c99" )

#######################################
# Add subdirectories                  #
#######################################
add_subdirectory(src)

set( SRC_DIR "${PROJECT_SOURCE_DIR}/src" )
set( CIP_SRC_DIR "${PROJECT_SOURCE_DIR}/src/cip" )
set( ENET_ENCAP_SRC_DIR "${PROJECT_SOURCE_DIR}/src/enet_encap" )
set( PORTS_SRC_DIR "${PROJECT_SOURCE_DIR}/src/ports")

include_directories( ${PROJECT_SOURCE_DIR} ${SRC_DIR} ${CIP_SRC_DIR} ${ENET_ENCAP_SRC_DIR} ${PORTS_SRC_DIR} )

if(PLATFORM_POSIX)
  include_directories(${PORTS_SRC_DIR}/platform-pc ${PORTS_SRC_DIR}/platform-pc/sample_application)
elseif(PLATFORM_WIN32)
  set( MSINTTYPES_DIR CACHE PATH "Location if inttypes for WIN32 platform needed" )
  include_directories(${PORTS_SRC_DIR}/platform-pc ${PORTS_SRC_DIR}/platform-pc/sample_application ${MSINTTYPES_DIR})
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN32" )
else(PLATFORM_POSIX)
  message(FATAL_ERROR "No platform selected!")
endif(PLATFORM_POSIX)
