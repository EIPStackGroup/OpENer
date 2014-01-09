FUNCTION(opener_add_definition)
  FOREACH(ARG ${ARGV})
    set_property(GLOBAL APPEND PROPERTY OPENER_DEFINITION ${ARG})
  ENDFOREACH(ARG)
ENDFUNCTION(opener_add_definition)

##############################################
# Adds platform specific include directories #
############################################## 
macro(opener_platform_support ARGS)

  if(OpENer_PLATFORM STREQUAL "")
    message(FATAL_ERROR "No platform selected!")  
  endif(OpENer_PLATFORM STREQUAL "")

  include( ${OpENer_BUILDSUPPORT_DIR}/${OpENer_PLATFORM}/OpENer_PLATFORM_${ARGS}.cmake)
  opener_platform_spec()
endmacro(opener_platform_support ARGS)


#######################################
# Adds common Include directories     #
####################################### 
macro(opener_common_includes)
  set( SRC_DIR "${PROJECT_SOURCE_DIR}/src" )
  set( CIP_SRC_DIR "${SRC_DIR}/cip" )
  set( ENET_ENCAP_SRC_DIR "${SRC_DIR}/enet_encap" )
  set( PORTS_SRC_DIR "${SRC_DIR}/ports")
  set( UTILS_SRC_DIR "${SRC_DIR}/utils")

  include_directories( ${PROJECT_SOURCE_DIR} ${SRC_DIR} ${CIP_SRC_DIR} ${ENET_ENCAP_SRC_DIR} ${PORTS_SRC_DIR} ${UTILS_SRC_DIR} ${OpENer_CIP_OBJECTS_DIR} )
endmacro(opener_common_includes)

MACRO(opener_add_cip_object NAME DESCRIPTION)
  set(OpENer_CIP_OBJECT_${NAME} OFF CACHE BOOL "${DESCRIPTION}")
  FOREACH(dependencies ${ARGN})
	if(NOT ${dependencies})
	return()
	endif(NOT ${dependencies})
  ENDFOREACH(dependencies)
  if(NOT OpENer_CIP_OBJECT_${NAME})
    return()
  endif(NOT OpENer_CIP_OBJECT_${NAME})
ENDMACRO(opener_add_cip_object)
