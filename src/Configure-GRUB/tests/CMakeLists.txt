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
# Configure-GRUB Test Suite
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

# Find bash interpreter for deterministic script execution
FIND_PROGRAM(BASH_EXECUTABLE bash REQUIRED)

# Resolve path to configured Configure-GRUB script
IF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/../Configure-GRUB.versioned")
   SET(CONFIGURE_GRUB_BIN "${CMAKE_CURRENT_BINARY_DIR}/../Configure-GRUB.versioned")
ELSEIF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/Configure-GRUB.versioned")
   SET(CONFIGURE_GRUB_BIN "${CMAKE_CURRENT_BINARY_DIR}/Configure-GRUB.versioned")
ELSEIF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/../Configure-GRUB")
   SET(CONFIGURE_GRUB_BIN "${CMAKE_CURRENT_BINARY_DIR}/../Configure-GRUB")
ELSEIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Configure-GRUB")
   SET(CONFIGURE_GRUB_BIN "${CMAKE_CURRENT_SOURCE_DIR}/../Configure-GRUB")
ELSEIF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/../configure-grub.versioned")
   SET(CONFIGURE_GRUB_BIN "${CMAKE_CURRENT_BINARY_DIR}/../configure-grub.versioned")
ELSEIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../configure-grub")
   SET(CONFIGURE_GRUB_BIN "${CMAKE_CURRENT_SOURCE_DIR}/../configure-grub")
ELSE()
   SET(CONFIGURE_GRUB_BIN "Configure-GRUB")
ENDIF()

# ===========================================================================
# Deterministic Test Fixtures (Mock GRUB Configurations)
# ===========================================================================

