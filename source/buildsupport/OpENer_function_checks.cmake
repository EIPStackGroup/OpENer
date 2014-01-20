########################################
# Check if functions exist on platform #
########################################

include (CheckFunctionExists)

check_function_exists( srand HAVE_SRAND )
check_function_exists( rand HAVE_RAND )

if( (NOT(HAVE_SRAND)) OR (NOT(HAVE_RAND)) )
  
endif( (NOT(HAVE_SRAND)) OR (NOT(HAVE_RAND)) )
