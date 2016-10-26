
opener_common_includes()

set( CipTestSrc connectionmanagertest.cpp )

include_directories( ${SRC_DIR}/cip )

add_library( CipTest ${CipTestSrc} )
