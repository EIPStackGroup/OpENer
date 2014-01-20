macro(opener_platform_spec)
  set( LM3S8962_CONTRIB_DIR "" CACHE PATH "Contrib folder of the used LM3S0862 board")
  include_directories(${PORTS_SRC_DIR}/${OpENer_PLATFORM} 
			${PORTS_SRC_DIR}/${OpENer_PLATFORM}/sample_application 
			${LM3S8962_CONTRIB_DIR}
			${LM3S8962_CONTRIB_DIR}/utils 
			${LM3S8962_CONTRIB_DIR}/boards/rdk-bldc/qs-bldc/
			${LM3S8962_CONTRIB_DIR}/boards/ek-lm3s8962
			${LM3S8962_CONTRIB_DIR}/boards/ek-lm3s8962/enet_lwip/
			${LM3S8962_CONTRIB_DIR}/third_party/lwip-1.3.2/src/include
			${LM3S8962_CONTRIB_DIR}/third_party/lwip-1.3.2/ports/stellaris/include/
			${LM3S8962_CONTRIB_DIR}/third_party/lwip-1.3.2/src/include/ipv4/
			${LM3S8962_CONTRIB_DIR}/inc/)
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -std=c99 -mfpu=fpv4-sp-d16 -DGCC_ARMCM3_LM3S102 -Dgcc -DEIP_DEBUG=0" )
  set( PLATFORM_LINKER_FLAGS "-Wl,--no-gc-sections -nostartfiles" )
endmacro(opener_platform_spec)

