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

  include( ${OpENer_BUILDSUPPORT_DIR}/${OpENer_PLATFORM}/OpENer_PLATFORM_INCLUDES.cmake)
  opener_platform_spec()
endmacro(opener_platform_support ARGS)


#######################################
# Adds common Include directories     #
#######################################
macro(opener_common_includes)
  set( SRC_DIR "${OpENer_SOURCE_DIR}/src" )
  set( API_SRC_DIR "${SRC_DIR}/api" )
  set( CORE_SRC_DIR "${SRC_DIR}/core" )
  set( CIP_SRC_DIR "${SRC_DIR}/cip" )
  set( ENET_ENCAP_SRC_DIR "${SRC_DIR}/enet_encap" )
  set( PORTS_SRC_DIR "${SRC_DIR}/ports")
  set( NVDATA_SRC_DIR "${SRC_DIR}/ports/nvdata")
  set( UTILS_SRC_DIR "${SRC_DIR}/utils")

  include_directories( ${PROJECT_SOURCE_DIR} ${SRC_DIR} ${API_SRC_DIR} ${CORE_SRC_DIR} ${CIP_SRC_DIR} ${CIP_CONNETION_MANAGER_SRC_DIR} ${ENET_ENCAP_SRC_DIR} ${PORTS_SRC_DIR} ${UTILS_SRC_DIR} ${OpENer_CIP_OBJECTS_DIR} ${NVDATA_SRC_DIR} )
  include_directories( "${PROJECT_BINARY_DIR}/src/ports" )
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

#######################################
# Creates options for trace level     #
#######################################
macro(createTraceLevelOptions)
  add_definitions( -DOPENER_WITH_TRACES )
  set( TRACE_LEVEL 0 )
  set( OpENer_TRACE_LEVEL_ERROR ON CACHE BOOL "Error trace level" )
  set( OpENer_TRACE_LEVEL_WARNING ON CACHE BOOL "Warning trace level" )
  set( OpENer_TRACE_LEVEL_STATE ON CACHE BOOL "State trace level" )
  set( OpENer_TRACE_LEVEL_INFO ON CACHE BOOL "Info trace level" )

  if(OpENer_TRACE_LEVEL_ERROR)
    math( EXPR TRACE_LEVEL "${TRACE_LEVEL} + 1" )
  endif(OpENer_TRACE_LEVEL_ERROR)
  if(OpENer_TRACE_LEVEL_WARNING)
    math( EXPR TRACE_LEVEL "${TRACE_LEVEL} + 2" )
  endif(OpENer_TRACE_LEVEL_WARNING)
  if(OpENer_TRACE_LEVEL_STATE)
    math( EXPR TRACE_LEVEL "${TRACE_LEVEL} + 4" )
  endif(OpENer_TRACE_LEVEL_STATE)
  if(OpENer_TRACE_LEVEL_INFO)
    math( EXPR TRACE_LEVEL "${TRACE_LEVEL} + 8" )
  endif(OpENer_TRACE_LEVEL_INFO)

  add_definitions(-DOPENER_TRACE_LEVEL=${TRACE_LEVEL})
endmacro(createTraceLevelOptions)

#######################################
# Build hardening, sanitizers, coverage
#######################################

