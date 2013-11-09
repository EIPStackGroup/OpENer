FUNCTION(opener_add_definition)
  FOREACH(ARG ${ARGV})
    set_property(GLOBAL APPEND PROPERTY OPENER_DEFINITION ${ARG})
  ENDFOREACH(ARG)
ENDFUNCTION(opener_add_definition)

macro(opener_platform_support ARGS)

  if(OpENer_PLATFORM STREQUAL "") #Hier Fehler korrigieren!
    message(FATAL_ERROR "No platform selected!")  
  endif(OpENer_PLATFORM STREQUAL "")

  include( ${OpENer_BUILDSUPPORT_DIR}/${OpENer_PLATFORM}/OpENer_PLATFORM_${ARGS}.cmake)
  opener_platform_spec()
endmacro(opener_platform_support ARGS)
 
macro(opener_common_includes)
  #######################################
  # Set source directories              #
  #######################################
  set( SRC_DIR "${PROJECT_SOURCE_DIR}" )
  set( CIP_SRC_DIR "${PROJECT_SOURCE_DIR}/cip" )
  set( ENET_ENCAP_SRC_DIR "${PROJECT_SOURCE_DIR}/enet_encap" )
  set( PORTS_SRC_DIR "${PROJECT_SOURCE_DIR}/ports")

  #######################################
  # Include directories                 #
  #######################################
  include_directories( ${PROJECT_SOURCE_DIR} ${SRC_DIR} ${CIP_SRC_DIR} ${ENET_ENCAP_SRC_DIR} ${PORTS_SRC_DIR} )
endmacro(opener_common_includes)
