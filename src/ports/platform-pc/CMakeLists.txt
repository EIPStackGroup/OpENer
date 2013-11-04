add_subdirectory(sample_application)

set( PLATFORM_SPEC_SRC networkhandler.c)

set( SRC_DIR "${PROJECT_SOURCE_DIR}/src" )
set( CIP_SRC_DIR "${PROJECT_SOURCE_DIR}/src/cip" )
set( ENET_ENCAP_SRC_DIR "${PROJECT_SOURCE_DIR}/src/enet_encap" )
set( PORTS_SRC_DIR "${PROJECT_SOURCE_DIR}/src/ports")

include_directories( ${PROJECT_SOURCE_DIR} ${SRC_DIR} ${CIP_SRC_DIR} ${ENET_ENCAP_SRC_DIR} ${PORTS_SRC_DIR} )

if(PLATFORM_POSIX)
  include_directories(${PORTS_SRC_DIR}/platform-pc ${PORTS_SRC_DIR}/platform-pc/sample_application)
elseif(PLATFORM_WIN32)
  include_directories(${PORTS_SRC_DIR}/platform-pc ${PORTS_SRC_DIR}/platform-pc/sample_application ${MSINTTYPES_INCLUDES})
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN32" )
  set( PLATFORM_SPEC_LIBS ws2_32)
else(PLATFORM_POSIX)
  message(FATAL_ERROR "No platform selected!")
endif(PLATFORM_POSIX)

add_library( PCPLATFORM_LIB ${PLATFORM_SPEC_SRC}) 

add_executable(OpENer main.c)

target_link_libraries( OpENer PCPLATFORM_LIB CIP_LIB SAMPLE_LIB ENET_ENCAP_LIB ${PLATFORM_SPEC_LIBS} )
