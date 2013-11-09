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
 
macro(opener_general_includes)
  
endmacro(opener_general_includes)