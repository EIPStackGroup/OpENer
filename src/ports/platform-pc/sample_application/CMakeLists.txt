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

add_library(SAMPLE_LIB sampleapplication.c)