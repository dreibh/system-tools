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
# System-Maintenance Test Suite
# Copyright (C) 2013-2026 by Thomas Dreibholz
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

# Find bash interpreter for deterministic script execution
FIND_PROGRAM(BASH_EXECUTABLE bash REQUIRED)

# Resolve path to configured System-Maintenance script
IF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/../System-Maintenance.versioned")
   SET(SYSTEM_MAINTENANCE_BIN "${CMAKE_CURRENT_BINARY_DIR}/../System-Maintenance.versioned")
ELSEIF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/System-Maintenance.versioned")
   SET(SYSTEM_MAINTENANCE_BIN "${CMAKE_CURRENT_BINARY_DIR}/System-Maintenance.versioned")
ELSEIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../System-Maintenance")
   SET(SYSTEM_MAINTENANCE_BIN "${CMAKE_CURRENT_SOURCE_DIR}/../System-Maintenance")
ELSE()
   SET(SYSTEM_MAINTENANCE_BIN "System-Maintenance")
ENDIF()


#############################################################################
#### Helper Functions                                                    ####
#############################################################################

# Helper for commands expected to succeed (exit code 0)
FUNCTION(ADD_SYSMAINT_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${BASH_EXECUTABLE} "${SYSTEM_MAINTENANCE_BIN}" ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "System-Maintenance;cli"
   )
ENDFUNCTION()

# Helper for commands verified by PASS_REGULAR_EXPRESSION (success or error)
FUNCTION(ADD_SYSMAINT_MATCH_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${BASH_EXECUTABLE} "${SYSTEM_MAINTENANCE_BIN}" ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "System-Maintenance;cli"
   )
ENDFUNCTION()

# Helper for commands asserting non-zero exit code without matching specific output
FUNCTION(ADD_SYSMAINT_FAIL_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${BASH_EXECUTABLE} "${SYSTEM_MAINTENANCE_BIN}" ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "System-Maintenance;negative;error"
   )
ENDFUNCTION()


#############################################################################
#### 1. Basic CLI Options & Internationalization                         ####
#############################################################################

# Test version flag (short option -v)
ADD_SYSMAINT_MATCH_TEST(sysmaint_cli_version_short -v)

# Test version flag (long option --version)
ADD_SYSMAINT_MATCH_TEST(sysmaint_cli_version_long --version)
SET_TESTS_PROPERTIES(sysmaint_cli_version_short sysmaint_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "System-Maintenance (@PROJECT_VERSION_MAJOR@|[0-9]+)"
)

# Test help flag (short option -h)
ADD_SYSMAINT_MATCH_TEST(sysmaint_cli_help_short -h)

