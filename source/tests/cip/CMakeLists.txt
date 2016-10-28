
opener_common_includes()

set( CipTestSrc cipepathtest.cpp )

include_directories( ${SRC_DIR}/cip )

add_library( CipTest ${CipTestSrc} )
