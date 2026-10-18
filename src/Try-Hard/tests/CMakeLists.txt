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
# Try-Hard Test Suite
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

# Determine try-hard executable path
IF(TARGET try-hard)
   GET_TARGET_PROPERTY(TRY_HARD_TYPE try-hard TYPE)
   IF(TRY_HARD_TYPE STREQUAL "EXECUTABLE")
      SET(TRY_HARD_BIN "$<TARGET_FILE:try-hard>")
   ELSE()
      SET(TRY_HARD_BIN "${CMAKE_CURRENT_BINARY_DIR}/../try-hard")
   ENDIF()
ELSEIF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/../try-hard")
   SET(TRY_HARD_BIN "${CMAKE_CURRENT_BINARY_DIR}/../try-hard")
ELSEIF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/bin/try-hard")
   SET(TRY_HARD_BIN "${CMAKE_CURRENT_BINARY_DIR}/bin/try-hard")
ELSE()
   SET(TRY_HARD_BIN "try-hard")
ENDIF()


#############################################################################
#### Helper Functions                                                    ####
#############################################################################

# Helper for deterministic commands expected to succeed (exit code 0)
FUNCTION(ADD_TRY_HARD_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${TRY_HARD_BIN} ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "try-hard;cli"
   )
ENDFUNCTION()

# Helper for commands verified by PASS_REGULAR_EXPRESSION (success or error)
FUNCTION(ADD_TRY_HARD_MATCH_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${TRY_HARD_BIN} ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "try-hard;cli"
   )
ENDFUNCTION()

# Helper for commands asserting non-zero exit code without matching specific output
FUNCTION(ADD_TRY_HARD_FAIL_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${TRY_HARD_BIN} ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "try-hard;negative;error"
   )
ENDFUNCTION()

# Helper for commands expected to return a specific exit code
FUNCTION(ADD_TRY_HARD_EXIT_CODE_TEST TEST_NAME EXPECTED_CODE)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${TRY_HARD_BIN} ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "try-hard;exit_code"
   )
   IF(EXPECTED_CODE EQUAL 0)
      SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
         FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      )
   ELSE()
      SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
         WILL_FAIL TRUE
      )
   ENDIF()
ENDFUNCTION()


#############################################################################
#### 1. Basic CLI Options & Internationalization                         ####
#############################################################################

# Test version flag (short option -v)
ADD_TRY_HARD_MATCH_TEST(tryhard_cli_version_short -v)

# Test version flag (long option --version)
ADD_TRY_HARD_MATCH_TEST(tryhard_cli_version_long --version)
SET_TESTS_PROPERTIES(tryhard_cli_version_short tryhard_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "try-hard (@BUILD_MAJOR@|[0-9]+)"
)

# Test help flag (short option -h)
ADD_TRY_HARD_TEST(tryhard_cli_help_short -h)

