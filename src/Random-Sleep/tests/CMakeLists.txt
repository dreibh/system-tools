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
# Random-Sleep Test Suite
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


#############################################################################
#### Helper Functions                                                    ####
#############################################################################

# ###### Helper for deterministic commands (expected return code: 0) ########
FUNCTION(ADD_RANDOM_SLEEP_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:random-sleep> ${ARGN}
   )
   SET(TEST_LABELS "random-sleep;cli")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


# ###### Helper for expected failures (expected return code: non-zero) ######
FUNCTION(ADD_RANDOM_SLEEP_FAIL_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:random-sleep> ${ARGN}
   )
   SET(TEST_LABELS "random-sleep;negative;error")
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

ADD_RANDOM_SLEEP_TEST(randomsleep_cli_version_short -v)
ADD_RANDOM_SLEEP_TEST(randomsleep_cli_version_long  --version)
ADD_RANDOM_SLEEP_TEST(randomsleep_cli_help_short    -h)
ADD_RANDOM_SLEEP_TEST(randomsleep_cli_help_long     --help)

SET_TESTS_PROPERTIES(randomsleep_cli_version_short randomsleep_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "random-sleep [0-9]+\\.[0-9]+"
)

SET_TESTS_PROPERTIES(randomsleep_cli_help_short randomsleep_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# === CLI Help Internationalization =========================================
ADD_RANDOM_SLEEP_TEST(randomsleep_cli_help_i18n_de -h)
ADD_RANDOM_SLEEP_TEST(randomsleep_cli_help_i18n_nb -h)

SET_TESTS_PROPERTIES(randomsleep_cli_help_i18n_de PROPERTIES
   ENVIRONMENT "LC_ALL=en_US.UTF-8;LANGUAGE=de;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Aufruf:"
   LABELS "random-sleep;cli;i18n;locale_de"
)

SET_TESTS_PROPERTIES(randomsleep_cli_help_i18n_nb PROPERTIES
   ENVIRONMENT "LC_ALL=en_US.UTF-8;LANGUAGE=nb;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Bruk:"
   LABELS "random-sleep;cli;i18n;locale_nb"
)


#############################################################################
#### Functional Execution Tests                                          ####
#############################################################################

# Zero delay: Instantaneous completion with default quiet mode (no output)
ADD_RANDOM_SLEEP_TEST(randomsleep_zero_delay_default 0 0)
SET_TESTS_PROPERTIES(randomsleep_zero_delay_default PROPERTIES
   FAIL_REGULAR_EXPRESSION "Sleeping for|Woke up!"
   LABELS "random-sleep;zero;quiet"
)

# Explicit quiet flags: -q and --quiet
ADD_RANDOM_SLEEP_TEST(randomsleep_zero_delay_quiet_short -q 0 0)
ADD_RANDOM_SLEEP_TEST(randomsleep_zero_delay_quiet_long  --quiet 0 0)
SET_TESTS_PROPERTIES(randomsleep_zero_delay_quiet_short randomsleep_zero_delay_quiet_long PROPERTIES
   FAIL_REGULAR_EXPRESSION "Sleeping for|Woke up!"
   LABELS "random-sleep;quiet"
)

# Verbose mode: Fixed delay verification (0.000000 s)
ADD_RANDOM_SLEEP_TEST(randomsleep_verbose_short -w 0 0)
ADD_RANDOM_SLEEP_TEST(randomsleep_verbose_long  --verbose 0 0)
SET_TESTS_PROPERTIES(randomsleep_verbose_short randomsleep_verbose_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Sleeping for 0\\.000000 s \\.\\.\\..*Woke up!"
   LABELS "random-sleep;verbose"
)

# Fractional seconds parsing (sub-millisecond delay)
ADD_RANDOM_SLEEP_TEST(randomsleep_fractional_delay -w 0.0001 0.0001)
SET_TESTS_PROPERTIES(randomsleep_fractional_delay PROPERTIES
   PASS_REGULAR_EXPRESSION "Sleeping for 0\\.000100 s \\.\\.\\..*Woke up!"
   LABELS "random-sleep;fractional;verbose"
)

# Range generation: min < max (0.0001 s to 0.0005 s)
ADD_RANDOM_SLEEP_TEST(randomsleep_range_delay -w 0.0001 0.0005)
SET_TESTS_PROPERTIES(randomsleep_range_delay PROPERTIES
   PASS_REGULAR_EXPRESSION "Sleeping for 0\\.000[1-5][0-9][0-9] s \\.\\.\\..*Woke up!"
   LABELS "random-sleep;range;verbose"
)

# Flag override: -w followed by -q remains quiet
ADD_RANDOM_SLEEP_TEST(randomsleep_flag_override_quiet -w -q 0 0)
SET_TESTS_PROPERTIES(randomsleep_flag_override_quiet PROPERTIES
   FAIL_REGULAR_EXPRESSION "Sleeping for|Woke up!"
   LABELS "random-sleep;flags"
)

# Flag override: -q followed by -w remains verbose
ADD_RANDOM_SLEEP_TEST(randomsleep_flag_override_verbose -q -w 0 0)
SET_TESTS_PROPERTIES(randomsleep_flag_override_verbose PROPERTIES
   PASS_REGULAR_EXPRESSION "Sleeping for 0\\.000000 s \\.\\.\\..*Woke up!"
   LABELS "random-sleep;flags"
)


#############################################################################
#### Error Handling & Validation                                         ####
#############################################################################

# Missing and excess arguments
ADD_RANDOM_SLEEP_FAIL_TEST(randomsleep_fail_no_args)
ADD_RANDOM_SLEEP_FAIL_TEST(randomsleep_fail_one_arg          0.5)
ADD_RANDOM_SLEEP_FAIL_TEST(randomsleep_fail_three_args        0.1 0.5 1.0)
ADD_RANDOM_SLEEP_FAIL_TEST(randomsleep_fail_unknown_flag     --invalid-option 0 1)

# Inverted bounds (min_delay > max_delay)
ADD_RANDOM_SLEEP_FAIL_TEST(randomsleep_fail_inverted_bounds  5.0 2.0)

# Negative values
ADD_RANDOM_SLEEP_FAIL_TEST(randomsleep_fail_negative_min    -0.5 1.0)
ADD_RANDOM_SLEEP_FAIL_TEST(randomsleep_fail_negative_both   -2.0 -1.0)

# Non-numeric arguments and trailing junk
ADD_RANDOM_SLEEP_FAIL_TEST(randomsleep_fail_alpha_min        abc 1.0)
ADD_RANDOM_SLEEP_FAIL_TEST(randomsleep_fail_alpha_max        0.0 xyz)
ADD_RANDOM_SLEEP_FAIL_TEST(randomsleep_fail_trailing_chars   0s 1s)

# Non-finite values (NaN / Inf)
ADD_RANDOM_SLEEP_FAIL_TEST(randomsleep_fail_nan              nan 1.0)
