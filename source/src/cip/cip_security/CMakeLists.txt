#######################################
# CIP Security library                #
#######################################

#######################################
# Add common includes                 #
#######################################
opener_common_includes()

#######################################
# Add platform-specific includes      #
#######################################
opener_platform_support("INCLUDES")

set( CIP_SECURITY_SRC cipsecurity.c certificatemanagement.c ethernetipsecurity.c gen_key.c cert_req.c cert_write.c)

add_library( CIP_SECURITY ${CIP_SECURITY_SRC})
# target_link_libraries(CIP_SECURITY  CIP_FILE_OBJECT)
target_link_libraries(CIP_SECURITY PUBLIC CIP_FILE_OBJECT 
MbedTLS::mbedtls
MbedTLS::mbedcrypto
MbedTLS::mbedx509)

###############################################################
# RSA key file for generated private-public key pair          #
###############################################################
set(OPENER_DATA_FOLDER "${OpENer_SOURCE_DIR}/../data")
if(EXISTS "${OPENER_DATA_FOLDER}/keyfile.key")
    find_file( RSA_KEY_FILE_LOCATION NAMES keyfile.key PATHS ${OPENER_DATA_FOLDER} NO_DEFAULT_PATH  DOC "RSA key file" )
else()
    file(WRITE ${OPENER_DATA_FOLDER}/keyfile.key "") # create empty file if not exists
    set( RSA_KEY_FILE_LOCATION ${OPENER_DATA_FOLDER}/keyfile.key)
endif()
add_compile_definitions(RSA_KEY_FILE_LOCATION=\"${RSA_KEY_FILE_LOCATION}\")

###############################################################
# Certificate file for default device certificate             #
###############################################################
find_file( FILE_OBJECT_CERTIFICATE_FILE_LOCATION NAMES cert.crt PATHS ${OPENER_DATA_FOLDER} NO_DEFAULT_PATH  DOC "Default Device Certificate File" )
add_compile_definitions(FILE_OBJECT_CERTIFICATE_FILE_LOCATION=\"${FILE_OBJECT_CERTIFICATE_FILE_LOCATION}\")

###############################################################
# CSR file for static File Object Instance                    # 
###############################################################
find_file( FILE_OBJECT_CSR_FILE_LOCATION NAMES csr.req PATHS ${OPENER_DATA_FOLDER} NO_DEFAULT_PATH  DOC "CSR File" )
add_compile_definitions(FILE_OBJECT_CSR_FILE_LOCATION=\"${FILE_OBJECT_CSR_FILE_LOCATION}\")