# Standard template file with placeholder and commented options
SET(MOCK_TEMPLATE_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_grub_template")
FILE(WRITE "${MOCK_TEMPLATE_FILE}"
"# Standard GRUB template\n"
"GRUB_DEFAULT=0\n"
"GRUB_TIMEOUT=5\n"
"GRUB_DISTRIBUTOR=`lsb_release -i -s 2> /dev/null || echo Debian`\n"
"GRUB_CMDLINE_LINUX_DEFAULT=\"quiet splash\"\n"
"GRUB_CMDLINE_LINUX=\"\"\n"
"#GRUB_GFXMODE=640x480\n"
"#GRUB_INIT_TUNE=\"480 440 1\"\n"
)

# Minimal template file without optional settings
SET(MOCK_MINIMAL_TEMPLATE_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_grub_minimal_template")
FILE(WRITE "${MOCK_MINIMAL_TEMPLATE_FILE}"
"GRUB_DEFAULT=0\n"
"GRUB_TIMEOUT=3\n"
)

# Template file explicitly defining cryptodisk
SET(MOCK_CRYPTO_TEMPLATE_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_grub_crypto_template")
FILE(WRITE "${MOCK_CRYPTO_TEMPLATE_FILE}"
"GRUB_DEFAULT=0\n"
"GRUB_ENABLE_CRYPTODISK=n\n"
)

# Existing input GRUB configuration fixture
SET(MOCK_INPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_grub_input")
FILE(WRITE "${MOCK_INPUT_FILE}"
"# Existing GRUB config\n"
"GRUB_DEFAULT=saved\n"
"GRUB_SAVEDEFAULT=\"true\"\n"
"GRUB_TIMEOUT=10\n"
"GRUB_CMDLINE_LINUX_DEFAULT=\"quiet splash acpi=force\"\n"
"GRUB_CMDLINE_LINUX=\"console=ttyS0,115200\"\n"
"GRUB_ENABLE_CRYPTODISK=y\n"
"GRUB_GFXMODE=\"1920x1080,auto\"\n"
"GRUB_INIT_TUNE=\"1000 440 2\"\n"
)

# Existing input configuration containing network interface naming arguments
SET(MOCK_INPUT_NETNAMES_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_grub_input_netnames")
FILE(WRITE "${MOCK_INPUT_NETNAMES_FILE}"
"GRUB_CMDLINE_LINUX=\"biosdevname=0 net.ifnames=0 console=tty0\"\n"
"GRUB_CMDLINE_LINUX_DEFAULT=\"quiet biosdevname=0 splash\"\n"
)

# Empty input configuration fixture
SET(MOCK_EMPTY_INPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_grub_input_empty")
FILE(WRITE "${MOCK_EMPTY_INPUT_FILE}" "")


#############################################################################
#### Helper Functions                                                    ####
#############################################################################

# Helper for commands expected to succeed (exit code 0)
FUNCTION(ADD_CONFIG_GRUB_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${BASH_EXECUTABLE} "${CONFIGURE_GRUB_BIN}" ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "Configure-GRUB;cli"
   )
ENDFUNCTION()

# Helper for commands verified by PASS_REGULAR_EXPRESSION (success or error)
FUNCTION(ADD_CONFIG_GRUB_MATCH_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${BASH_EXECUTABLE} "${CONFIGURE_GRUB_BIN}" ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "Configure-GRUB;cli"
   )
ENDFUNCTION()

# Helper for commands asserting non-zero exit code without matching specific output
FUNCTION(ADD_CONFIG_GRUB_FAIL_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${BASH_EXECUTABLE} "${CONFIGURE_GRUB_BIN}" ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "LC_ALL=C.UTF-8"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "Configure-GRUB;negative;error"
   )
ENDFUNCTION()


#############################################################################
#### 1. Basic CLI Options                                                ####
#############################################################################

# Test version flag (short option -v)[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_cli_version_short -v)

# Test version flag (long option --version)[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_cli_version_long --version)
SET_TESTS_PROPERTIES(configgrub_cli_version_short configgrub_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "configure-grub (@BUILD_MAJOR@|[0-9]+)"
)

# Test help flag (short option -h)[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_cli_help_short -h)

# Test help flag (long option --help)[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_cli_help_long --help)
SET_TESTS_PROPERTIES(configgrub_cli_help_short configgrub_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)


#############################################################################
#### 2. Configuration Merging: Input and Template Files                  ####
#############################################################################

SET(TEST_OUT_MERGE "${CMAKE_CURRENT_BINARY_DIR}/out_grub_merge")

# Test merging template and existing input using short options -i and -o[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_merge_short_opts
   -i "${MOCK_INPUT_FILE}" -o "${TEST_OUT_MERGE}" "${MOCK_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_merge_short_opts PROPERTIES
   PASS_REGULAR_EXPRESSION "Configuration:.*OPTION_CMDLINE:.*console=ttyS0,115200"
   LABELS "Configure-GRUB;merge"
)

SET(TEST_OUT_LONG_OPTS "${CMAKE_CURRENT_BINARY_DIR}/out_grub_long_opts")

# Test merging template and existing input using long options --input and --output[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_merge_long_opts
   --input "${MOCK_INPUT_FILE}" --output "${TEST_OUT_LONG_OPTS}" "${MOCK_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_merge_long_opts PROPERTIES
   PASS_REGULAR_EXPRESSION "Configuration:.*OPTION_CMDLINE_DEFAULT:.*acpi=force"
   LABELS "Configure-GRUB;merge"
)

SET(TEST_OUT_DEVNULL "${CMAKE_CURRENT_BINARY_DIR}/out_grub_devnull")

# Test merging template with /dev/null input[cite: 28]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_merge_input_devnull
   -i "/dev/null" -o "${TEST_OUT_DEVNULL}" "${MOCK_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_merge_input_devnull PROPERTIES
   PASS_REGULAR_EXPRESSION "Configuration:.*Input file:[ \t]+/dev/null"
   LABELS "Configure-GRUB;merge"
)

SET(TEST_OUT_EMPTY "${CMAKE_CURRENT_BINARY_DIR}/out_grub_empty")

# Test merging template with an empty input file[cite: 28]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_merge_input_empty
   -i "${MOCK_EMPTY_INPUT_FILE}" -o "${TEST_OUT_EMPTY}" "${MOCK_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_merge_input_empty PROPERTIES
   PASS_REGULAR_EXPRESSION "Configuration:.*OPTION_CMDLINE:[ \t]+GRUB_CMDLINE_LINUX=\"\""
   LABELS "Configure-GRUB;merge"
)

SET(TEST_OUT_CRYPTO "${CMAKE_CURRENT_BINARY_DIR}/out_grub_crypto")

# Test preserving GRUB_ENABLE_CRYPTODISK=y from input configuration[cite: 28]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_merge_cryptodisk
   -i "${MOCK_INPUT_FILE}" -o "${TEST_OUT_CRYPTO}" "${MOCK_CRYPTO_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_merge_cryptodisk PROPERTIES
   PASS_REGULAR_EXPRESSION "OPTION_ENABLE_CRYPTODISK:[ \t]+GRUB_ENABLE_CRYPTODISK=y"
   LABELS "Configure-GRUB;merge;crypto"
)

SET(TEST_OUT_NETNAMES "${CMAKE_CURRENT_BINARY_DIR}/out_grub_netnames")

# Test stripping biosdevname=0 and net.ifnames=0 when absent from /proc/cmdline[cite: 28]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_strip_netnames
   -i "${MOCK_INPUT_NETNAMES_FILE}" -o "${TEST_OUT_NETNAMES}" "${MOCK_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_strip_netnames PROPERTIES
   PASS_REGULAR_EXPRESSION "OPTION_CMDLINE:[ \t]+GRUB_CMDLINE_LINUX=\"console=tty0\""
   LABELS "Configure-GRUB;merge;cmdline"
)


