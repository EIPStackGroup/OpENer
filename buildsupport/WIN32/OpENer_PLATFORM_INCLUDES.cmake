macro(opener_platform_spec)
  include_directories(${PORTS_SRC_DIR}/${OpENer_PLATFORM} ${PORTS_SRC_DIR}/${OpENer_PLATFORM}/sample_application)
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN32 -Za" )
endmacro(opener_platform_includes)

