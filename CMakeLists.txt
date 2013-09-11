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
set( PLATFORM_PC ON CACHE BOOL "Platform for OpENer is PC" )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPENER_WITH_TRACES -DOPENER_TRACE_LEVEL=0xFF -W -Wall -ansi -std=c99" )

#######################################
# Add subdirectories                  #
#######################################
add_subdirectory(src)

set( SRC_DIR "${PROJECT_SOURCE_DIR}/src" )
set( CIP_SRC_DIR "${PROJECT_SOURCE_DIR}/src/cip" )
set( ENET_ENCAP_SRC_DIR "${PROJECT_SOURCE_DIR}/src/enet_encap" )
set( PORTS_SRC_DIR "${PROJECT_SOURCE_DIR}/src/ports")

include_directories( ${PROJECT_SOURCE_DIR} ${SRC_DIR} ${CIP_SRC_DIR} ${ENET_ENCAP_SRC_DIR} ${PORTS_SRC_DIR} )

if(PLATFORM_PC)
  include_directories(${PORTS_SRC_DIR}/platform-pc ${PORTS_SRC_DIR}/platform-pc/sample_application)
else(PLATFORM_PC)
  message(FATAL_ERROR "No platform selected!")
endif(PLATFORM_PC)