macro(opener_platform_spec)
  include_directories(${PORTS_SRC_DIR}/${OpENer_PLATFORM} ${PORTS_SRC_DIR}/${OpENer_PLATFORM}/sample_application )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DRESTRICT=__restrict -DWIN32 -D_WIN32_WINNT=_WIN32_WINNT_VISTA" )
endmacro(opener_platform_spec)