# Test help flag (long option --help)
ADD_SYSMAINT_MATCH_TEST(sysmaint_cli_help_long --help)
SET_TESTS_PROPERTIES(sysmaint_cli_help_short sysmaint_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test localized help in German (de)
ADD_SYSMAINT_MATCH_TEST(sysmaint_cli_help_i18n_de -h)
SET_TESTS_PROPERTIES(sysmaint_cli_help_i18n_de PROPERTIES
   ENVIRONMENT "LC_ALL=en_US.UTF-8;LANGUAGE=de;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Aufruf:"
   LABELS "System-Maintenance;cli;i18n;locale_de"
)

# Test localized help in Norwegian Bokmål (nb)
ADD_SYSMAINT_MATCH_TEST(sysmaint_cli_help_i18n_nb -h)
SET_TESTS_PROPERTIES(sysmaint_cli_help_i18n_nb PROPERTIES
   ENVIRONMENT "LC_ALL=en_US.UTF-8;LANGUAGE=nb;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Bruk:"
   LABELS "System-Maintenance;cli;i18n;locale_nb"
)


#############################################################################
#### 2. Flag Precedence with Help and Version Options                    ####
#############################################################################

# Test help taking precedence when combined with short -k (-k -h)
ADD_SYSMAINT_MATCH_TEST(sysmaint_precedence_k_then_h -k -h)
SET_TESTS_PROPERTIES(sysmaint_precedence_k_then_h PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test help taking precedence when specified before -k (-h -k)
ADD_SYSMAINT_MATCH_TEST(sysmaint_precedence_h_then_k -h -k)
SET_TESTS_PROPERTIES(sysmaint_precedence_h_then_k PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test version taking precedence when combined with short -k (-k -v)
ADD_SYSMAINT_MATCH_TEST(sysmaint_precedence_k_then_v -k -v)
SET_TESTS_PROPERTIES(sysmaint_precedence_k_then_v PROPERTIES
   PASS_REGULAR_EXPRESSION "System-Maintenance (@PROJECT_VERSION_MAJOR@|[0-9]+)"
)

# Test version taking precedence when specified before -k (-v -k)
ADD_SYSMAINT_MATCH_TEST(sysmaint_precedence_v_then_k -v -k)
SET_TESTS_PROPERTIES(sysmaint_precedence_v_then_k PROPERTIES
   PASS_REGULAR_EXPRESSION "System-Maintenance (@PROJECT_VERSION_MAJOR@|[0-9]+)"
)

# Test help taking precedence with long options (--interactive-kernel-removal --help)
ADD_SYSMAINT_MATCH_TEST(sysmaint_precedence_long_k_then_help --interactive-kernel-removal --help)
SET_TESTS_PROPERTIES(sysmaint_precedence_long_k_then_help PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test version taking precedence with long options (--interactive-kernel-removal --version)
ADD_SYSMAINT_MATCH_TEST(sysmaint_precedence_long_k_then_version --interactive-kernel-removal --version)
SET_TESTS_PROPERTIES(sysmaint_precedence_long_k_then_version PROPERTIES
   PASS_REGULAR_EXPRESSION "System-Maintenance (@PROJECT_VERSION_MAJOR@|[0-9]+)"
)


#############################################################################
#### 3. Superuser Permission Verification (Safe Non-Root Guard)          ####
#############################################################################

# Check whether CMake/CTest is running as root
EXECUTE_PROCESS(
   COMMAND id -u
   OUTPUT_VARIABLE CURRENT_UID
   OUTPUT_STRIP_TRAILING_WHITESPACE
   ERROR_QUIET
)

# Only run execution tests that trigger the root guard when unprivileged
IF(NOT CURRENT_UID EQUAL 0)
   # Test error when invoked without arguments as non-root user
   ADD_SYSMAINT_MATCH_TEST(sysmaint_permission_denied_default)
   SET_TESTS_PROPERTIES(sysmaint_permission_denied_default PROPERTIES
      PASS_REGULAR_EXPRESSION "ERROR: System-Maintenance requires superuser permissions!"
      LABELS "System-Maintenance;permission;security"
   )

   # Test error when invoked with short -k option as non-root user
   ADD_SYSMAINT_MATCH_TEST(sysmaint_permission_denied_short_k -k)
   SET_TESTS_PROPERTIES(sysmaint_permission_denied_short_k PROPERTIES
      PASS_REGULAR_EXPRESSION "ERROR: System-Maintenance requires superuser permissions!"
      LABELS "System-Maintenance;permission;security"
   )

   # Test error when invoked with long --interactive-kernel-removal option as non-root user
   ADD_SYSMAINT_MATCH_TEST(sysmaint_permission_denied_long_k --interactive-kernel-removal)
   SET_TESTS_PROPERTIES(sysmaint_permission_denied_long_k PROPERTIES
      PASS_REGULAR_EXPRESSION "ERROR: System-Maintenance requires superuser permissions!"
      LABELS "System-Maintenance;permission;security"
   )
ENDIF()


#############################################################################
#### 4. Extraneous Positional Argument Validation                        ####
#############################################################################

# Test error on single extraneous positional argument
ADD_SYSMAINT_MATCH_TEST(sysmaint_fail_extraneous_single_arg extra_arg)
SET_TESTS_PROPERTIES(sysmaint_fail_extraneous_single_arg PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error on multiple extraneous positional arguments
ADD_SYSMAINT_MATCH_TEST(sysmaint_fail_extraneous_two_args arg1 arg2)
SET_TESTS_PROPERTIES(sysmaint_fail_extraneous_two_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error on three extraneous positional arguments
ADD_SYSMAINT_MATCH_TEST(sysmaint_fail_extraneous_multiple_args foo bar baz)
SET_TESTS_PROPERTIES(sysmaint_fail_extraneous_multiple_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error on numeric extraneous positional arguments
ADD_SYSMAINT_MATCH_TEST(sysmaint_fail_extraneous_numeric_args 1 2 3)
SET_TESTS_PROPERTIES(sysmaint_fail_extraneous_numeric_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error on double-dash followed by extraneous argument
ADD_SYSMAINT_MATCH_TEST(sysmaint_fail_extraneous_with_delimiter -- extra)
SET_TESTS_PROPERTIES(sysmaint_fail_extraneous_with_delimiter PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error on short option -k followed by extraneous argument
ADD_SYSMAINT_MATCH_TEST(sysmaint_fail_extraneous_with_k_short -k extra)
SET_TESTS_PROPERTIES(sysmaint_fail_extraneous_with_k_short PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error on long option --interactive-kernel-removal followed by extraneous argument
ADD_SYSMAINT_MATCH_TEST(sysmaint_fail_extraneous_with_k_long --interactive-kernel-removal extra)
SET_TESTS_PROPERTIES(sysmaint_fail_extraneous_with_k_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)


#############################################################################
#### 5. Option Misuse & Disallowed Parameter Arguments                   ####
#############################################################################

# Test error when --interactive-kernel-removal is given an argument (--k=foo)
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_k_long_with_arg --interactive-kernel-removal=foo)

# Test error when --help is given an argument (--help=foo)
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_help_long_with_arg --help=foo)

# Test error when --version is given an argument (--version=foo)
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_version_long_with_arg --version=foo)


#############################################################################
#### 6. Unrecognized Options                                             ####
#############################################################################

# Test error on unknown short option -z
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_unknown_short_opt_z -z)

# Test error on unknown short option -x
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_unknown_short_opt_x -x)

# Test error on unknown short option -a
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_unknown_short_opt_a -a)

# Test error on unknown short option -i
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_unknown_short_opt_i -i)

# Test error on unknown short option -r
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_unknown_short_opt_r -r)

# Test error on unknown long option --invalid-option
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_unknown_long_opt --invalid-option)

# Test error on unknown long option --dry-run
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_unknown_long_opt_dryrun --dry-run)

