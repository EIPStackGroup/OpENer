
opener_common_includes()
opener_platform_spec()

set( UTILS_SRC random.c xorshiftrandom.c doublylinkedlist.c  enipmessage.c)

add_library( Utils ${UTILS_SRC} )

if( BUILD_SHARED_LIBS )
  install(TARGETS Utils
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION lib
  )
  install(DIRECTORY ${UTILS_SRC_DIR}
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
   )
endif( BUILD_SHARED_LIBS )