# Test help flag (long option --help)
ADD_TRY_HARD_TEST(tryhard_cli_help_long --help)
SET_TESTS_PROPERTIES(tryhard_cli_help_short tryhard_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test localized help in German (de)
ADD_TRY_HARD_TEST(tryhard_cli_help_i18n_de -h)
SET_TESTS_PROPERTIES(tryhard_cli_help_i18n_de PROPERTIES
   ENVIRONMENT "LANGUAGE=de;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Aufruf:"
   LABELS "try-hard;cli;i18n;locale_de"
)

# Test localized help in Norwegian Bokmål (nb)
ADD_TRY_HARD_TEST(tryhard_cli_help_i18n_nb -h)
SET_TESTS_PROPERTIES(tryhard_cli_help_i18n_nb PROPERTIES
   ENVIRONMENT "LANGUAGE=nb;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Bruk:"
   LABELS "try-hard;cli;i18n;locale_nb"
)


#############################################################################
#### 2. Basic Successful Execution (Single & Multiple Trials)            ####
#############################################################################

# Test command succeeding immediately on 1 trial
ADD_TRY_HARD_TEST(tryhard_success_single_trial 1 0 -D -- true)

# Test command succeeding with 2 trials allowed
ADD_TRY_HARD_TEST(tryhard_success_2_trials 2 0 -D -- true)

# Test command succeeding with 3 trials allowed
ADD_TRY_HARD_TEST(tryhard_success_3_trials 3 0 -D -- true)

# Test command succeeding with 5 trials allowed
ADD_TRY_HARD_TEST(tryhard_success_5_trials 5 0 -D -- true)

# Test command succeeding with 10 trials allowed
ADD_TRY_HARD_TEST(tryhard_success_10_trials 10 0 -D -- true)

# Test executing a command with arguments
ADD_TRY_HARD_TEST(tryhard_success_with_args 1 0 -D -- echo "Hello World")
SET_TESTS_PROPERTIES(tryhard_success_with_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Hello World"
)

# Test executing echo with options and multiple arguments
ADD_TRY_HARD_TEST(tryhard_success_echo_options 1 0 -D -- echo -n "Test Token")
SET_TESTS_PROPERTIES(tryhard_success_echo_options PROPERTIES
   PASS_REGULAR_EXPRESSION "Test Token"
)

# Test executing a complex inline script with exit 0
ADD_TRY_HARD_TEST(tryhard_success_inline_sh 2 0 -D -- sh -c "exit 0")

# Test long option (--deterministic)
ADD_TRY_HARD_TEST(tryhard_success_long_opts 1 0 --deterministic -- true)


#############################################################################
#### 3. Command Failure & Return Code Propagation                        ####
#############################################################################

# Test failure with standard false command (exit code 1) on 1 trial
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_single_trial 1 0 -D -- false)

# Test failure with standard false command on 2 trials
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_2_trials 2 0 -D -- false)

# Test failure after exhausting 3 trials (exit code 1)
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_3_trials 3 0 -D -- false)

# Test failure after exhausting 5 trials (exit code 1)
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_5_trials 5 0 -D -- false)

# Test preserving command exit code 2
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_2 2 1 0 -D -- sh -c "exit 2")

# Test preserving command exit code 3
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_3 3 1 0 -D -- sh -c "exit 3")

# Test preserving command exit code 4
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_4 4 1 0 -D -- sh -c "exit 4")

# Test preserving command exit code 7
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_7 7 1 0 -D -- sh -c "exit 7")

# Test preserving command exit code 13
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_13 13 2 0 -D -- sh -c "exit 13")

# Test preserving command exit code 27
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_27 27 2 0 -D -- sh -c "exit 27")

# Test preserving command exit code 42
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_42 42 2 0 -D -- sh -c "exit 42")

# Test preserving command exit code 64
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_64 64 2 0 -D -- sh -c "exit 64")

# Test preserving command exit code 77
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_77 77 1 0 -D -- sh -c "exit 77")

# Test preserving command exit code 99
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_99 99 2 0 -D -- sh -c "exit 99")

# Test preserving command exit code 101
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_101 101 1 0 -D -- sh -c "exit 101")

# Test preserving command exit code 111
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_111 111 2 0 -D -- sh -c "exit 111")

# Test preserving command exit code 125
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_125 125 2 0 -D -- sh -c "exit 125")

# Test preserving command exit code 126
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_126 126 1 0 -D -- sh -c "exit 126")

# Test preserving command exit code 127
ADD_TRY_HARD_EXIT_CODE_TEST(tryhard_exit_code_127 127 1 0 -D -- sh -c "exit 127")


#############################################################################
#### 4. Transient Failure Recovery State Machine                         ####
#############################################################################

