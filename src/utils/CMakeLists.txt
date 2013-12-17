
opener_common_includes()
opener_platform_spec()

set( UTILS_SRC random.c mtrandom.c xorshiftrandom.c)

add_library( Utils ${UTILS_SRC} )