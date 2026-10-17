# ==========================================================================
#         ____            _                     _____           _
#        / ___| _   _ ___| |_ ___ _ __ ___     |_   _|__   ___ | |___
#        \___ \| | | / __| __/ _ \ '_ ` _ \ _____| |/ _ \ / _ \| / __|
#         ___) | |_| \__ \ ||  __/ | | | | |_____| | (_) | (_) | \__ \
#        |____/ \__, |___/\__\___|_| |_| |_|     |_|\___/ \___/|_|___/
#               |___/
#                             --- System-Tools ---
#                  https://www.nntb.no/~dreibh/system-tools/
# ==========================================================================
#
# Reset-Machine-ID Test Suite
# Copyright (C) 2024-2026 by Thomas Dreibholz
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact: thomas.dreibholz@gmail.com

SET(TEST_LOCALE_DIR "${CMAKE_BINARY_DIR}/po")

# Fallback if tests subdirectory is ever evaluated standalone
IF(NOT RESET_MACHINE_ID_EXECUTABLE)
   SET(RESET_MACHINE_ID_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/../Reset-Machine-ID.versioned")
ENDIF()


#############################################################################
#### Helper Functions                                                    ####
#############################################################################

# ###### Helper for deterministic commands (expected return code: 0) ########
FUNCTION(ADD_RESET_MACHINE_ID_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${RESET_MACHINE_ID_EXECUTABLE} ${ARGN}
   )
   SET(TEST_LABELS "reset-machine-id;cli")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


# ###### Helper for expected failures (expected return code: non-zero) ######
FUNCTION(ADD_RESET_MACHINE_ID_FAIL_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${RESET_MACHINE_ID_EXECUTABLE} ${ARGN}
   )
   SET(TEST_LABELS "reset-machine-id;negative;error")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


#############################################################################
#### Basic CLI Options                                                   ####
#############################################################################

ADD_RESET_MACHINE_ID_TEST(resetmachineid_cli_version_short -v)
ADD_RESET_MACHINE_ID_TEST(resetmachineid_cli_version_long  --version)
ADD_RESET_MACHINE_ID_TEST(resetmachineid_cli_help_short    -h)
ADD_RESET_MACHINE_ID_TEST(resetmachineid_cli_help_long     --help)

SET_TESTS_PROPERTIES(resetmachineid_cli_version_short resetmachineid_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Reset-Machine-ID [0-9]+\\.[0-9]+"
)

SET_TESTS_PROPERTIES(resetmachineid_cli_help_short resetmachineid_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# === CLI Help Internationalization =========================================
ADD_RESET_MACHINE_ID_TEST(resetmachineid_cli_help_i18n_de -h)
ADD_RESET_MACHINE_ID_TEST(resetmachineid_cli_help_i18n_nb -h)

SET_TESTS_PROPERTIES(resetmachineid_cli_help_i18n_de PROPERTIES
   ENVIRONMENT "LANGUAGE=de;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Aufruf:"
   LABELS "reset-machine-id;cli;i18n;locale_de"
)

SET_TESTS_PROPERTIES(resetmachineid_cli_help_i18n_nb PROPERTIES
   ENVIRONMENT "LANGUAGE=nb;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Bruk:"
   LABELS "reset-machine-id;cli;i18n;locale_nb"
)


#############################################################################
#### Hostname Syntax Validation Tests                                    ####
#############################################################################

# Leading digit (labels must start with a letter):
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_hostname_leading_digit -H 123server)

# Leading hyphen:
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_hostname_leading_hyphen -H -server.example.com)

# Underscore is prohibited in standard hostnames:
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_hostname_underscore -H server_name.example.com)

# Leading dot:
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_hostname_leading_dot -H .server.example.com)

# Consecutive dots:
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_hostname_double_dot -H server..example.com)

# Special characters:
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_hostname_special -H host@domain.example)

# Long option flag equivalent (--hostname):
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_hostname_long_invalid --hostname 123badhost)


#############################################################################
#### CLI Error Handling & Validation                                     ####
#############################################################################

# Unknown flags:
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_unknown_flag      --invalid-option)

# Missing option arguments:
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_missing_host_short -H)
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_missing_host_long  --hostname)
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_missing_zbx_short  -Z)
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_missing_zbx_long   --zabbix)

# Excess positional arguments:
ADD_RESET_MACHINE_ID_FAIL_TEST(resetmachineid_fail_extra_argument     extra_positional_arg)
