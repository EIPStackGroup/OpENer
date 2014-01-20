add_subdirectory(sample_application)

#######################################
# Platform specific options           #
#######################################
set( USE_STATIC_IP OFF CACHE BOOL "Use the static IP defined in main.c" )

if( USE_STATIC_IP )
  add_definitions( -DuseStaticIP=1 )
else( USE_STATIC_IP )
  add_definitions( -DuseStaticIP=0 )
endif( USE_STATIC_IP )

set( NEWLIB_SRC "" CACHE PATH "Path to NewLib source directory" )

set( PLATFORM_SPEC_SRC networkhandler.c basiccip.c flashmgr.c stubs.c startup_gcc.c )
set( SBRK_SRC ${NEWLIB_SRC}/libgloss/libnosys/sbrk.c)

add_library(sbrk ${SBRK_SRC} )

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

include_directories(${CIP_ELECTRICAL_ENERGY_INCLUDE})
set (PLATFORMLIBNAME ${OpENer_PLATFORM}PLATFORM)

link_directories( ${LM3S8962_CONTRIB_DIR}/driverlib/gcc-cm3 )

add_library( ${PLATFORMLIBNAME} ${PLATFORM_SPEC_SRC} )

add_executable( OpENer ${LM3S8962_CONTRIB_DIR}/utils/lwiplib.c )

target_link_libraries( OpENer SAMPLE_APP CIP ENET_ENCAP Utils ${PLATFORMLIBNAME} ${PLATFORM_SPEC_LIBS} )
target_link_libraries( OpENer ${OpENer_ADD_CIP_OBJECTS} ${PLATFORM_LINKER_FLAGS} driver-cm3 m gcc c sbrk "-T ${CMAKE_CURRENT_SOURCE_DIR}/stellaris.ld")