# Easy toggles for sanitizers, hardening, coverage and warnings
option(OpENer_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(OpENer_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(OpENer_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(OpENer_ENABLE_MSAN "Enable MemorySanitizer (Clang only)" OFF)
option(OpENer_ENABLE_LSAN "Enable LeakSanitizer" OFF)
option(OpENer_ENABLE_COVERAGE "Enable code coverage flags" OFF)
option(OpENer_ENABLE_HARDENING "Enable hardening flags (FORTIFY, PIE, RELRO, stack protector)" OFF)
option(OpENer_ENABLE_STRICT_WARNINGS "Enable comprehensive warnings" OFF)

function(opener_apply_build_options)
  # Compose sanitizer list
  set(_sanitizers "")
  if(OpENer_ENABLE_ASAN)
    list(APPEND _sanitizers address)
  endif()
  if(OpENer_ENABLE_UBSAN)
    list(APPEND _sanitizers undefined)
  endif()
  if(OpENer_ENABLE_TSAN)
    list(APPEND _sanitizers thread)
  endif()
  if(OpENer_ENABLE_MSAN)
    list(APPEND _sanitizers memory)
  endif()
  if(OpENer_ENABLE_LSAN)
    list(APPEND _sanitizers leak)
  endif()

  if(_sanitizers)
    string(REPLACE ";" "," _sanitizer_string "${_sanitizers}")
    # Apply to compile and link flags only for enabled languages
    foreach(lang C CXX)
      # Skip languages that haven't been enabled
      if(NOT DEFINED CMAKE_${lang}_COMPILER_ID)
        continue()
      endif()
      if(CMAKE_${lang}_COMPILER_ID MATCHES "Clang" OR CMAKE_${lang}_COMPILER_ID MATCHES "GNU")
        set(var_cflags "${CMAKE_${lang}_FLAGS}")
        set(var_cflags "${var_cflags} -fsanitize=${_sanitizer_string} -fno-omit-frame-pointer")
        set(CMAKE_${lang}_FLAGS "${var_cflags}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=${_sanitizer_string}")
      else()
        message(WARNING "Sanitizers requested but unsupported compiler: ${CMAKE_${lang}_COMPILER_ID}")
      endif()
    endforeach()
    # UBSan: add recover behavior to aid debugging if only UBSan requested
    if(OpENer_ENABLE_UBSAN)
      add_compile_options(-fno-sanitize-recover=undefined)
    endif()
  endif()

  if(OpENer_ENABLE_COVERAGE)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
      message(STATUS "Enabling code coverage flags")
      foreach(lang C CXX)
        set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -g -O0 --coverage")
      endforeach()
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    else()
      message(WARNING "Coverage requested but build type is not Debug; consider using -DCMAKE_BUILD_TYPE=Debug")
    endif()
  endif()

  if(OpENer_ENABLE_HARDENING)
    message(STATUS "Enabling hardening/fortify flags")
    add_compile_options(-fstack-protector-strong -D_FORTIFY_SOURCE=2)
    # PIE and RELRO
    add_compile_options(-fPIE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro -Wl,-z,now -pie")
  endif()

  if(OpENer_ENABLE_STRICT_WARNINGS)
    message(STATUS "Enabling strict warning flags")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wdouble-promotion -Wformat=2)
  endif()
endfunction(opener_apply_build_options)

## If this is a Debug or CI/Test build, enable a sensible default set of
## sanitizers, coverage, hardening and strict warnings to make CI runs
## more effective. This will apply when the build type is Debug, tests are
## enabled (OpENer_TESTS) or the CI environment variable is present. These
## cache variables are forced here so CI scripts that don't pass flags still
## get the helpful defaults.
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR OpENer_TESTS OR DEFINED ENV{CI})
  message(STATUS "CI/Debug/Test detected: enabling sanitizer/coverage/hardening defaults")
  set(OpENer_ENABLE_ASAN     ON CACHE BOOL "Enable AddressSanitizer" FORCE)
  set(OpENer_ENABLE_UBSAN    ON CACHE BOOL "Enable UndefinedBehaviorSanitizer" FORCE)
  set(OpENer_ENABLE_LSAN     ON CACHE BOOL "Enable LeakSanitizer" FORCE)
  set(OpENer_ENABLE_COVERAGE ON CACHE BOOL "Enable code coverage flags" FORCE)
  set(OpENer_ENABLE_HARDENING ON CACHE BOOL "Enable hardening flags (FORTIFY, PIE, RELRO, stack protector)" FORCE)
  set(OpENer_ENABLE_STRICT_WARNINGS ON CACHE BOOL "Enable comprehensive warnings" FORCE)
  # MSAN and TSAN remain opt-in only due to toolchain/runtime requirements
endif()

# Apply options immediately so callers don't need to remember to invoke the function
opener_apply_build_options()
