add_subdirectory(sample_application)

set( PLATFORM_SPEC_SRC networkhandler.c)

set( SRC_DIR "${PROJECT_SOURCE_DIR}/src" )
set( CIP_SRC_DIR "${PROJECT_SOURCE_DIR}/src/cip" )
set( ENET_ENCAP_SRC_DIR "${PROJECT_SOURCE_DIR}/src/enet_encap" )
set( PORTS_SRC_DIR "${PROJECT_SOURCE_DIR}/src/ports")

include_directories( ${PROJECT_SOURCE_DIR} ${SRC_DIR} ${CIP_SRC_DIR} ${ENET_ENCAP_SRC_DIR} ${PORTS_SRC_DIR} )

opener_platform_support("INCLUDES")

add_library( PCPLATFORM ${PLATFORM_SPEC_SRC}) 

add_executable(OpENer main.c)

target_link_libraries( OpENer PCPLATFORM CIP SAMPLE_APP ENET_ENCAP ${PLATFORM_SPEC_LIBS} )