#############################################################################
#### 3. Video and Tune Parameter Overrides (-V, -T)                      ####
#############################################################################

SET(TEST_OUT_VIDEO_SHORT "${CMAKE_CURRENT_BINARY_DIR}/out_grub_video_short")

# Test overriding video mode with short option (-V)[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_override_video_short
   -i "${MOCK_INPUT_FILE}" -o "${TEST_OUT_VIDEO_SHORT}"
   -V "auto,1920x1080,1024x768" "${MOCK_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_override_video_short PROPERTIES
   PASS_REGULAR_EXPRESSION "OPTION_GFXMODE:[ \t]+GRUB_GFXMODE=\"auto,1920x1080,1024x768\""
   LABELS "Configure-GRUB;video"
)

SET(TEST_OUT_VIDEO_LONG "${CMAKE_CURRENT_BINARY_DIR}/out_grub_video_long")

# Test overriding video mode with long option (--video)[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_override_video_long
   -i "${MOCK_INPUT_FILE}" -o "${TEST_OUT_VIDEO_LONG}"
   --video "1280x1024" "${MOCK_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_override_video_long PROPERTIES
   PASS_REGULAR_EXPRESSION "OPTION_GFXMODE:[ \t]+GRUB_GFXMODE=\"1280x1024\""
   LABELS "Configure-GRUB;video"
)

SET(TEST_OUT_TUNE_SHORT "${CMAKE_CURRENT_BINARY_DIR}/out_grub_tune_short")

# Test overriding startup tune with short option (-T)[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_override_tune_short
   -i "${MOCK_INPUT_FILE}" -o "${TEST_OUT_TUNE_SHORT}"
   -T "20000 584 144 736 48" "${MOCK_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_override_tune_short PROPERTIES
   PASS_REGULAR_EXPRESSION "OPTION_INIT_TUNE:[ \t]+GRUB_INIT_TUNE=\"20000 584 144 736 48\""
   LABELS "Configure-GRUB;tune"
)

SET(TEST_OUT_TUNE_LONG "${CMAKE_CURRENT_BINARY_DIR}/out_grub_tune_long")

# Test overriding startup tune with long option (--tune)[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_override_tune_long
   -i "${MOCK_INPUT_FILE}" -o "${TEST_OUT_TUNE_LONG}"
   --tune "480 440 1" "${MOCK_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_override_tune_long PROPERTIES
   PASS_REGULAR_EXPRESSION "OPTION_INIT_TUNE:[ \t]+GRUB_INIT_TUNE=\"480 440 1\""
   LABELS "Configure-GRUB;tune"
)

SET(TEST_OUT_VT_COMBINED "${CMAKE_CURRENT_BINARY_DIR}/out_grub_vt_combined")

# Test combining video mode and startup tune overrides with short options
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_override_video_and_tune_short
   -i "${MOCK_INPUT_FILE}" -o "${TEST_OUT_VT_COMBINED}"
   -V "1024x768" -T "1000 440 2" "${MOCK_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_override_video_and_tune_short PROPERTIES
   PASS_REGULAR_EXPRESSION "OPTION_INIT_TUNE:[ \t]+GRUB_INIT_TUNE=\"1000 440 2\".*OPTION_GFXMODE:[ \t]+GRUB_GFXMODE=\"1024x768\""
   LABELS "Configure-GRUB;video;tune"
)

SET(TEST_OUT_VT_COMBINED_LONG "${CMAKE_CURRENT_BINARY_DIR}/out_grub_vt_combined_long")

# Test combining video mode and startup tune overrides with long options
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_override_video_and_tune_long
   --input "${MOCK_INPUT_FILE}" --output "${TEST_OUT_VT_COMBINED_LONG}"
   --video "1920x1080" --tune "2000 440 1" "${MOCK_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_override_video_and_tune_long PROPERTIES
   PASS_REGULAR_EXPRESSION "OPTION_INIT_TUNE:[ \t]+GRUB_INIT_TUNE=\"2000 440 1\".*OPTION_GFXMODE:[ \t]+GRUB_GFXMODE=\"1920x1080\""
   LABELS "Configure-GRUB;video;tune"
)