# Test recovering on trial 2 using a temporary marker file
SET(RECOVERY_STATE_FILE_2 "${CMAKE_CURRENT_BINARY_DIR}/recovery_state_2.txt")
ADD_TEST(
   NAME tryhard_transient_recovery_trial_2_cleanup
   COMMAND ${CMAKE_COMMAND} -E remove -f "${RECOVERY_STATE_FILE_2}"
)
ADD_TEST(
   NAME tryhard_transient_recovery_trial_2_execute
   COMMAND ${TRY_HARD_BIN} 3 0 -D -- sh -c "if [ -f \"${RECOVERY_STATE_FILE_2}\" ]; then rm -f \"${RECOVERY_STATE_FILE_2}\"; exit 0; else touch \"${RECOVERY_STATE_FILE_2}\"; exit 1; fi"
)
SET_TESTS_PROPERTIES(tryhard_transient_recovery_trial_2_execute PROPERTIES
   DEPENDS tryhard_transient_recovery_trial_2_cleanup
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
   LABELS "try-hard;recovery"
)

# Test recovering on trial 3 using an attempt counter file
SET(RECOVERY_STATE_FILE_3 "${CMAKE_CURRENT_BINARY_DIR}/recovery_state_3.txt")
ADD_TEST(
   NAME tryhard_transient_recovery_trial_3_cleanup
   COMMAND ${CMAKE_COMMAND} -E remove -f "${RECOVERY_STATE_FILE_3}"
)
ADD_TEST(
   NAME tryhard_transient_recovery_trial_3_execute
   COMMAND ${TRY_HARD_BIN} 4 0 -D -- sh -c "CNT=0; [ -f \"${RECOVERY_STATE_FILE_3}\" ] && CNT=\$(cat \"${RECOVERY_STATE_FILE_3}\"); CNT=\$((CNT + 1)); echo \$CNT > \"${RECOVERY_STATE_FILE_3}\"; if [ \$CNT -ge 3 ]; then rm -f \"${RECOVERY_STATE_FILE_3}\"; exit 0; else exit 1; fi"
)
SET_TESTS_PROPERTIES(tryhard_transient_recovery_trial_3_execute PROPERTIES
   DEPENDS tryhard_transient_recovery_trial_3_cleanup
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
   LABELS "try-hard;recovery"
)

# Test recovering on trial 4 using an attempt counter file
SET(RECOVERY_STATE_FILE_4 "${CMAKE_CURRENT_BINARY_DIR}/recovery_state_4.txt")
ADD_TEST(
   NAME tryhard_transient_recovery_trial_4_cleanup
   COMMAND ${CMAKE_COMMAND} -E remove -f "${RECOVERY_STATE_FILE_4}"
)
ADD_TEST(
   NAME tryhard_transient_recovery_trial_4_execute
   COMMAND ${TRY_HARD_BIN} 5 0 -D -- sh -c "CNT=0; [ -f \"${RECOVERY_STATE_FILE_4}\" ] && CNT=\$(cat \"${RECOVERY_STATE_FILE_4}\"); CNT=\$((CNT + 1)); echo \$CNT > \"${RECOVERY_STATE_FILE_4}\"; if [ \$CNT -ge 4 ]; then rm -f \"${RECOVERY_STATE_FILE_4}\"; exit 0; else exit 1; fi"
)
SET_TESTS_PROPERTIES(tryhard_transient_recovery_trial_4_execute PROPERTIES
   DEPENDS tryhard_transient_recovery_trial_4_cleanup
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
   LABELS "try-hard;recovery"
)

# Test failing when required attempts exceed max_trials limit
SET(RECOVERY_STATE_FILE_FAIL "${CMAKE_CURRENT_BINARY_DIR}/recovery_state_fail.txt")
ADD_TEST(
   NAME tryhard_transient_exhausted_cleanup
   COMMAND ${CMAKE_COMMAND} -E remove -f "${RECOVERY_STATE_FILE_FAIL}"
)
ADD_TEST(
   NAME tryhard_transient_exhausted_execute
   COMMAND ${TRY_HARD_BIN} 2 0 -D -- sh -c "CNT=0; [ -f \"${RECOVERY_STATE_FILE_FAIL}\" ] && CNT=\$(cat \"${RECOVERY_STATE_FILE_FAIL}\"); CNT=\$((CNT + 1)); echo \$CNT > \"${RECOVERY_STATE_FILE_FAIL}\"; if [ \$CNT -ge 3 ]; then exit 0; else exit 1; fi"
)
SET_TESTS_PROPERTIES(tryhard_transient_exhausted_execute PROPERTIES
   WILL_FAIL TRUE
   DEPENDS tryhard_transient_exhausted_cleanup
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
   LABELS "try-hard;recovery"
)


