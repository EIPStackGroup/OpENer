
opener_common_includes()
opener_platform_spec()

set( UTILS_SRC random.c xorshiftrandom.c doublylinkedlist.c  enipmessage.c)

add_library( Utils ${UTILS_SRC} )

if( OPENER_INSTALL_AS_LIB )
  install(TARGETS Utils
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(DIRECTORY ${UTILS_SRC_DIR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
   )
endif()