#############################################################################
#### 4. File Appending & Minimal Template Expansion                      ####
#############################################################################

SET(TEST_OUT_MINIMAL "${CMAKE_CURRENT_BINARY_DIR}/out_grub_minimal")

# Test appending settings when absent from minimal template[cite: 28]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_minimal_template_appends
   -i "${MOCK_INPUT_FILE}" -o "${TEST_OUT_MINIMAL}" "${MOCK_MINIMAL_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_minimal_template_appends PROPERTIES
   PASS_REGULAR_EXPRESSION "Differences between.*and ${TEST_OUT_MINIMAL}"
   LABELS "Configure-GRUB;merge"
)


#############################################################################
#### 5. Command-Line Error Handling & Negative Boundaries                ####
#############################################################################

# ------ Missing Positional Arguments ---------------------------------------
# Test error when invoked with no arguments[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_fail_missing_all_args)
SET_TESTS_PROPERTIES(configgrub_fail_missing_all_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error when only options are passed without template argument[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_fail_only_options
   -i "${MOCK_INPUT_FILE}" -o "out.txt"
)
SET_TESTS_PROPERTIES(configgrub_fail_only_options PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# ------ Extraneous Positional Arguments ------------------------------------
# Test error on two positional arguments[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_fail_extraneous_two_args
   "${MOCK_TEMPLATE_FILE}" extra_arg
)
SET_TESTS_PROPERTIES(configgrub_fail_extraneous_two_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error on three positional arguments[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_fail_extraneous_multiple_args
   "${MOCK_TEMPLATE_FILE}" extra1 extra2
)
SET_TESTS_PROPERTIES(configgrub_fail_extraneous_multiple_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test error on double-dash followed by extra positional arguments[cite: 28, 29]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_fail_extraneous_with_delimiter
   -- "${MOCK_TEMPLATE_FILE}" extra
)
SET_TESTS_PROPERTIES(configgrub_fail_extraneous_with_delimiter PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# ------ Non-Existent Files -------------------------------------------------
# Test error when input file does not exist[cite: 28]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_fail_nonexistent_input
   -i "/nonexistent/input/path" "${MOCK_TEMPLATE_FILE}"
)
SET_TESTS_PROPERTIES(configgrub_fail_nonexistent_input PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Input configuration /nonexistent/input/path not found!"
)

# Test error when template file does not exist[cite: 28]
ADD_CONFIG_GRUB_MATCH_TEST(configgrub_fail_nonexistent_template
   -i "${MOCK_INPUT_FILE}" "/nonexistent/template/path"
)
SET_TESTS_PROPERTIES(configgrub_fail_nonexistent_template PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Template configuration /nonexistent/template/path not found!"
)

# ------ Missing Required Option Arguments ----------------------------------
# Test error when -i is passed without argument[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_missing_opt_arg_i -i)

# Test error when -o is passed without argument[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_missing_opt_arg_o -o)

# Test error when -V is passed without argument[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_missing_opt_arg_V -V)

# Test error when -T is passed without argument[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_missing_opt_arg_T -T)

# Test error when --input is passed without argument[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_missing_opt_arg_input --input)

# Test error when --output is passed without argument[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_missing_opt_arg_output --output)

# Test error when --video is passed without argument[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_missing_opt_arg_video --video)

# Test error when --tune is passed without argument[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_missing_opt_arg_tune --tune)

# ------ Disallowed Arguments to Flag Options -------------------------------
# Test error when --help is passed with an argument[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_help_with_arg --help=foo)

# Test error when --version is passed with an argument[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_version_with_arg --version=foo)

# ------ Unrecognized Options -----------------------------------------------
# Test error on unrecognized short option -z[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_unknown_short_opt_z -z "${MOCK_TEMPLATE_FILE}")

# Test error on unrecognized short option -a[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_unknown_short_opt_a -a "${MOCK_TEMPLATE_FILE}")

# Test error on unrecognized long option --invalid-option[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_unknown_long_opt --invalid-option "${MOCK_TEMPLATE_FILE}")

# Test error on unrecognized long option --custom-flag[cite: 28]
ADD_CONFIG_GRUB_FAIL_TEST(configgrub_fail_unknown_long_opt_custom --custom-flag "${MOCK_TEMPLATE_FILE}")