#############################################################################
#### 5. Verbosity Control (-w / --verbose, -q / --quiet)                 ####
#############################################################################

# Test verbose output printing trial progress and timestamps (-w)
ADD_TRY_HARD_MATCH_TEST(tryhard_verbose_progress_short 2 0 -D -w -- false)
SET_TESTS_PROPERTIES(tryhard_verbose_progress_short PROPERTIES
   PASS_REGULAR_EXPRESSION "Trial 1/2:.*Trial 2/2:.*giving up now!"
   LABELS "try-hard;verbose"
)

# Test verbose output with long option (--verbose)
ADD_TRY_HARD_MATCH_TEST(tryhard_verbose_progress_long 2 0 -D --verbose -- false)
SET_TESTS_PROPERTIES(tryhard_verbose_progress_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Trial 1/2:.*Trial 2/2:.*giving up now!"
   LABELS "try-hard;verbose"
)

# Test verbose output on 3 trials showing each iteration
ADD_TRY_HARD_MATCH_TEST(tryhard_verbose_progress_3_trials 3 0 -D -w -- false)
SET_TESTS_PROPERTIES(tryhard_verbose_progress_3_trials PROPERTIES
   PASS_REGULAR_EXPRESSION "Trial 1/3:.*Trial 2/3:.*Trial 3/3:.*giving up now!"
   LABELS "try-hard;verbose"
)

# Test verbose output validating ISO timestamp format
ADD_TRY_HARD_MATCH_TEST(tryhard_verbose_timestamp_format 1 0 -D -w -- true)
SET_TESTS_PROPERTIES(tryhard_verbose_timestamp_format PROPERTIES
   PASS_REGULAR_EXPRESSION "[0-9]+-[0-9]+-[0-9]+T[0-9]+:[0-9]+:[0-9]+: Trial 1/1:"
   LABELS "try-hard;verbose"
)

# Test quiet mode suppressing trial progress output (-q)
ADD_TRY_HARD_FAIL_TEST(tryhard_quiet_mode_short 2 0 -D -q -- false)
SET_TESTS_PROPERTIES(tryhard_quiet_mode_short PROPERTIES
   FAIL_REGULAR_EXPRESSION "Trial 1/2|giving up now"
   LABELS "try-hard;quiet"
)

# Test quiet mode with long option (--quiet)
ADD_TRY_HARD_FAIL_TEST(tryhard_quiet_mode_long 2 0 -D --quiet -- false)
SET_TESTS_PROPERTIES(tryhard_quiet_mode_long PROPERTIES
   FAIL_REGULAR_EXPRESSION "Trial 1/2|giving up now"
   LABELS "try-hard;quiet"
)

# Test default verbosity (quiet by default without -w)
ADD_TRY_HARD_FAIL_TEST(tryhard_default_verbosity 2 0 -D -- false)
SET_TESTS_PROPERTIES(tryhard_default_verbosity PROPERTIES
   FAIL_REGULAR_EXPRESSION "Trial 1/2|giving up now"
   LABELS "try-hard;quiet"
)

# Test flag precedence: verbose followed by quiet (-w -q -> quiet)
ADD_TRY_HARD_FAIL_TEST(tryhard_verbosity_override_verbose_then_quiet 2 0 -D -w -q -- false)
SET_TESTS_PROPERTIES(tryhard_verbosity_override_verbose_then_quiet PROPERTIES
   FAIL_REGULAR_EXPRESSION "Trial 1/2|giving up now"
   LABELS "try-hard;quiet"
)