# Test error on unknown long option --force
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_unknown_long_opt_force --force)

# Test error on valid option -k combined with unknown short option -z
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_unknown_short_with_k -k -z)

# Test error on valid option --interactive-kernel-removal combined with unknown long option
ADD_SYSMAINT_FAIL_TEST(sysmaint_fail_unknown_long_with_k --interactive-kernel-removal --unknown)


#############################################################################
#### 7. Hook Script and Bash Completion Syntax Validation                ####
#############################################################################

# Locate the XX-example hook script
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../system-maintenance.d/XX-example")
   SET(XX_EXAMPLE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../system-maintenance.d/XX-example")
ELSEIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/system-maintenance.d/XX-example")
   SET(XX_EXAMPLE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/system-maintenance.d/XX-example")
ENDIF()

# Locate the bash completion script
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../System-Maintenance.bash-completion")
   SET(BASH_COMPLETION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../System-Maintenance.bash-completion")
ELSEIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/System-Maintenance.bash-completion")
   SET(BASH_COMPLETION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/System-Maintenance.bash-completion")
ENDIF()

# Test bash completion syntax validation via bash -n
IF(BASH_COMPLETION_FILE)
   ADD_TEST(
      NAME sysmaint_syntax_check_bash_completion
      COMMAND ${BASH_EXECUTABLE} -n "${BASH_COMPLETION_FILE}"
   )
   SET_TESTS_PROPERTIES(sysmaint_syntax_check_bash_completion PROPERTIES
      LABELS "System-Maintenance;syntax;completion"
   )
ENDIF()

# Test XX-example hook script syntax and dummy invocation
IF(XX_EXAMPLE_SCRIPT)
   # Syntax check on XX-example
   ADD_TEST(
      NAME sysmaint_syntax_check_xx_example
      COMMAND ${BASH_EXECUTABLE} -n "${XX_EXAMPLE_SCRIPT}"
   )
   SET_TESTS_PROPERTIES(sysmaint_syntax_check_xx_example PROPERTIES
      LABELS "System-Maintenance;syntax;hooks"
   )

   # Create empty package list fixture
   SET(MOCK_EMPTY_PKG_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_packages_empty.txt")
   FILE(WRITE "${MOCK_EMPTY_PKG_FILE}" "")

   # Test running XX-example against an empty package list
   ADD_TEST(
      NAME sysmaint_run_xx_example_empty
      COMMAND ${BASH_EXECUTABLE} "${XX_EXAMPLE_SCRIPT}" "${MOCK_EMPTY_PKG_FILE}"
   )
   SET_TESTS_PROPERTIES(sysmaint_run_xx_example_empty PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "System-Maintenance;hooks"
   )

   # Create populated mock package list fixture
   SET(MOCK_POPULATED_PKG_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_packages_populated.txt")
   FILE(WRITE "${MOCK_POPULATED_PKG_FILE}"
      "system-tools 2.7.15\n"
      "bash 5.2.21\n"
      "cmake 3.28.3\n"
   )

   # Test running XX-example against populated package list
   ADD_TEST(
      NAME sysmaint_run_xx_example_populated
      COMMAND ${BASH_EXECUTABLE} "${XX_EXAMPLE_SCRIPT}" "${MOCK_POPULATED_PKG_FILE}"
   )
   SET_TESTS_PROPERTIES(sysmaint_run_xx_example_populated PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "System-Maintenance;hooks"
   )
ENDIF()
