#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

set( CipTestSrc cipepathtest.cpp cipelectronickeytest.cpp  cipelectronickeyformattest.cpp cipconnectionmanagertest.cpp cipconnectionobjecttest.cpp cipcommontests.cpp cipstringtests.cpp)

include_directories( ${SRC_DIR}/cip )

add_library( CipTest ${CipTestSrc} )

target_link_libraries( CipTest CIP ENET_ENCAP PLATFORM_GENERIC ${OpENer_PLATFORM}PLATFORM ${PLATFORM_SPEC_LIBS} rt)

target_link_libraries( CipTest gcov ${CPPUTEST_LIBRARY} ${CPPUTESTEXT_LIBRARY} )