# Test flag precedence: quiet followed by verbose (-q -w -> verbose)
ADD_TRY_HARD_MATCH_TEST(tryhard_verbosity_override_quiet_then_verbose 2 0 -D -q -w -- false)
SET_TESTS_PROPERTIES(tryhard_verbosity_override_quiet_then_verbose PROPERTIES
   PASS_REGULAR_EXPRESSION "Trial 1/2:.*giving up now!"
   LABELS "try-hard;verbose"
)

# Test flag precedence with long options (--verbose --quiet -> quiet)
ADD_TRY_HARD_FAIL_TEST(tryhard_verbosity_override_long_v_then_q 2 0 -D --verbose --quiet -- false)
SET_TESTS_PROPERTIES(tryhard_verbosity_override_long_v_then_q PROPERTIES
   FAIL_REGULAR_EXPRESSION "Trial 1/2|giving up now"
   LABELS "try-hard;quiet"
)

# Test flag precedence with long options (--quiet --verbose -> verbose)
ADD_TRY_HARD_MATCH_TEST(tryhard_verbosity_override_long_q_then_v 2 0 -D --quiet --verbose -- false)
SET_TESTS_PROPERTIES(tryhard_verbosity_override_long_q_then_v PROPERTIES
   PASS_REGULAR_EXPRESSION "Trial 1/2:.*giving up now!"
   LABELS "try-hard;verbose"
)


#############################################################################
#### 6. Backoff Options & Parameter Combinations                         ####
#############################################################################

# Test minimum delay short option (-m)
ADD_TRY_HARD_TEST(tryhard_opt_min_delay_short -m 0.0 1 0 -D -- true)

# Test minimum delay long option (--min-delay)
ADD_TRY_HARD_TEST(tryhard_opt_min_delay_long --min-delay 0.0 1 0 -D -- true)

# Test additive increase short option (-A)
ADD_TRY_HARD_TEST(tryhard_opt_additive_short -A 0.01 1 0 -D -- true)

# Test additive increase long option (--additive-increase)
ADD_TRY_HARD_TEST(tryhard_opt_additive_long --additive-increase 0.01 1 0 -D -- true)

# Test multiplicative increase short option (-M)
ADD_TRY_HARD_TEST(tryhard_opt_multiplicative_short -M 1.5 1 0 -D -- true)

# Test multiplicative increase long option (--multiplicative-increase)
ADD_TRY_HARD_TEST(tryhard_opt_multiplicative_long --multiplicative-increase 1.5 1 0 -D -- true)

# Test truncate delay short option (-T)
ADD_TRY_HARD_TEST(tryhard_opt_truncate_short -T 1.0 1 0 -D -- true)

# Test truncate delay long option (--truncate-delay)
ADD_TRY_HARD_TEST(tryhard_opt_truncate_long --truncate-delay 1.0 1 0 -D -- true)

# Test combining all backoff parameters with short options
ADD_TRY_HARD_TEST(tryhard_opts_combined_short -m 0.0 -A 0.01 -M 2.0 -T 0.1 2 0 -D -- true)

# Test combining all backoff parameters with long options
ADD_TRY_HARD_TEST(tryhard_opts_combined_long
   --min-delay 0.0 --additive-increase 0.01 --multiplicative-increase 2.0 --truncate-delay 0.1
   2 0 --deterministic -- true
)

# Test automatic swap when min_delay argument > max_delay argument
ADD_TRY_HARD_MATCH_TEST(tryhard_min_greater_than_max -m 0.02 2 0.01 -D -w -- false)
SET_TESTS_PROPERTIES(tryhard_min_greater_than_max PROPERTIES
   PASS_REGULAR_EXPRESSION "Trial 1/2"
   LABELS "try-hard;backoff"
)

