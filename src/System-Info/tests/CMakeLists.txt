SET(TEST_LOCALE_DIR "${CMAKE_BINARY_DIR}/po")


#############################################################################
#### get-system-info                                                     ####
#############################################################################

# ###### Helper for deterministic commands (expected return code: 0) ########
FUNCTION(ADD_GETSYSTEMINFO_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:get-system-info> ${ARGN}
   )
   SET(TEST_LABELS "cli")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


# ###### Helper for expected failures (expected return code: non-zero) ######
FUNCTION(ADD_GETSYSTEMINFO_FAIL_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:get-system-info> ${ARGN}
   )
   SET(TEST_LABELS "negative;error")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


# ###### Basic CLI Options ##################################################

ADD_GETSYSTEMINFO_TEST(getsysteminfo_cli_version_short -v)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_cli_version_long  --version)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_cli_help_short    -h)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_cli_help_long     --help)

SET_TESTS_PROPERTIES(getsysteminfo_cli_version_short getsysteminfo_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "get-system-info [0-9]+\\.[0-9]+"
)

SET_TESTS_PROPERTIES(getsysteminfo_cli_help_short getsysteminfo_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)


#############################################################################
#### System-Info                                                         ####
#############################################################################

# ###### Helper for deterministic commands (expected return code: 0) ########
FUNCTION(ADD_SYSTEMINFO_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:System-Info> ${ARGN}
   )
   SET(TEST_LABELS "cli")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


# ###### Helper for expected failures (expected return code: non-zero) ######
FUNCTION(ADD_SYSTEMINFO_FAIL_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:System-Info> ${ARGN}
   )
   SET(TEST_LABELS "negative;error")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


# ###### Basic CLI Options ##################################################

ADD_SYSTEMINFO_TEST(systeminfo_cli_version_short -v)
ADD_SYSTEMINFO_TEST(systeminfo_cli_version_long  --version)
ADD_SYSTEMINFO_TEST(systeminfo_cli_help_short    -h)
ADD_SYSTEMINFO_TEST(systeminfo_cli_help_long     --help)

SET_TESTS_PROPERTIES(systeminfo_cli_version_short systeminfo_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "System-Info [0-9]+\\.[0-9]+"
)

SET_TESTS_PROPERTIES(systeminfo_cli_help_short systeminfo_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)
