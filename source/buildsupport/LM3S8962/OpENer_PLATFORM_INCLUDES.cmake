macro(opener_platform_spec)
  set( LM3S8962_CONTRIB_DIR "" CACHE PATH "Contrib folder of the used LM3S0862 board")
  include_directories(${PORTS_SRC_DIR}/${OpENer_PLATFORM} ${PORTS_SRC_DIR}/${OpENer_PLATFORM}/sample_application ${LM3S8962_CONTRIB_DIR}/utils ${LM3S8962_CONTRIB_DIR}/boards/rdk-bldc/qs-bldc/ )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -std=c99 -mthumb mcpu=cortex-m3 -mfloat-abi=soft -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -DGCC_ARMCM3_LM3S102 -Dgcc" )
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostartfiles --no-gc-sections" )
endmacro(opener_platform_spec)