# Test backoff message in deterministic mode logs sleep duration
ADD_TRY_HARD_MATCH_TEST(tryhard_deterministic_log_delay 2 0 -D -w -- false)
SET_TESTS_PROPERTIES(tryhard_deterministic_log_delay PROPERTIES
   PASS_REGULAR_EXPRESSION "waiting for 0 s \\.\\.\\."
   LABELS "try-hard;verbose;backoff"
)

# Test that -T caps backoff delay progression across multiple retries
ADD_TRY_HARD_MATCH_TEST(tryhard_backoff_truncation_enforced
   -m 0.0 -A 10.0 -T 5.0 3 0.0 -D -w -- false
)
SET_TESTS_PROPERTIES(tryhard_backoff_truncation_enforced PROPERTIES
   PASS_REGULAR_EXPRESSION "waiting for 5 s \\.\\.\\."
   LABELS "try-hard;backoff;truncation"
)


#############################################################################
#### 7. Floating-Point Delay Value Formats (Accepted Syntax)             ####
#############################################################################

# Test integer delay (0)
ADD_TRY_HARD_TEST(tryhard_delay_format_integer_0 1 0 -D -- true)

# Test integer delay (1)
ADD_TRY_HARD_TEST(tryhard_delay_format_integer_1 1 1 -D -- true)

# Test float with leading zero and fractional part (0.0)
ADD_TRY_HARD_TEST(tryhard_delay_format_zero_dot_zero 1 0.0 -D -- true)

# Test float with multiple zeros (0.000)
ADD_TRY_HARD_TEST(tryhard_delay_format_multi_zero 1 0.000 -D -- true)

# Test float without leading zero (.0)
ADD_TRY_HARD_TEST(tryhard_delay_format_dot_zero 1 .0 -D -- true)

# Test float without leading zero (.5)
ADD_TRY_HARD_TEST(tryhard_delay_format_dot_five 1 .5 -D -- true)

# Test float without trailing zero (0.)
ADD_TRY_HARD_TEST(tryhard_delay_format_zero_dot 1 0. -D -- true)

# Test float without trailing zero (1.)
ADD_TRY_HARD_TEST(tryhard_delay_format_one_dot 1 1. -D -- true)

# Test sub-millisecond float (0.001)
ADD_TRY_HARD_TEST(tryhard_delay_format_submilli 1 0.001 -D -- true)

# Test microsecond scale float (0.000001)
ADD_TRY_HARD_TEST(tryhard_delay_format_micro 1 0.000001 -D -- true)


#############################################################################
#### 8. Option Passthrough After Delimiter (--)                          ####
#############################################################################

# Test passing options that look like try-hard flags (-m, -v) to child command
ADD_TRY_HARD_MATCH_TEST(tryhard_passthrough_child_flags 1 0 -D -- echo -m -v --help)
SET_TESTS_PROPERTIES(tryhard_passthrough_child_flags PROPERTIES
   PASS_REGULAR_EXPRESSION "-m -v --help"
   LABELS "try-hard;passthrough"
)


#############################################################################
#### 9. Command-Line Error Handling & Negative Boundaries                ####
#############################################################################

# ------ Missing Positional Arguments (< 3 arguments) -----------------------
# Test error when invoked with no arguments
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_missing_all_args)
SET_TESTS_PROPERTIES(tryhard_fail_missing_all_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error when missing max_delay and command (1 arg)
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_missing_delay_and_cmd 1)
SET_TESTS_PROPERTIES(tryhard_fail_missing_delay_and_cmd PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error when missing command (2 args)
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_missing_command 1 0)
SET_TESTS_PROPERTIES(tryhard_fail_missing_command PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error when only option flags are passed without positional arguments
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_only_options -D -w)
SET_TESTS_PROPERTIES(tryhard_fail_only_options PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error when only delimiter -- is passed without command
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_only_delimiter --)
SET_TESTS_PROPERTIES(tryhard_fail_only_delimiter PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# ------ Invalid max_trials Parameter ---------------------------------------
# Test error when max_trials is 0
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_max_trials_zero 0 0 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_max_trials_zero PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid maximum trials 0!"
)

# Test error when max_trials is non-numeric
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_max_trials_non_numeric abc 0 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_max_trials_non_numeric PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid maximum trials abc!"
)

# Test error when max_trials is a float
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_max_trials_float 1.5 0 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_max_trials_float PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid maximum trials 1\\.5!"
)

# Test error when max_trials contains spaces
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_max_trials_spaces " 1 " 0 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_max_trials_spaces PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid maximum trials"
)

# ------ Invalid max_delay Parameter ----------------------------------------
# Test error when max_delay is non-numeric
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_max_delay_non_numeric 1 abc -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_max_delay_non_numeric PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid delay value abc!"
)

# Test error when max_delay contains multiple decimal points
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_max_delay_multiple_dots 1 1..2 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_max_delay_multiple_dots PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid delay value 1\\.\\.2!"
)

# Test error when max_delay contains invalid trailing suffix
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_max_delay_suffix 1 1.0s -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_max_delay_suffix PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid delay value 1\\.0s!"
)

# ------ Invalid Option Arguments -------------------------------------------
# Test error on non-numeric -m argument
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_invalid_min_delay_str -m abc 1 0 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_invalid_min_delay_str PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid delay value abc!"
)

# Test error on multiple dots in -m argument
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_invalid_min_delay_dots -m 1..2 1 0 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_invalid_min_delay_dots PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid delay value 1\\.\\.2!"
)

# Test error on non-numeric -A argument
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_invalid_additive_str -A xyz 1 0 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_invalid_additive_str PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid delay value xyz!"
)

# Test error on multiple dots in -A argument
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_invalid_additive_dots -A 0..1 1 0 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_invalid_additive_dots PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid delay value 0\\.\\.1!"
)

# Test error on non-numeric -M argument
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_invalid_multiplicative_str -M foo 1 0 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_invalid_multiplicative_str PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid delay value foo!"
)

# Test error on multiple dots in -M argument
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_invalid_multiplicative_dots -M 1.2.3 1 0 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_invalid_multiplicative_dots PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid delay value 1\\.2\\.3!"
)

# Test error on non-numeric -T argument
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_invalid_truncate_str -T bar 1 0 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_invalid_truncate_str PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid delay value bar!"
)

# Test error on multiple dots in -T argument
ADD_TRY_HARD_MATCH_TEST(tryhard_fail_invalid_truncate_dots -T 10..5 1 0 -D -- true)
SET_TESTS_PROPERTIES(tryhard_fail_invalid_truncate_dots PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Invalid delay value 10\\.\\.5!"
)

# ------ Missing Required Option Arguments ----------------------------------
# Test error when -m is passed without argument
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_missing_opt_arg_m -m)

# Test error when -A is passed without argument
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_missing_opt_arg_A -A)

# Test error when -M is passed without argument
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_missing_opt_arg_M -M)

# Test error when -T is passed without argument
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_missing_opt_arg_T -T)

# Test error when --min-delay is passed without argument
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_missing_opt_arg_min_delay --min-delay)

# Test error when --additive-increase is passed without argument
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_missing_opt_arg_additive --additive-increase)

# Test error when --multiplicative-increase is passed without argument
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_missing_opt_arg_multiplicative --multiplicative-increase)

# Test error when --truncate-delay is passed without argument
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_missing_opt_arg_truncate --truncate-delay)

# ------ Unrecognized Options -----------------------------------------------
# Test error on unknown short option flag -z
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_unknown_short_opt_z -z 1 0 -D -- true)

# Test error on unknown short option flag -X
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_unknown_short_opt_X -X 1 0 -D -- true)

# Test error on unknown long option flag --invalid-option
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_unknown_long_opt --invalid-option 1 0 -D -- true)

# Test error on unknown long option flag --bogus
ADD_TRY_HARD_FAIL_TEST(tryhard_fail_unknown_long_opt_bogus --bogus 1 0 -D -- true)
