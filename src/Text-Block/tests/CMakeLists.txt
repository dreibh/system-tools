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
# Text-Block Test Suite
# Copyright (C) 2025-2026 by Thomas Dreibholz
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

# ===========================================================================
# Deterministic Test Fixtures (Mock Inputs)
# ===========================================================================
SET(MOCK_LINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_lines.txt")
FILE(WRITE "${MOCK_LINES_FILE}"
"Line 1\n"
"Line 2\n"
"Line 3\n"
"Line 4\n"
"Line 5\n"
)

SET(MOCK_LONG_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_long.txt")
FILE(WRITE "${MOCK_LONG_FILE}"
"Row 01\n"
"Row 02\n"
"Row 03\n"
"Row 04\n"
"Row 05\n"
"Row 06\n"
"Row 07\n"
"Row 08\n"
"Row 09\n"
"Row 10\n"
)

SET(MOCK_TAGGED_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_tagged.txt")
FILE(WRITE "${MOCK_TAGGED_FILE}"
"Header line\n"
"<BEGIN>\n"
"Block line 1\n"
"Block line 2\n"
"<END>\n"
"Footer line\n"
)

SET(MOCK_MULTI_TAG_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_multitag.txt")
FILE(WRITE "${MOCK_MULTI_TAG_FILE}"
"Header\n"
"<BEGIN>\n"
"Block 1\n"
"<END>\n"
"Middle\n"
"<BEGIN>\n"
"Block 2\n"
"<END>\n"
"Footer\n"
)

SET(MOCK_THREE_BLOCKS_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_three_blocks.txt")
FILE(WRITE "${MOCK_THREE_BLOCKS_FILE}"
"Top\n"
"<B>\n"
"One\n"
"</B>\n"
"Mid1\n"
"<B>\n"
"Two\n"
"</B>\n"
"Mid2\n"
"<B>\n"
"Three\n"
"</B>\n"
"Bottom\n"
)

SET(MOCK_SINGLE_TAG_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_singletag.txt")
FILE(WRITE "${MOCK_SINGLE_TAG_FILE}"
"Line A\n"
"<MARKER>\n"
"Line B\n"
)

SET(MOCK_MULTI_BLOCK_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_multiblock.txt")
FILE(WRITE "${MOCK_MULTI_BLOCK_FILE}"
"Intro [B]First[/B] Middle [B]Second[/B] Outro\n"
)

SET(MOCK_CODE_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_code.txt")
FILE(WRITE "${MOCK_CODE_FILE}"
"// Header\n"
"#ifdef DEBUG\n"
"debug_log();\n"
"#endif\n"
"// Footer\n"
)

SET(MOCK_XML_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_xml.txt")
FILE(WRITE "${MOCK_XML_FILE}"
"<root>\n"
"<!--BEGIN-->\n"
"<item>Payload</item>\n"
"<!--END-->\n"
"</root>\n"
)

SET(MOCK_UNICODE_TAGS_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_unicode_tags.txt")
FILE(WRITE "${MOCK_UNICODE_TAGS_FILE}"
"Preamble\n"
"🟢\n"
"Unicode Payload æøå\n"
"🔴\n"
"Postscript\n"
)

SET(MOCK_ADJACENT_TAGS_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_adjacent_tags.txt")
FILE(WRITE "${MOCK_ADJACENT_TAGS_FILE}"
"Prefix<BEGIN><END>Suffix\n"
)

SET(MOCK_UNCLOSED_TAG_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_unclosed_tag.txt")
FILE(WRITE "${MOCK_UNCLOSED_TAG_FILE}"
"Header line\n"
"<BEGIN>\n"
"Unclosed line 1\n"
"Unclosed line 2\n"
)

SET(MOCK_INSERT_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_insert.txt")
FILE(WRITE "${MOCK_INSERT_FILE}"
"INSERTED_PAYLOAD\n"
)

SET(MOCK_EMPTY_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_empty.txt")
FILE(WRITE "${MOCK_EMPTY_FILE}" "")

# 8192-byte line to test buffer boundary (> 4096 bytes readLine buffer)
SET(LONG_CHUNK "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF")
SET(LONG_8K "${LONG_CHUNK}")
FOREACH(I RANGE 1 7)
   STRING(APPEND LONG_8K "${LONG_8K}")
ENDFOREACH()
SET(MOCK_LONGLINE_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_longline.txt")
FILE(WRITE "${MOCK_LONGLINE_FILE}"
"Start line\n"
"<BEGIN>\n"
"${LONG_8K}\n"
"<END>\n"
"Finish line\n"
)

# Dedicated mock files for in-place updates to avoid concurrent test race conditions
SET(TEST_INPLACE_FILE "${CMAKE_CURRENT_BINARY_DIR}/inplace_target.txt")
FILE(WRITE "${TEST_INPLACE_FILE}" "Alpha\nBeta\nGamma\nDelta\n")

SET(TEST_INPLACE_DEL_FILE "${CMAKE_CURRENT_BINARY_DIR}/inplace_del.txt")
FILE(WRITE "${TEST_INPLACE_DEL_FILE}" "Top\n<B>\nPayload\n</B>\nBottom\n")

SET(TEST_INPLACE_REP_FILE "${CMAKE_CURRENT_BINARY_DIR}/inplace_rep.txt")
FILE(WRITE "${TEST_INPLACE_REP_FILE}" "Top\n<B>\nPayload\n</B>\nBottom\n")

SET(TEST_INPLACE_INSF_FILE "${CMAKE_CURRENT_BINARY_DIR}/inplace_insf.txt")
FILE(WRITE "${TEST_INPLACE_INSF_FILE}" "Top\n<B>\nPayload\n</B>\nBottom\n")

SET(TEST_INPLACE_INSB_FILE "${CMAKE_CURRENT_BINARY_DIR}/inplace_insb.txt")
FILE(WRITE "${TEST_INPLACE_INSB_FILE}" "Top\n<B>\nPayload\n</B>\nBottom\n")

SET(TEST_INPLACE_MB_FILE "${CMAKE_CURRENT_BINARY_DIR}/inplace_mb.txt")
FILE(WRITE "${TEST_INPLACE_MB_FILE}" "Start [B]First[/B] Mid [B]Second[/B] End\n")

# ===========================================================================
# Deterministic Stdin Test Runners
# ===========================================================================
SET(RUN_STDIN_INSERT_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/run_stdin_insert.cmake")
FILE(WRITE "${RUN_STDIN_INSERT_SCRIPT}"
"execute_process(\n"
"   COMMAND \"\${PROG}\" -i \"\${INPUT_FILE}\" \"\${MODE_OPT}\" \"-\" \${EXTRA_ARGS}\n"
"   INPUT_FILE \"\${INSERT_PAYLOAD_FILE}\"\n"
"   RESULT_VARIABLE RES\n"
"   OUTPUT_VARIABLE OUT\n"
"   ERROR_VARIABLE ERR\n"
")\n"
"if(NOT RES EQUAL 0)\n"
"   message(FATAL_ERROR \"Process failed with code \${RES}: \${ERR}\")\n"
"endif()\n"
"if(DEFINED EXPECTED_MATCH AND NOT OUT MATCHES \"\${EXPECTED_MATCH}\")\n"
"   message(FATAL_ERROR \"Output did not match '\${EXPECTED_MATCH}'. Output: \${OUT}\")\n"
"endif()\n"
)

SET(RUN_STDIN_INPUT_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/run_stdin_input.cmake")
FILE(WRITE "${RUN_STDIN_INPUT_SCRIPT}"
"execute_process(\n"
"   COMMAND \"\${PROG}\" \${CMD_ARGS}\n"
"   INPUT_FILE \"\${STDIN_SOURCE_FILE}\"\n"
"   RESULT_VARIABLE RES\n"
"   OUTPUT_VARIABLE OUT\n"
"   ERROR_VARIABLE ERR\n"
")\n"
"if(NOT RES EQUAL EXPECTED_CODE)\n"
"   message(FATAL_ERROR \"Exit code \${RES} did not match expected \${EXPECTED_CODE}: \${ERR}\")\n"
"endif()\n"
"if(DEFINED EXPECTED_MATCH AND NOT OUT MATCHES \"\${EXPECTED_MATCH}\")\n"
"   message(FATAL_ERROR \"Output did not match '\${EXPECTED_MATCH}'. Output: \${OUT}\")\n"
"endif()\n"
)


#############################################################################
#### Helper Functions                                                    ####
#############################################################################

# ###### Helper for deterministic commands (expected return code: 0) ########
FUNCTION(ADD_TEXT_BLOCK_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:text-block> ${ARGN}
   )
   SET(TEST_LABELS "text-block;cli")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()

# ###### Helper for expected failures (expected return code: non-zero) ######
FUNCTION(ADD_TEXT_BLOCK_FAIL_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:text-block> ${ARGN}
   )
   SET(TEST_LABELS "text-block;negative;error")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


#############################################################################
#### 1. Basic CLI Options & Internationalization                         ####
#############################################################################

# Test version flag (short option -v)
ADD_TEXT_BLOCK_TEST(textblock_cli_version_short -v)

# Test version flag (long option --version)
ADD_TEXT_BLOCK_TEST(textblock_cli_version_long  --version)

# Test help flag (short option -h)
ADD_TEXT_BLOCK_TEST(textblock_cli_help_short    -h)

# Test help flag (long option --help)
ADD_TEXT_BLOCK_TEST(textblock_cli_help_long     --help)

SET_TESTS_PROPERTIES(textblock_cli_version_short textblock_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "text-block [0-9]+\\.[0-9]+"
)
SET_TESTS_PROPERTIES(textblock_cli_help_short textblock_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test localized help in German (de)
ADD_TEXT_BLOCK_TEST(textblock_cli_help_i18n_de -h)

# Test localized help in Norwegian Bokmål (nb)
ADD_TEXT_BLOCK_TEST(textblock_cli_help_i18n_nb -h)

SET_TESTS_PROPERTIES(textblock_cli_help_i18n_de PROPERTIES
   ENVIRONMENT "LANGUAGE=de;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Aufruf:"
   LABELS "text-block;cli;i18n;locale_de"
)
SET_TESTS_PROPERTIES(textblock_cli_help_i18n_nb PROPERTIES
   ENVIRONMENT "LANGUAGE=nb;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Bruk:"
   LABELS "text-block;cli;i18n;locale_nb"
)


#############################################################################
#### 2. Cat and Discard Modes                                            ####
#############################################################################

# Test Cat mode using short flag -C
ADD_TEXT_BLOCK_TEST(textblock_cat_short   -i "${MOCK_LINES_FILE}" -C)

# Test Cat mode using long flag --cat
ADD_TEXT_BLOCK_TEST(textblock_cat_long    -i "${MOCK_LINES_FILE}" --cat)

# Test Cat mode by default when no mode option is provided
ADD_TEXT_BLOCK_TEST(textblock_cat_default -i "${MOCK_LINES_FILE}")

# Test Cat mode on an empty file fixture
ADD_TEXT_BLOCK_TEST(textblock_cat_empty   -i "${MOCK_EMPTY_FILE}" -C)

SET_TESTS_PROPERTIES(textblock_cat_short textblock_cat_long textblock_cat_default PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 2.*Line 3.*Line 4.*Line 5"
   LABELS "text-block;cat"
)
SET_TESTS_PROPERTIES(textblock_cat_empty PROPERTIES
   FAIL_REGULAR_EXPRESSION "Line|Header|Footer"
   LABELS "text-block;cat"
)

# Test Discard mode using short flag -0
ADD_TEXT_BLOCK_TEST(textblock_discard_short -i "${MOCK_LINES_FILE}" -0)

# Test Discard mode using long flag --discard
ADD_TEXT_BLOCK_TEST(textblock_discard_long  -i "${MOCK_LINES_FILE}" --discard)

# Test Discard mode on an empty file
ADD_TEXT_BLOCK_TEST(textblock_discard_empty -i "${MOCK_EMPTY_FILE}" -0)

SET_TESTS_PROPERTIES(textblock_discard_short textblock_discard_long textblock_discard_empty PROPERTIES
   FAIL_REGULAR_EXPRESSION "Line [1-5]"
   LABELS "text-block;discard"
)


#############################################################################
#### 3. Enumerate Mode Formatting Variants                               ####
#############################################################################

# Test Enumerate mode with default formatting (short flag -E)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_default_short -i "${MOCK_LINES_FILE}" -E)

# Test Enumerate mode with default formatting (long flag --enumerate)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_default_long  -i "${MOCK_LINES_FILE}" --enumerate)
SET_TESTS_PROPERTIES(textblock_enumerate_default_short textblock_enumerate_default_long PROPERTIES
   PASS_REGULAR_EXPRESSION "000001.*Line 1.*000005.*Line 5"
   LABELS "text-block;enumerate"
)

# Test unpadded single-digit format width 1
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_1 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 1 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_1 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1: Line 1.*#5: Line 5"
   LABELS "text-block;enumerate"
)

# Test 2-digit zero-padded format (02)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_02 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 02 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_02 PROPERTIES
   PASS_REGULAR_EXPRESSION "#01: Line 1.*#05: Line 5"
   LABELS "text-block;enumerate"
)

# Test 3-digit zero-padded format (03)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_03 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 03 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_03 PROPERTIES
   PASS_REGULAR_EXPRESSION "#001: Line 1.*#005: Line 5"
   LABELS "text-block;enumerate"
)

# Test 4-digit zero-padded format (04)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_04 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 04 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_04 PROPERTIES
   PASS_REGULAR_EXPRESSION "#0001: Line 1.*#0005: Line 5"
   LABELS "text-block;enumerate"
)

# Test 5-digit zero-padded format (05)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_05 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 05 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_05 PROPERTIES
   PASS_REGULAR_EXPRESSION "#00001: Line 1.*#00005: Line 5"
   LABELS "text-block;enumerate"
)

# Test 6-digit zero-padded format (06)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_06 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 06 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_06 PROPERTIES
   PASS_REGULAR_EXPRESSION "#000001: Line 1.*#000005: Line 5"
   LABELS "text-block;enumerate"
)

# Test 7-digit zero-padded format (07)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_07 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 07 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_07 PROPERTIES
   PASS_REGULAR_EXPRESSION "#0000001: Line 1.*#0000005: Line 5"
   LABELS "text-block;enumerate"
)

# Test 8-digit zero-padded format (08)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_08 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 08 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_08 PROPERTIES
   PASS_REGULAR_EXPRESSION "#00000001: Line 1.*#00000005: Line 5"
   LABELS "text-block;enumerate"
)

# Test 9-digit zero-padded format (09)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_09 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 09 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_09 PROPERTIES
   PASS_REGULAR_EXPRESSION "#000000001: Line 1.*#000000005: Line 5"
   LABELS "text-block;enumerate"
)

# Test 10-digit space-padded format (10)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_10 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 10 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_10 PROPERTIES
   PASS_REGULAR_EXPRESSION "#         1: Line 1.*#         5: Line 5"
   LABELS "text-block;enumerate"
)

# Test 2-digit space-padded format (2)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_2 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 2 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_2 PROPERTIES
   PASS_REGULAR_EXPRESSION "# 1: Line 1.*# 5: Line 5"
   LABELS "text-block;enumerate"
)

# Test 3-digit space-padded format (3)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_3 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 3 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_3 PROPERTIES
   PASS_REGULAR_EXPRESSION "#  1: Line 1.*#  5: Line 5"
   LABELS "text-block;enumerate"
)

# Test 4-digit space-padded format (4)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_4 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 4 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_4 PROPERTIES
   PASS_REGULAR_EXPRESSION "#   1: Line 1.*#   5: Line 5"
   LABELS "text-block;enumerate"
)

# Test 5-digit space-padded format (5)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_5 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 5 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_5 PROPERTIES
   PASS_REGULAR_EXPRESSION "#    1: Line 1.*#    5: Line 5"
   LABELS "text-block;enumerate"
)

# Test 6-digit space-padded format (6)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_6 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 6 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_6 PROPERTIES
   PASS_REGULAR_EXPRESSION "#     1: Line 1.*#     5: Line 5"
   LABELS "text-block;enumerate"
)

# Test 7-digit space-padded format (7)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_7 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 7 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_7 PROPERTIES
   PASS_REGULAR_EXPRESSION "#      1: Line 1.*#      5: Line 5"
   LABELS "text-block;enumerate"
)

# Test 8-digit space-padded format (8)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_8 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 8 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_8 PROPERTIES
   PASS_REGULAR_EXPRESSION "#       1: Line 1.*#       5: Line 5"
   LABELS "text-block;enumerate"
)

# Test 9-digit space-padded format (9)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_9 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 9 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_9 PROPERTIES
   PASS_REGULAR_EXPRESSION "#        1: Line 1.*#        5: Line 5"
   LABELS "text-block;enumerate"
)

# Test left-adjusted 1-digit format (-1)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_left_1 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format -1 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_left_1 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1: Line 1.*#5: Line 5"
   LABELS "text-block;enumerate"
)

# Test left-adjusted 2-digit format (-2)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_left_2 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format -2 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_left_2 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1 : Line 1.*#5 : Line 5"
   LABELS "text-block;enumerate"
)

# Test left-adjusted 3-digit format (-3)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_left_3 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format -3 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_left_3 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1  : Line 1.*#5  : Line 5"
   LABELS "text-block;enumerate"
)

# Test left-adjusted 4-digit format (-4)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_left_4 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format -4 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_left_4 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1   : Line 1.*#5   : Line 5"
   LABELS "text-block;enumerate"
)

# Test left-adjusted 5-digit format (-5)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_left_5 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format -5 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_left_5 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1    : Line 1.*#5    : Line 5"
   LABELS "text-block;enumerate"
)

# Test left-adjusted 6-digit format (-6)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_left_6 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format -6 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_left_6 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1     : Line 1.*#5     : Line 5"
   LABELS "text-block;enumerate"
)

# Test left-adjusted 7-digit format (-7)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_left_7 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format -7 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_left_7 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1      : Line 1.*#5      : Line 5"
   LABELS "text-block;enumerate"
)

# Test left-adjusted 8-digit format (-8)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_left_8 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format -8 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_left_8 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1       : Line 1.*#5       : Line 5"
   LABELS "text-block;enumerate"
)

# Test left-adjusted 10-digit format (-10)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_left_10 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format -10 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_left_10 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1         : Line 1.*#5         : Line 5"
   LABELS "text-block;enumerate"
)

# Test custom prefix label string (--enumerate-label1)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_label1_prefix -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 2 --enumerate-label1 "ROW_"
)
SET_TESTS_PROPERTIES(textblock_enumerate_label1_prefix PROPERTIES
   PASS_REGULAR_EXPRESSION "ROW_ 1.*ROW_ 5"
   LABELS "text-block;enumerate"
)

# Test custom separator pipe label (--enumerate-label2)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_label2_pipe -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 2 --enumerate-label2 " | "
)
SET_TESTS_PROPERTIES(textblock_enumerate_label2_pipe PROPERTIES
   PASS_REGULAR_EXPRESSION " 1 \\| Line 1.* 5 \\| Line 5"
   LABELS "text-block;enumerate"
)

# Test custom arrow style numbering labels
ADD_TEXT_BLOCK_TEST(textblock_enumerate_arrow_labels -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 3 --enumerate-label1 "--> " --enumerate-label2 " <-- "
)
SET_TESTS_PROPERTIES(textblock_enumerate_arrow_labels PROPERTIES
   PASS_REGULAR_EXPRESSION "-->   1 <-- Line 1.*-->   5 <-- Line 5"
   LABELS "text-block;enumerate"
)

# Test XML-attribute style enumeration labels
ADD_TEXT_BLOCK_TEST(textblock_enumerate_xml_tags -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 2 --enumerate-label1 "<line n=\"" --enumerate-label2 "\"> "
)
SET_TESTS_PROPERTIES(textblock_enumerate_xml_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "<line n=\" 1\"> Line 1.*<line n=\" 5\"> Line 5"
   LABELS "text-block;enumerate"
)

# Test parenthesis/bracket style enumeration labels
ADD_TEXT_BLOCK_TEST(textblock_enumerate_bracket_labels -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 2 --enumerate-label1 "(#" --enumerate-label2 ") "
)
SET_TESTS_PROPERTIES(textblock_enumerate_bracket_labels PROPERTIES
   PASS_REGULAR_EXPRESSION "\\(# 1\\) Line 1.*\\(# 5\\) Line 5"
   LABELS "text-block;enumerate"
)

# Test curly brace style enumeration labels
ADD_TEXT_BLOCK_TEST(textblock_enumerate_brace_labels -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 2 --enumerate-label1 "{" --enumerate-label2 "}: "
)
SET_TESTS_PROPERTIES(textblock_enumerate_brace_labels PROPERTIES
   PASS_REGULAR_EXPRESSION "{ 1}: Line 1.*{ 5}: Line 5"
   LABELS "text-block;enumerate"
)

# Test enumeration mode on an empty file fixture
ADD_TEXT_BLOCK_TEST(textblock_enumerate_empty_file -i "${MOCK_EMPTY_FILE}" -E)
SET_TESTS_PROPERTIES(textblock_enumerate_empty_file PROPERTIES
   FAIL_REGULAR_EXPRESSION "000001|Line"
   LABELS "text-block;enumerate"
)


#############################################################################
#### 4. Tag-Based Extraction (-X / --extract)                            ####
#############################################################################

# Test extraction with default tags-only and excluded tags (short option -X)
ADD_TEXT_BLOCK_TEST(textblock_extract_tags_only_exclude_short
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>"
)

# Test extraction with explicit long options --extract, --tags-only, --exclude-tags
ADD_TEXT_BLOCK_TEST(textblock_extract_tags_only_exclude_long
   -i "${MOCK_TAGGED_FILE}" --extract --begin-tag "<BEGIN>" --end-tag "<END>" --tags-only --exclude-tags
)
SET_TESTS_PROPERTIES(textblock_extract_tags_only_exclude_short textblock_extract_tags_only_exclude_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 1.*Block line 2"
   FAIL_REGULAR_EXPRESSION "Header line|Footer line|<BEGIN>|<END>"
   LABELS "text-block;extract"
)

# Test extraction with tags-only including marker tags (-y -g)
ADD_TEXT_BLOCK_TEST(textblock_extract_tags_only_include
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -y -g
)
SET_TESTS_PROPERTIES(textblock_extract_tags_only_include PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>.*Block line 1.*Block line 2.*<END>"
   FAIL_REGULAR_EXPRESSION "Header line|Footer line"
   LABELS "text-block;extract"
)

# Test extraction with full tag lines excluding tags (-f -x)
ADD_TEXT_BLOCK_TEST(textblock_extract_full_lines_exclude
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -f -x
)
SET_TESTS_PROPERTIES(textblock_extract_full_lines_exclude PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 1.*Block line 2"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Header|Footer"
   LABELS "text-block;extract"
)

# Test extraction with full tag lines including tags (-f -y)
ADD_TEXT_BLOCK_TEST(textblock_extract_full_lines_include
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_extract_full_lines_include PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>.*Block line 1.*Block line 2.*<END>"
   FAIL_REGULAR_EXPRESSION "Header line|Footer line"
   LABELS "text-block;extract"
)

# Test extracting multiple separated blocks across file
ADD_TEXT_BLOCK_TEST(textblock_extract_multiple_blocks
   -i "${MOCK_MULTI_TAG_FILE}" -X -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_extract_multiple_blocks PROPERTIES
   PASS_REGULAR_EXPRESSION "Block 1.*Block 2"
   FAIL_REGULAR_EXPRESSION "Header|Middle|Footer"
   LABELS "text-block;extract"
)

# Test extracting multiple separated blocks including tags
ADD_TEXT_BLOCK_TEST(textblock_extract_multiple_blocks_included
   -i "${MOCK_MULTI_TAG_FILE}" -X -b "<BEGIN>" -e "<END>" -y
)
SET_TESTS_PROPERTIES(textblock_extract_multiple_blocks_included PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>.*Block 1.*<END>.*<BEGIN>.*Block 2.*<END>"
   FAIL_REGULAR_EXPRESSION "Header|Middle|Footer"
   LABELS "text-block;extract"
)

# Test extracting 3 distinct blocks with full tag lines
ADD_TEXT_BLOCK_TEST(textblock_extract_three_blocks_full_lines
   -i "${MOCK_THREE_BLOCKS_FILE}" -X -b "<B>" -e "</B>" -f -y
)
SET_TESTS_PROPERTIES(textblock_extract_three_blocks_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "<B>.*One.*</B>.*<B>.*Two.*</B>.*<B>.*Three.*</B>"
   FAIL_REGULAR_EXPRESSION "Top|Mid1|Mid2|Bottom"
   LABELS "text-block;extract"
)

# Test extracting 3 distinct blocks with tags-only excluded
ADD_TEXT_BLOCK_TEST(textblock_extract_three_blocks_tags_only
   -i "${MOCK_THREE_BLOCKS_FILE}" -X -b "<B>" -e "</B>" -g -x
)
SET_TESTS_PROPERTIES(textblock_extract_three_blocks_tags_only PROPERTIES
   PASS_REGULAR_EXPRESSION "One.*Two.*Three"
   FAIL_REGULAR_EXPRESSION "<B>|</B>|Top|Mid1|Mid2|Bottom"
   LABELS "text-block;extract"
)

# Test extracting multiple inline blocks from a single line
ADD_TEXT_BLOCK_TEST(textblock_extract_inline_multiblock
   -i "${MOCK_MULTI_BLOCK_FILE}" -X -b "[B]" -e "[/B]"
)
SET_TESTS_PROPERTIES(textblock_extract_inline_multiblock PROPERTIES
   PASS_REGULAR_EXPRESSION "FirstSecond"
   FAIL_REGULAR_EXPRESSION "Intro|Middle|Outro|\\[B\\]|\\[/B\\]"
   LABELS "text-block;extract"
)

# Test extracting combined single marker tag with include-tags
ADD_TEXT_BLOCK_TEST(textblock_extract_single_tag_include
   -i "${MOCK_SINGLE_TAG_FILE}" -X -t "<MARKER>" -y
)
SET_TESTS_PROPERTIES(textblock_extract_single_tag_include PROPERTIES
   PASS_REGULAR_EXPRESSION "<MARKER>"
   FAIL_REGULAR_EXPRESSION "Line A|Line B"
   LABELS "text-block;extract"
)

# Test extracting block delineated by Unicode emoji tags
ADD_TEXT_BLOCK_TEST(textblock_extract_unicode_emoji_tags
   -i "${MOCK_UNICODE_TAGS_FILE}" -X -b "🟢" -e "🔴"
)
SET_TESTS_PROPERTIES(textblock_extract_unicode_emoji_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Unicode Payload æøå"
   FAIL_REGULAR_EXPRESSION "Preamble|Postscript|🟢|🔴"
   LABELS "text-block;extract;unicode"
)

# Test extraction when specified tags do not exist in the file
ADD_TEXT_BLOCK_TEST(textblock_extract_no_matches
   -i "${MOCK_TAGGED_FILE}" -X -b "<ABSENT>" -e "</ABSENT>"
)
SET_TESTS_PROPERTIES(textblock_extract_no_matches PROPERTIES
   FAIL_REGULAR_EXPRESSION "Header|Block|Footer"
   LABELS "text-block;extract"
)

# Test adjacent zero-length tags extraction with exclude-tags
ADD_TEXT_BLOCK_TEST(textblock_extract_adjacent_tags_exclude
   -i "${MOCK_ADJACENT_TAGS_FILE}" -X -b "<BEGIN>" -e "<END>" -x
)
SET_TESTS_PROPERTIES(textblock_extract_adjacent_tags_exclude PROPERTIES
   FAIL_REGULAR_EXPRESSION "Prefix|Suffix|<BEGIN>|<END>"
   LABELS "text-block;extract"
)

# Test adjacent zero-length tags extraction with include-tags
ADD_TEXT_BLOCK_TEST(textblock_extract_adjacent_tags_include
   -i "${MOCK_ADJACENT_TAGS_FILE}" -X -b "<BEGIN>" -e "<END>" -y
)
SET_TESTS_PROPERTIES(textblock_extract_adjacent_tags_include PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN><END>"
   FAIL_REGULAR_EXPRESSION "Prefix|Suffix"
   LABELS "text-block;extract"
)


#############################################################################
#### 5. Tag-Based Deletion / Removal (-D / --delete / --remove)          ####
#############################################################################

# Test Delete mode keeping tags (short option -D -x)
ADD_TEXT_BLOCK_TEST(textblock_delete_keep_tags_short -i "${MOCK_TAGGED_FILE}" -D -b "<BEGIN>" -e "<END>" -x)

# Test Delete mode keeping tags (long option --delete --exclude-tags)
ADD_TEXT_BLOCK_TEST(textblock_delete_keep_tags_long  -i "${MOCK_TAGGED_FILE}" --delete -b "<BEGIN>" -e "<END>" --exclude-tags)

# Test Remove mode alias keeping tags (long option --remove --exclude-tags)
ADD_TEXT_BLOCK_TEST(textblock_remove_keep_tags_long  -i "${MOCK_TAGGED_FILE}" --remove -b "<BEGIN>" -e "<END>" --exclude-tags)
SET_TESTS_PROPERTIES(textblock_delete_keep_tags_short textblock_delete_keep_tags_long textblock_remove_keep_tags_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line.*<BEGIN><END>.*Footer line"
   FAIL_REGULAR_EXPRESSION "Block line"
   LABELS "text-block;delete"
)

# Test Delete mode deleting block and tags together (-y)
ADD_TEXT_BLOCK_TEST(textblock_delete_include_tags
   -i "${MOCK_TAGGED_FILE}" -D -b "<BEGIN>" -e "<END>" -y
)
SET_TESTS_PROPERTIES(textblock_delete_include_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line.*Footer line"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block line"
   LABELS "text-block;delete"
)

# Test Delete mode removing full lines of marker tags (-f -y)
ADD_TEXT_BLOCK_TEST(textblock_delete_full_lines
   -i "${MOCK_TAGGED_FILE}" --delete -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_delete_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line\nFooter line"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block line"
   LABELS "text-block;delete"
)

# Test Delete mode on multiple distinct blocks across file
ADD_TEXT_BLOCK_TEST(textblock_delete_multiple_blocks
   -i "${MOCK_MULTI_TAG_FILE}" -D -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_delete_multiple_blocks PROPERTIES
   PASS_REGULAR_EXPRESSION "Header\nMiddle\nFooter"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block 1|Block 2"
   LABELS "text-block;delete"
)

# Test Delete mode on 3 distinct blocks across file
ADD_TEXT_BLOCK_TEST(textblock_delete_three_blocks
   -i "${MOCK_THREE_BLOCKS_FILE}" -D -b "<B>" -e "</B>" -f -y
)
SET_TESTS_PROPERTIES(textblock_delete_three_blocks PROPERTIES
   PASS_REGULAR_EXPRESSION "Top.*Mid1.*Mid2.*Bottom"
   FAIL_REGULAR_EXPRESSION "<B>|</B>|One|Two|Three"
   LABELS "text-block;delete"
)

# Test Delete mode on multiple inline blocks including tags on same line
ADD_TEXT_BLOCK_TEST(textblock_delete_inline_multiblock
   -i "${MOCK_MULTI_BLOCK_FILE}" -D -b "[B]" -e "[/B]" -y
)
SET_TESTS_PROPERTIES(textblock_delete_inline_multiblock PROPERTIES
   PASS_REGULAR_EXPRESSION "Intro  Middle  Outro"
   FAIL_REGULAR_EXPRESSION "First|Second|\\[B\\]|\\[/B\\]"
   LABELS "text-block;delete"
)

# Test Delete mode on multiple inline blocks keeping tags on same line
ADD_TEXT_BLOCK_TEST(textblock_delete_inline_keep_tags
   -i "${MOCK_MULTI_BLOCK_FILE}" -D -b "[B]" -e "[/B]" -x
)
SET_TESTS_PROPERTIES(textblock_delete_inline_keep_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Intro \\[B\\]\\[/B\\] Middle \\[B\\]\\[/B\\] Outro"
   FAIL_REGULAR_EXPRESSION "First|Second"
   LABELS "text-block;delete"
)

# Test Delete mode removing single combined marker tag line (-t -y -f)
ADD_TEXT_BLOCK_TEST(textblock_delete_single_tag
   -i "${MOCK_SINGLE_TAG_FILE}" -D -t "<MARKER>" -y -f
)
SET_TESTS_PROPERTIES(textblock_delete_single_tag PROPERTIES
   PASS_REGULAR_EXPRESSION "Line A\nLine B"
   FAIL_REGULAR_EXPRESSION "<MARKER>"
   LABELS "text-block;delete"
)

# Test Delete mode removing block marked by Unicode emojis
ADD_TEXT_BLOCK_TEST(textblock_delete_unicode_tags
   -i "${MOCK_UNICODE_TAGS_FILE}" -D -b "🟢" -e "🔴" -f -y
)
SET_TESTS_PROPERTIES(textblock_delete_unicode_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Preamble\nPostscript"
   FAIL_REGULAR_EXPRESSION "🟢|🔴|Unicode Payload"
   LABELS "text-block;delete;unicode"
)

# Test Delete mode on adjacent zero-length tags with tags included
ADD_TEXT_BLOCK_TEST(textblock_delete_adjacent_tags_include
   -i "${MOCK_ADJACENT_TAGS_FILE}" -D -b "<BEGIN>" -e "<END>" -y
)
SET_TESTS_PROPERTIES(textblock_delete_adjacent_tags_include PROPERTIES
   PASS_REGULAR_EXPRESSION "PrefixSuffix"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>"
   LABELS "text-block;delete"
)


#############################################################################
#### 6. Tag-Based Insertion & Replacement (-F, -B, -R)                   ####
#############################################################################

# Test Insert Front mode (short option -F)
ADD_TEXT_BLOCK_TEST(textblock_insert_front_short
   -i "${MOCK_TAGGED_FILE}" -F "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)

# Test Insert Front mode (long option --insert-front)
ADD_TEXT_BLOCK_TEST(textblock_insert_front_long
   -i "${MOCK_TAGGED_FILE}" --insert-front "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_insert_front_short textblock_insert_front_long PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>INSERTED_PAYLOAD.*Block line 1"
   LABELS "text-block;insert"
)

# Test Insert Front mode with full tag lines (-f)
ADD_TEXT_BLOCK_TEST(textblock_insert_front_full_lines
   -i "${MOCK_TAGGED_FILE}" -F "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -f
)
SET_TESTS_PROPERTIES(textblock_insert_front_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>\nINSERTED_PAYLOAD\nBlock line 1"
   LABELS "text-block;insert"
)

# Test Insert Front across multiple blocks with full tag lines
ADD_TEXT_BLOCK_TEST(textblock_insert_front_multitag
   -i "${MOCK_MULTI_TAG_FILE}" -F "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -f
)
SET_TESTS_PROPERTIES(textblock_insert_front_multitag PROPERTIES
   PASS_REGULAR_EXPRESSION "INSERTED_PAYLOAD\nBlock 1.*INSERTED_PAYLOAD\nBlock 2"
   LABELS "text-block;insert"
)

# Test Insert Front across 3 blocks with full tag lines
ADD_TEXT_BLOCK_TEST(textblock_insert_front_three_blocks
   -i "${MOCK_THREE_BLOCKS_FILE}" -F "${MOCK_INSERT_FILE}" -b "<B>" -e "</B>" -f
)
SET_TESTS_PROPERTIES(textblock_insert_front_three_blocks PROPERTIES
   PASS_REGULAR_EXPRESSION "INSERTED_PAYLOAD\nOne.*INSERTED_PAYLOAD\nTwo.*INSERTED_PAYLOAD\nThree"
   LABELS "text-block;insert"
)

# Test Insert Back mode (short option -B)
ADD_TEXT_BLOCK_TEST(textblock_insert_back_short
   -i "${MOCK_TAGGED_FILE}" -B "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)

# Test Insert Back mode (long option --insert-back)
ADD_TEXT_BLOCK_TEST(textblock_insert_back_long
   -i "${MOCK_TAGGED_FILE}" --insert-back "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_insert_back_short textblock_insert_back_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 2.*INSERTED_PAYLOAD.*<END>"
   LABELS "text-block;insert"
)

# Test Insert Back mode with full tag lines (-f)
ADD_TEXT_BLOCK_TEST(textblock_insert_back_full_lines
   -i "${MOCK_TAGGED_FILE}" -B "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -f
)
SET_TESTS_PROPERTIES(textblock_insert_back_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 2\nINSERTED_PAYLOAD\n<END>"
   LABELS "text-block;insert"
)

# Test Insert Back across multiple blocks with full tag lines
ADD_TEXT_BLOCK_TEST(textblock_insert_back_multitag
   -i "${MOCK_MULTI_TAG_FILE}" -B "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -f
)
SET_TESTS_PROPERTIES(textblock_insert_back_multitag PROPERTIES
   PASS_REGULAR_EXPRESSION "Block 1\nINSERTED_PAYLOAD\n<END>.*Block 2\nINSERTED_PAYLOAD\n<END>"
   LABELS "text-block;insert"
)

# Test Insert Back across 3 blocks with full tag lines
ADD_TEXT_BLOCK_TEST(textblock_insert_back_three_blocks
   -i "${MOCK_THREE_BLOCKS_FILE}" -B "${MOCK_INSERT_FILE}" -b "<B>" -e "</B>" -f
)
SET_TESTS_PROPERTIES(textblock_insert_back_three_blocks PROPERTIES
   PASS_REGULAR_EXPRESSION "One\nINSERTED_PAYLOAD\n</B>.*Two\nINSERTED_PAYLOAD\n</B>.*Three\nINSERTED_PAYLOAD\n</B>"
   LABELS "text-block;insert"
)

# Test Replace mode keeping tags (-R -x)
ADD_TEXT_BLOCK_TEST(textblock_replace_keep_tags
   -i "${MOCK_TAGGED_FILE}" -R "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -x
)
SET_TESTS_PROPERTIES(textblock_replace_keep_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>INSERTED_PAYLOAD.*<END>"
   FAIL_REGULAR_EXPRESSION "Block line"
   LABELS "text-block;replace"
)

# Test Replace mode replacing block content and tags (-R -y)
ADD_TEXT_BLOCK_TEST(textblock_replace_include_tags
   -i "${MOCK_TAGGED_FILE}" --replace "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -y
)
SET_TESTS_PROPERTIES(textblock_replace_include_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line.*INSERTED_PAYLOAD.*Footer line"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block line"
   LABELS "text-block;replace"
)

# Test Replace mode replacing full tag lines (-R -f -y)
ADD_TEXT_BLOCK_TEST(textblock_replace_full_lines
   -i "${MOCK_TAGGED_FILE}" -R "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_replace_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line\nINSERTED_PAYLOAD\nFooter line"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block line"
   LABELS "text-block;replace"
)

# Test Replace mode on multiple blocks across file
ADD_TEXT_BLOCK_TEST(textblock_replace_multitag
   -i "${MOCK_MULTI_TAG_FILE}" -R "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_replace_multitag PROPERTIES
   PASS_REGULAR_EXPRESSION "Header\nINSERTED_PAYLOAD\nMiddle\nINSERTED_PAYLOAD\nFooter"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block 1|Block 2"
   LABELS "text-block;replace"
)

# Test Replace mode on 3 blocks with full tag lines
ADD_TEXT_BLOCK_TEST(textblock_replace_three_blocks
   -i "${MOCK_THREE_BLOCKS_FILE}" -R "${MOCK_INSERT_FILE}" -b "<B>" -e "</B>" -f -y
)
SET_TESTS_PROPERTIES(textblock_replace_three_blocks PROPERTIES
   PASS_REGULAR_EXPRESSION "Top\nINSERTED_PAYLOAD\nMid1\nINSERTED_PAYLOAD\nMid2\nINSERTED_PAYLOAD\nBottom"
   FAIL_REGULAR_EXPRESSION "<B>|</B>|One|Two|Three"
   LABELS "text-block;replace"
)

# Test Insert Front at single marker tag position (-t)
ADD_TEXT_BLOCK_TEST(textblock_single_tag_insert_front
   -i "${MOCK_SINGLE_TAG_FILE}" -F "${MOCK_INSERT_FILE}" -t "<MARKER>"
)
SET_TESTS_PROPERTIES(textblock_single_tag_insert_front PROPERTIES
   PASS_REGULAR_EXPRESSION "Line A.*<MARKER>INSERTED_PAYLOAD.*Line B"
   LABELS "text-block;insert"
)

# Test Insert Back at single marker tag position (-t)
ADD_TEXT_BLOCK_TEST(textblock_single_tag_insert_back
   -i "${MOCK_SINGLE_TAG_FILE}" -B "${MOCK_INSERT_FILE}" -t "<MARKER>"
)
SET_TESTS_PROPERTIES(textblock_single_tag_insert_back PROPERTIES
   PASS_REGULAR_EXPRESSION "Line A.*<MARKER>INSERTED_PAYLOAD.*Line B"
   LABELS "text-block;insert"
)

# Test Replace at single marker tag position (-t -y)
ADD_TEXT_BLOCK_TEST(textblock_single_tag_replace_tag
   -i "${MOCK_SINGLE_TAG_FILE}" -R "${MOCK_INSERT_FILE}" -t "<MARKER>" -y
)
SET_TESTS_PROPERTIES(textblock_single_tag_replace_tag PROPERTIES
   PASS_REGULAR_EXPRESSION "Line A.*INSERTED_PAYLOAD.*Line B"
   FAIL_REGULAR_EXPRESSION "<MARKER>"
   LABELS "text-block;replace"
)

# Test Insert Front on multiple inline blocks on same line
ADD_TEXT_BLOCK_TEST(textblock_insert_front_inline_multiblock
   -i "${MOCK_MULTI_BLOCK_FILE}" -F "${MOCK_INSERT_FILE}" -b "[B]" -e "[/B]"
)
SET_TESTS_PROPERTIES(textblock_insert_front_inline_multiblock PROPERTIES
   PASS_REGULAR_EXPRESSION "\\[B\\]INSERTED_PAYLOAD.*First.*\\[B\\]INSERTED_PAYLOAD.*Second"
   LABELS "text-block;insert"
)

# Test Insert Back on multiple inline blocks on same line
ADD_TEXT_BLOCK_TEST(textblock_insert_back_inline_multiblock
   -i "${MOCK_MULTI_BLOCK_FILE}" -B "${MOCK_INSERT_FILE}" -b "[B]" -e "[/B]"
)
SET_TESTS_PROPERTIES(textblock_insert_back_inline_multiblock PROPERTIES
   PASS_REGULAR_EXPRESSION "FirstINSERTED_PAYLOAD.*\\[/B\\].*SecondINSERTED_PAYLOAD"
   LABELS "text-block;insert"
)

# Test Replace on multiple inline blocks on same line
ADD_TEXT_BLOCK_TEST(textblock_replace_inline_multiblock
   -i "${MOCK_MULTI_BLOCK_FILE}" -R "${MOCK_INSERT_FILE}" -b "[B]" -e "[/B]" -y
)
SET_TESTS_PROPERTIES(textblock_replace_inline_multiblock PROPERTIES
   PASS_REGULAR_EXPRESSION "Intro INSERTED_PAYLOAD.*Middle INSERTED_PAYLOAD.*Outro"
   FAIL_REGULAR_EXPRESSION "First|Second|\\[B\\]|\\[/B\\]"
   LABELS "text-block;replace"
)

# Test Replace with Unicode emoji tags
ADD_TEXT_BLOCK_TEST(textblock_replace_unicode_tags
   -i "${MOCK_UNICODE_TAGS_FILE}" -R "${MOCK_INSERT_FILE}" -b "🟢" -e "🔴" -f -y
)
SET_TESTS_PROPERTIES(textblock_replace_unicode_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Preamble\nINSERTED_PAYLOAD\nPostscript"
   FAIL_REGULAR_EXPRESSION "🟢|🔴|Unicode Payload"
   LABELS "text-block;replace;unicode"
)

# Test Insert Front on adjacent zero-length tags
ADD_TEXT_BLOCK_TEST(textblock_insert_front_adjacent_tags
   -i "${MOCK_ADJACENT_TAGS_FILE}" -F "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_insert_front_adjacent_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Prefix<BEGIN>INSERTED_PAYLOAD.*<END>Suffix"
   LABELS "text-block;insert"
)

# Test Insert Back on adjacent zero-length tags
ADD_TEXT_BLOCK_TEST(textblock_insert_back_adjacent_tags
   -i "${MOCK_ADJACENT_TAGS_FILE}" -B "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_insert_back_adjacent_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Prefix<BEGIN>INSERTED_PAYLOAD.*<END>Suffix"
   LABELS "text-block;insert"
)

# Test Replace on adjacent zero-length tags with tags included
ADD_TEXT_BLOCK_TEST(textblock_replace_adjacent_tags_include
   -i "${MOCK_ADJACENT_TAGS_FILE}" -R "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -y
)
SET_TESTS_PROPERTIES(textblock_replace_adjacent_tags_include PROPERTIES
   PASS_REGULAR_EXPRESSION "PrefixINSERTED_PAYLOAD.*Suffix"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>"
   LABELS "text-block;replace"
)


#############################################################################
#### 7. Highlight Mode (-H / --highlight)                                ####
#############################################################################

# Test Highlight mode with default emojis (short option -H)
ADD_TEXT_BLOCK_TEST(textblock_highlight_default_short -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>")

# Test Highlight mode with default emojis (long option --highlight)
ADD_TEXT_BLOCK_TEST(textblock_highlight_default_long  -i "${MOCK_TAGGED_FILE}" --highlight -b "<BEGIN>" -e "<END>")
SET_TESTS_PROPERTIES(textblock_highlight_default_short textblock_highlight_default_long PROPERTIES
   PASS_REGULAR_EXPRESSION "⭐.*Block line 1.*Block line 2.*🛑"
   LABELS "text-block;highlight"
)

# Test custom begin/end marker strings in Highlight mode
ADD_TEXT_BLOCK_TEST(textblock_highlight_custom_begin_end
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-begin "[[BEGIN]]"
   --highlight-end "[[END]]"
)
SET_TESTS_PROPERTIES(textblock_highlight_custom_begin_end PROPERTIES
   PASS_REGULAR_EXPRESSION "\\[\\[BEGIN\\]\\].*Block line 1.*\\[\\[END\\]\\]"
   LABELS "text-block;highlight"
)

# Test custom unmarked text styling strings in Highlight mode
ADD_TEXT_BLOCK_TEST(textblock_highlight_custom_unmarked
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-unmarked1 "<U>"
   --highlight-unmarked2 "</U>"
)
SET_TESTS_PROPERTIES(textblock_highlight_custom_unmarked PROPERTIES
   PASS_REGULAR_EXPRESSION "<U>Header line\n</U>.*<U>Footer line\n</U>"
   LABELS "text-block;highlight"
)

# Test custom marked text styling strings in Highlight mode
ADD_TEXT_BLOCK_TEST(textblock_highlight_custom_marked
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-marked1 "<M>"
   --highlight-marked2 "</M>"
)
SET_TESTS_PROPERTIES(textblock_highlight_custom_marked PROPERTIES
   PASS_REGULAR_EXPRESSION "<M>Block line 1\n</M><M>Block line 2\n</M>"
   LABELS "text-block;highlight"
)

# Test setting all six individual custom highlight labels simultaneously
ADD_TEXT_BLOCK_TEST(textblock_highlight_custom_all_six
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-begin "{START}"
   --highlight-end "{STOP}"
   --highlight-unmarked1 "(u:"
   --highlight-unmarked2 ":u)"
   --highlight-marked1 "(m:"
   --highlight-marked2 ":m)"
)
SET_TESTS_PROPERTIES(textblock_highlight_custom_all_six PROPERTIES
   PASS_REGULAR_EXPRESSION "\\(u:Header line.*{START}.*\\(m:Block line 1.*{STOP}.*\\(u:Footer line"
   LABELS "text-block;highlight"
)

# Test 6-parameter highlight batch shortcut option (--highlight-param)
ADD_TEXT_BLOCK_TEST(textblock_highlight_param_shortcut
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-param "{START}" "{STOP}" "[u]" "[/u]" "[m]" "[/m]"
)
SET_TESTS_PROPERTIES(textblock_highlight_param_shortcut PROPERTIES
   PASS_REGULAR_EXPRESSION "\\[u\\]Header line.*\\[u\\]<BEGIN>\\[/u\\]{START}\\[m\\].*{STOP}\\[u\\]<END>"
   LABELS "text-block;highlight"
)

# Test 6-parameter highlight batch shortcut alias option (--highlight-params)
ADD_TEXT_BLOCK_TEST(textblock_highlight_params_shortcut
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-params "{START}" "{STOP}" "[u]" "[/u]" "[m]" "[/m]"
)
SET_TESTS_PROPERTIES(textblock_highlight_params_shortcut PROPERTIES
   PASS_REGULAR_EXPRESSION "\\[u\\]Header line.*\\[u\\]<BEGIN>\\[/u\\]{START}\\[m\\].*{STOP}\\[u\\]<END>"
   LABELS "text-block;highlight"
)

# Test Highlight mode on a single combined marker tag (-t)
ADD_TEXT_BLOCK_TEST(textblock_highlight_single_tag
   -i "${MOCK_SINGLE_TAG_FILE}" -H -t "<MARKER>"
)
SET_TESTS_PROPERTIES(textblock_highlight_single_tag PROPERTIES
   PASS_REGULAR_EXPRESSION "<MARKER>.*⭐🛑.*Line B"
   LABELS "text-block;highlight"
)

# Test Highlight mode driven by line range selection (-s)
ADD_TEXT_BLOCK_TEST(textblock_highlight_line_selection
   -i "${MOCK_LINES_FILE}" -H -s 2 3
)
SET_TESTS_PROPERTIES(textblock_highlight_line_selection PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*⭐.*Line 2.*Line 3.*🛑.*Line 4"
   LABELS "text-block;highlight"
)

# Test Highlight mode with full tag lines enabled (-f)
ADD_TEXT_BLOCK_TEST(textblock_highlight_full_lines
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" -f
)
SET_TESTS_PROPERTIES(textblock_highlight_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>.*⭐.*Block line.*🛑.*<END>"
   LABELS "text-block;highlight"
)

# Test Highlight mode across multiple separated blocks in a file
ADD_TEXT_BLOCK_TEST(textblock_highlight_multitag
   -i "${MOCK_MULTI_TAG_FILE}" -H -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_highlight_multitag PROPERTIES
   PASS_REGULAR_EXPRESSION "⭐.*Block 1.*🛑.*⭐.*Block 2.*🛑"
   LABELS "text-block;highlight"
)

# Test Highlight mode on multiple inline blocks within a single line
ADD_TEXT_BLOCK_TEST(textblock_highlight_inline_multiblock
   -i "${MOCK_MULTI_BLOCK_FILE}" -H -b "[B]" -e "[/B]"
)
SET_TESTS_PROPERTIES(textblock_highlight_inline_multiblock PROPERTIES
   PASS_REGULAR_EXPRESSION "⭐.*First.*🛑.*⭐.*Second.*🛑"
   LABELS "text-block;highlight"
)

# Test Highlight mode on adjacent zero-length tags
ADD_TEXT_BLOCK_TEST(textblock_highlight_adjacent_tags
   -i "${MOCK_ADJACENT_TAGS_FILE}" -H -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_highlight_adjacent_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Prefix.*<BEGIN>.*⭐🛑.*<END>.*Suffix"
   LABELS "text-block;highlight"
)


#############################################################################
#### 8. Line Range Selection: Positive, Negative & Mixed Ranges          ####
#############################################################################

# Test selecting entire file with Extract (-s 1 0)
ADD_TEXT_BLOCK_TEST(textblock_select_entire_file_extract
   -i "${MOCK_LINES_FILE}" -s 1 0 -X
)
SET_TESTS_PROPERTIES(textblock_select_entire_file_extract PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 2.*Line 3.*Line 4.*Line 5"
   LABELS "text-block;select"
)

# Test deleting entire file with Delete (-s 1 0)
ADD_TEXT_BLOCK_TEST(textblock_select_entire_file_delete
   -i "${MOCK_LINES_FILE}" -s 1 0 -D
)
SET_TESTS_PROPERTIES(textblock_select_entire_file_delete PROPERTIES
   FAIL_REGULAR_EXPRESSION "Line [1-5]"
   LABELS "text-block;select"
)

# Test selecting single line 1
ADD_TEXT_BLOCK_TEST(textblock_select_line_1_only -i "${MOCK_LINES_FILE}" -s 1 1 -X)
SET_TESTS_PROPERTIES(textblock_select_line_1_only PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1"
   FAIL_REGULAR_EXPRESSION "Line 2|Line 3|Line 4|Line 5"
   LABELS "text-block;select"
)

# Test selecting single line 2
ADD_TEXT_BLOCK_TEST(textblock_select_line_2_only -i "${MOCK_LINES_FILE}" -s 2 2 -X)
SET_TESTS_PROPERTIES(textblock_select_line_2_only PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 2"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 3|Line 4|Line 5"
   LABELS "text-block;select"
)

# Test selecting single line 3
ADD_TEXT_BLOCK_TEST(textblock_select_line_3_only -i "${MOCK_LINES_FILE}" -s 3 3 -X)
SET_TESTS_PROPERTIES(textblock_select_line_3_only PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 3"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2|Line 4|Line 5"
   LABELS "text-block;select"
)

# Test selecting single line 4
ADD_TEXT_BLOCK_TEST(textblock_select_line_4_only -i "${MOCK_LINES_FILE}" -s 4 4 -X)
SET_TESTS_PROPERTIES(textblock_select_line_4_only PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 4"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2|Line 3|Line 5"
   LABELS "text-block;select"
)

# Test selecting single line 5
ADD_TEXT_BLOCK_TEST(textblock_select_line_5_only -i "${MOCK_LINES_FILE}" -s 5 5 -X)
SET_TESTS_PROPERTIES(textblock_select_line_5_only PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2|Line 3|Line 4"
   LABELS "text-block;select"
)

# Test extracting positive line range 1 to 3
ADD_TEXT_BLOCK_TEST(textblock_select_range_1_to_3 -i "${MOCK_LINES_FILE}" -s 1 3 -X)
SET_TESTS_PROPERTIES(textblock_select_range_1_to_3 PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 2.*Line 3"
   FAIL_REGULAR_EXPRESSION "Line 4|Line 5"
   LABELS "text-block;select"
)

# Test extracting positive line range 2 to 4
ADD_TEXT_BLOCK_TEST(textblock_select_range_2_to_4 -i "${MOCK_LINES_FILE}" -s 2 4 -X)
SET_TESTS_PROPERTIES(textblock_select_range_2_to_4 PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 2.*Line 3.*Line 4"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 5"
   LABELS "text-block;select"
)

# Test extracting positive line range 3 to 5
ADD_TEXT_BLOCK_TEST(textblock_select_range_3_to_5 -i "${MOCK_LINES_FILE}" -s 3 5 -X)
SET_TESTS_PROPERTIES(textblock_select_range_3_to_5 PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 3.*Line 4.*Line 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2"
   LABELS "text-block;select"
)

# Test deleting positive line range 1 to 2
ADD_TEXT_BLOCK_TEST(textblock_select_delete_1_to_2 -i "${MOCK_LINES_FILE}" -s 1 2 -D)
SET_TESTS_PROPERTIES(textblock_select_delete_1_to_2 PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 3\nLine 4\nLine 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2"
   LABELS "text-block;select"
)

# Test deleting positive line range 2 to 4
ADD_TEXT_BLOCK_TEST(textblock_select_delete_2_to_4 -i "${MOCK_LINES_FILE}" -s 2 4 -D)
SET_TESTS_PROPERTIES(textblock_select_delete_2_to_4 PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nLine 5"
   FAIL_REGULAR_EXPRESSION "Line 2|Line 3|Line 4"
   LABELS "text-block;select"
)

# Test deleting positive line range 4 to 5
ADD_TEXT_BLOCK_TEST(textblock_select_delete_4_to_5 -i "${MOCK_LINES_FILE}" -s 4 5 -D)
SET_TESTS_PROPERTIES(textblock_select_delete_4_to_5 PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nLine 2\nLine 3"
   FAIL_REGULAR_EXPRESSION "Line 4|Line 5"
   LABELS "text-block;select"
)

# Test open-ended extraction from line 2 to EOF
ADD_TEXT_BLOCK_TEST(textblock_select_to_eof_from_2 -i "${MOCK_LINES_FILE}" -s 2 0 -X)
SET_TESTS_PROPERTIES(textblock_select_to_eof_from_2 PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 2.*Line 3.*Line 4.*Line 5"
   FAIL_REGULAR_EXPRESSION "Line 1"
   LABELS "text-block;select"
)

# Test open-ended extraction from line 4 to EOF
ADD_TEXT_BLOCK_TEST(textblock_select_to_eof_from_4 -i "${MOCK_LINES_FILE}" -s 4 0 -X)
SET_TESTS_PROPERTIES(textblock_select_to_eof_from_4 PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 4\nLine 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2|Line 3"
   LABELS "text-block;select"
)

# Test negative line index for the last line only (-s -1 -1)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_1_only -i "${MOCK_LINES_FILE}" -s -1 -1 -X)
SET_TESTS_PROPERTIES(textblock_select_neg_1_only PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2|Line 3|Line 4"
   LABELS "text-block;select"
)

# Test negative line index for the second to last line only (-s -2 -2)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_2_only -i "${MOCK_LINES_FILE}" -s -2 -2 -X)
SET_TESTS_PROPERTIES(textblock_select_neg_2_only PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 4"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2|Line 3|Line 5"
   LABELS "text-block;select"
)

# Test negative line index for the last two lines (-s -2 -1)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_last_two -i "${MOCK_LINES_FILE}" -s -2 -1 -X)
SET_TESTS_PROPERTIES(textblock_select_neg_last_two PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 4\nLine 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2|Line 3"
   LABELS "text-block;select"
)

# Test negative line index for the last three lines (-s -3 -1)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_last_three -i "${MOCK_LINES_FILE}" -s -3 -1 -X)
SET_TESTS_PROPERTIES(textblock_select_neg_last_three PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 3.*Line 4.*Line 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2"
   LABELS "text-block;select"
)

# Test negative line index for all lines (-s -5 -1)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_all -i "${MOCK_LINES_FILE}" -s -5 -1 -X)
SET_TESTS_PROPERTIES(textblock_select_neg_all PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 2.*Line 3.*Line 4.*Line 5"
   LABELS "text-block;select"
)

# Test negative line index deleting the last two lines (-s -2 -1 -D)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_delete_last_two -i "${MOCK_LINES_FILE}" -s -2 -1 -D)
SET_TESTS_PROPERTIES(textblock_select_neg_delete_last_two PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 2.*Line 3"
   FAIL_REGULAR_EXPRESSION "Line 4|Line 5"
   LABELS "text-block;select"
)

# Test negative line index deleting middle lines (-s -4 -2 -D)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_delete_middle -i "${MOCK_LINES_FILE}" -s -4 -2 -D)
SET_TESTS_PROPERTIES(textblock_select_neg_delete_middle PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nLine 5"
   FAIL_REGULAR_EXPRESSION "Line 2|Line 3|Line 4"
   LABELS "text-block;select"
)

# Test negative line index to EOF from 3 lines back (-s -3 0)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_to_eof_3 -i "${MOCK_LINES_FILE}" -s -3 0 -X)
SET_TESTS_PROPERTIES(textblock_select_neg_to_eof_3 PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 3.*Line 4.*Line 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2"
   LABELS "text-block;select"
)

# Test negative line index to EOF for final line (-s -1 0)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_to_eof_1 -i "${MOCK_LINES_FILE}" -s -1 0 -X)
SET_TESTS_PROPERTIES(textblock_select_neg_to_eof_1 PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2|Line 3|Line 4"
   LABELS "text-block;select"
)

# Test mixed positive start and negative end extracting middle (-s 2 -2)
ADD_TEXT_BLOCK_TEST(textblock_select_mixed_2_neg2 -i "${MOCK_LINES_FILE}" -s 2 -2 -X)
SET_TESTS_PROPERTIES(textblock_select_mixed_2_neg2 PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 2.*Line 3.*Line 4"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 5"
   LABELS "text-block;select"
)

# Test mixed positive start and negative end deleting middle (-s 2 -2 -D)
ADD_TEXT_BLOCK_TEST(textblock_select_mixed_delete_2_neg2 -i "${MOCK_LINES_FILE}" -s 2 -2 -D)
SET_TESTS_PROPERTIES(textblock_select_mixed_delete_2_neg2 PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nLine 5"
   FAIL_REGULAR_EXPRESSION "Line 2|Line 3|Line 4"
   LABELS "text-block;select"
)

# Test extracting first half of 10-line file (rows 1 to 5)
ADD_TEXT_BLOCK_TEST(textblock_select_long_range_1_to_5 -i "${MOCK_LONG_FILE}" -s 1 5 -X)
SET_TESTS_PROPERTIES(textblock_select_long_range_1_to_5 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 01.*Row 02.*Row 03.*Row 04.*Row 05"
   FAIL_REGULAR_EXPRESSION "Row 06|Row 07|Row 08|Row 09|Row 10"
   LABELS "text-block;select"
)

# Test extracting second half of 10-line file (rows 6 to 10)
ADD_TEXT_BLOCK_TEST(textblock_select_long_range_6_to_10 -i "${MOCK_LONG_FILE}" -s 6 10 -X)
SET_TESTS_PROPERTIES(textblock_select_long_range_6_to_10 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 06.*Row 07.*Row 08.*Row 09.*Row 10"
   FAIL_REGULAR_EXPRESSION "Row 01|Row 02|Row 03|Row 04|Row 05"
   LABELS "text-block;select"
)

# Test extracting middle section of 10-line file (rows 4 to 7)
ADD_TEXT_BLOCK_TEST(textblock_select_long_range_4_to_7 -i "${MOCK_LONG_FILE}" -s 4 7 -X)
SET_TESTS_PROPERTIES(textblock_select_long_range_4_to_7 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 04.*Row 05.*Row 06.*Row 07"
   FAIL_REGULAR_EXPRESSION "Row 01|Row 02|Row 03|Row 08|Row 09|Row 10"
   LABELS "text-block;select"
)

# Test negative line range on 10-line file (-s -4 -2)
ADD_TEXT_BLOCK_TEST(textblock_select_long_neg_range_neg4_to_neg2 -i "${MOCK_LONG_FILE}" -s -4 -2 -X)
SET_TESTS_PROPERTIES(textblock_select_long_neg_range_neg4_to_neg2 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 07.*Row 08.*Row 09"
   FAIL_REGULAR_EXPRESSION "Row 01|Row 06|Row 10"
   LABELS "text-block;select"
)

# Test negative line start to EOF on 10-line file (-s -5 0)
ADD_TEXT_BLOCK_TEST(textblock_select_long_neg_to_eof -i "${MOCK_LONG_FILE}" -s -5 0 -X)
SET_TESTS_PROPERTIES(textblock_select_long_neg_to_eof PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 06.*Row 07.*Row 08.*Row 09.*Row 10"
   FAIL_REGULAR_EXPRESSION "Row 01|Row 02|Row 03|Row 04|Row 05"
   LABELS "text-block;select"
)

# Test deleting from negative line offset to EOF on 10-line file (-s -4 0 -D)
ADD_TEXT_BLOCK_TEST(textblock_select_long_delete_neg_to_eof -i "${MOCK_LONG_FILE}" -s -4 0 -D)
SET_TESTS_PROPERTIES(textblock_select_long_delete_neg_to_eof PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 01.*Row 02.*Row 03.*Row 04.*Row 05.*Row 06"
   FAIL_REGULAR_EXPRESSION "Row 07|Row 08|Row 09|Row 10"
   LABELS "text-block;select"
)


#############################################################################
#### 9. Line Range Selection: Insert & Replace Operations                ####
#############################################################################

# Test Insert Front driven by positive line range selection
ADD_TEXT_BLOCK_TEST(textblock_select_insert_front_pos
   -i "${MOCK_LINES_FILE}" -s 2 3 -F "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_front_pos PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nINSERTED_PAYLOAD\nLine 2\nLine 3\nLine 4"
   LABELS "text-block;select;insert"
)

# Test Insert Front at the beginning of file driven by range (-s 1 2)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_front_top
   -i "${MOCK_LINES_FILE}" -s 1 2 -F "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_front_top PROPERTIES
   PASS_REGULAR_EXPRESSION "INSERTED_PAYLOAD\nLine 1\nLine 2\nLine 3"
   LABELS "text-block;select;insert"
)

# Test Insert Front driven by negative line range selection (-s -2 -1)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_front_neg
   -i "${MOCK_LINES_FILE}" -s -2 -1 -F "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_front_neg PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 3\nINSERTED_PAYLOAD\nLine 4\nLine 5"
   LABELS "text-block;select;insert"
)

# Test Insert Back driven by positive line range selection
ADD_TEXT_BLOCK_TEST(textblock_select_insert_back_pos
   -i "${MOCK_LINES_FILE}" -s 2 3 -B "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_back_pos PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nLine 2\nLine 3\nINSERTED_PAYLOAD\nLine 4"
   LABELS "text-block;select;insert"
)

# Test Insert Back driven by line range extending to EOF (-s 4 0)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_back_eof
   -i "${MOCK_LINES_FILE}" -s 4 0 -B "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_back_eof PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 4\nLine 5\nINSERTED_PAYLOAD"
   LABELS "text-block;select;insert"
)

# Test Insert Back driven by negative line range selection (-s -2 -1)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_back_neg
   -i "${MOCK_LINES_FILE}" -s -2 -1 -B "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_back_neg PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 4\nLine 5\nINSERTED_PAYLOAD"
   LABELS "text-block;select;insert"
)

# Test Replace driven by positive line range selection
ADD_TEXT_BLOCK_TEST(textblock_select_replace_pos
   -i "${MOCK_LINES_FILE}" -s 2 4 -R "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_replace_pos PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nINSERTED_PAYLOAD\nLine 5"
   FAIL_REGULAR_EXPRESSION "Line 2|Line 3|Line 4"
   LABELS "text-block;select;replace"
)

# Test Replace on the first line only (-s 1 1)
ADD_TEXT_BLOCK_TEST(textblock_select_replace_top
   -i "${MOCK_LINES_FILE}" -s 1 1 -R "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_replace_top PROPERTIES
   PASS_REGULAR_EXPRESSION "INSERTED_PAYLOAD\nLine 2\nLine 3"
   FAIL_REGULAR_EXPRESSION "Line 1"
   LABELS "text-block;select;replace"
)

# Test Replace on the last line only (-s 5 5)
ADD_TEXT_BLOCK_TEST(textblock_select_replace_bottom
   -i "${MOCK_LINES_FILE}" -s 5 5 -R "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_replace_bottom PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 4\nINSERTED_PAYLOAD"
   FAIL_REGULAR_EXPRESSION "Line 5"
   LABELS "text-block;select;replace"
)

# Test Replace driven by negative line range selection (-s -3 -1)
ADD_TEXT_BLOCK_TEST(textblock_select_replace_neg
   -i "${MOCK_LINES_FILE}" -s -3 -1 -R "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_replace_neg PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nLine 2\nINSERTED_PAYLOAD"
   FAIL_REGULAR_EXPRESSION "Line 3|Line 4|Line 5"
   LABELS "text-block;select;replace"
)


#############################################################################
#### 10. Action Limits (-m / --min-actions, -M / --max-actions)          ####
#############################################################################

# Test exact single action constraint matching 1 block
ADD_TEXT_BLOCK_TEST(textblock_actions_exact_1
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -m 1 -M 1
)

# Test minimum action count satisfied (requires >= 1, file has 1)
ADD_TEXT_BLOCK_TEST(textblock_actions_min_1_pass
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -m 1
)

# Test minimum action failure (requires >= 2, file has 1)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_min_2_fail
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" --min-actions 2
)

# Test maximum action count satisfied (allows <= 1, file has 1)
ADD_TEXT_BLOCK_TEST(textblock_actions_max_1_pass
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -M 1
)

# Test maximum action failure (allows <= 0, file has 1)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_max_0_fail
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" --max-actions 0
)

# Test exact matching on multi-block file (requires exactly 2, file has 2)
ADD_TEXT_BLOCK_TEST(textblock_actions_multi_exact_2
   -i "${MOCK_MULTI_TAG_FILE}" -X -b "<BEGIN>" -e "<END>" -m 2 -M 2
)

# Test action range constraint satisfied on multi-block file
ADD_TEXT_BLOCK_TEST(textblock_actions_multi_range_1_to_3
   -i "${MOCK_MULTI_TAG_FILE}" -X -b "<BEGIN>" -e "<END>" -m 1 -M 3
)

# Test under minimum actions failure on multi-block file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_multi_under_min
   -i "${MOCK_MULTI_TAG_FILE}" -X -b "<BEGIN>" -e "<END>" -m 3
)

# Test over maximum actions failure on multi-block file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_multi_over_max
   -i "${MOCK_MULTI_TAG_FILE}" -X -b "<BEGIN>" -e "<END>" -M 1
)

# Test exact matching on 3-block file (requires exactly 3, file has 3)
ADD_TEXT_BLOCK_TEST(textblock_actions_three_blocks_exact
   -i "${MOCK_THREE_BLOCKS_FILE}" -X -b "<B>" -e "</B>" -m 3 -M 3
)

# Test action range constraint satisfied on 3-block file
ADD_TEXT_BLOCK_TEST(textblock_actions_three_blocks_range_2_to_4
   -i "${MOCK_THREE_BLOCKS_FILE}" -X -b "<B>" -e "</B>" -m 2 -M 4
)

# Test under minimum actions failure on 3-block file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_three_blocks_under_min
   -i "${MOCK_THREE_BLOCKS_FILE}" -X -b "<B>" -e "</B>" -m 4
)

# Test over maximum actions failure on 3-block file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_three_blocks_over_max
   -i "${MOCK_THREE_BLOCKS_FILE}" -X -b "<B>" -e "</B>" -M 2
)

# Test exact matching on inline multi-block line
ADD_TEXT_BLOCK_TEST(textblock_actions_inline_exact_2
   -i "${MOCK_MULTI_BLOCK_FILE}" -X -b "[B]" -e "[/B]" -m 2 -M 2
)

# Test under minimum actions failure on inline multi-block line
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_inline_under_min
   -i "${MOCK_MULTI_BLOCK_FILE}" -X -b "[B]" -e "[/B]" -m 3
)

# Test over maximum actions failure on inline multi-block line
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_inline_over_max
   -i "${MOCK_MULTI_BLOCK_FILE}" -X -b "[B]" -e "[/B]" -M 1
)

# Test zero matches passing when max-actions is 0
ADD_TEXT_BLOCK_TEST(textblock_actions_zero_matches_pass_max_0
   -i "${MOCK_LINES_FILE}" -X -b "<ABSENT>" -e "</ABSENT>" -M 0
)

# Test zero matches failing when min-actions is 1
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_zero_matches_fail_min_1
   -i "${MOCK_LINES_FILE}" -X -b "<ABSENT>" -e "</ABSENT>" -m 1
)

# Test min-actions 0 always succeeds when zero matches occur
ADD_TEXT_BLOCK_TEST(textblock_actions_min_0_always_pass
   -i "${MOCK_LINES_FILE}" -X -b "<ABSENT>" -e "</ABSENT>" -m 0
)

# Test line range selection action counting (counts as exactly 1 action)
ADD_TEXT_BLOCK_TEST(textblock_actions_select_exact_1
   -i "${MOCK_LINES_FILE}" -s 2 4 -X -m 1 -M 1
)

# Test line range selection failing when min-actions requires 2
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_select_under_min
   -i "${MOCK_LINES_FILE}" -s 2 4 -X -m 2
)


#############################################################################
#### 11. Warnings and Warning Suppression (-q / --suppress-warnings)     ####
#############################################################################

# Test non-fatal warning when tags are provided to Cat mode
ADD_TEXT_BLOCK_TEST(textblock_warning_tags_cat
   -i "${MOCK_LINES_FILE}" -C -b "<TAG>"
)
SET_TESTS_PROPERTIES(textblock_warning_tags_cat PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Begin/end tags.*have no effect for Cat"
   LABELS "text-block;warning"
)

# Test non-fatal warning when tags are provided to Discard mode
ADD_TEXT_BLOCK_TEST(textblock_warning_tags_discard
   -i "${MOCK_LINES_FILE}" -0 -b "<TAG>"
)
SET_TESTS_PROPERTIES(textblock_warning_tags_discard PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Begin/end tags.*have no effect for Cat, Discard, or Enumerate"
   LABELS "text-block;warning"
)

# Test non-fatal warning when tags are provided to Enumerate mode
ADD_TEXT_BLOCK_TEST(textblock_warning_tags_enumerate
   -i "${MOCK_LINES_FILE}" -E -b "<TAG>"
)
SET_TESTS_PROPERTIES(textblock_warning_tags_enumerate PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Begin/end tags.*have no effect for Cat, Discard, or Enumerate"
   LABELS "text-block;warning"
)

# Test non-fatal warning when identical tags are excluded in Extract mode
ADD_TEXT_BLOCK_TEST(textblock_warning_identical_tags_extract
   -i "${MOCK_LINES_FILE}" -X -t "<TAG>" -x
)
SET_TESTS_PROPERTIES(textblock_warning_identical_tags_extract PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Identical begin/end tags.*not useful for Extract, Remove, or Replace"
   LABELS "text-block;warning"
)

# Test non-fatal warning when identical tags are excluded in Delete mode
ADD_TEXT_BLOCK_TEST(textblock_warning_identical_tags_delete
   -i "${MOCK_LINES_FILE}" -D -t "<TAG>" -x
)
SET_TESTS_PROPERTIES(textblock_warning_identical_tags_delete PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Identical begin/end tags.*not useful for Extract, Remove, or Replace"
   LABELS "text-block;warning"
)

# Test non-fatal warning when identical tags are excluded in Replace mode
ADD_TEXT_BLOCK_TEST(textblock_warning_identical_tags_replace
   -i "${MOCK_LINES_FILE}" -R "${MOCK_INSERT_FILE}" -t "<TAG>" -x
)
SET_TESTS_PROPERTIES(textblock_warning_identical_tags_replace PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Identical begin/end tags.*not useful for Extract, Remove, or Replace"
   LABELS "text-block;warning"
)

# Test non-fatal warning when line selection range is inverted (start > end)
ADD_TEXT_BLOCK_TEST(textblock_warning_inverted_selection
   -i "${MOCK_LINES_FILE}" -s 5 2 -X
)
SET_TESTS_PROPERTIES(textblock_warning_inverted_selection PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Line selection range.*not useful"
   LABELS "text-block;warning"
)

# Test warning suppression in Cat mode via short flag -q
ADD_TEXT_BLOCK_TEST(textblock_suppress_warning_cat
   -i "${MOCK_LINES_FILE}" -C -b "<TAG>" -q
)
SET_TESTS_PROPERTIES(textblock_suppress_warning_cat PROPERTIES
   FAIL_REGULAR_EXPRESSION "WARNING:"
   LABELS "text-block;warning"
)

# Test warning suppression in Discard mode via long flag --suppress-warnings
ADD_TEXT_BLOCK_TEST(textblock_suppress_warning_discard
   -i "${MOCK_LINES_FILE}" -0 -b "<TAG>" --suppress-warnings
)
SET_TESTS_PROPERTIES(textblock_suppress_warning_discard PROPERTIES
   FAIL_REGULAR_EXPRESSION "WARNING:"
   LABELS "text-block;warning"
)

# Test warning suppression on inverted line range via long flag --suppress-warnings
ADD_TEXT_BLOCK_TEST(textblock_suppress_warning_inverted_select
   -i "${MOCK_LINES_FILE}" -s 5 2 -X --suppress-warnings
)
SET_TESTS_PROPERTIES(textblock_suppress_warning_inverted_select PROPERTIES
   FAIL_REGULAR_EXPRESSION "WARNING:"
   LABELS "text-block;warning"
)


#############################################################################
#### 12. File I/O Operations (-o, -o -, -a, -p, -k)                      ####
#############################################################################

SET(TEST_OUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/out_redirect.txt")

# Test output redirection using short option -o
ADD_TEXT_BLOCK_TEST(textblock_output_file_short
   -i "${MOCK_LINES_FILE}" -o "${TEST_OUT_FILE}" -C
)

# Test output redirection using long option --output
ADD_TEXT_BLOCK_TEST(textblock_output_file_long
   -i "${MOCK_LINES_FILE}" --output "${TEST_OUT_FILE}" -C
)

# Test output redirection combined with Extract mode
ADD_TEXT_BLOCK_TEST(textblock_output_with_extract
   -i "${MOCK_TAGGED_FILE}" -o "${TEST_OUT_FILE}" -X -b "<BEGIN>" -e "<END>"
)

# Test output redirection combined with Delete mode
ADD_TEXT_BLOCK_TEST(textblock_output_with_delete
   -i "${MOCK_TAGGED_FILE}" -o "${TEST_OUT_FILE}" -D -b "<BEGIN>" -e "<END>" -f -y
)

# Test explicit stdout routing placeholder (-o -) in Cat mode
ADD_TEXT_BLOCK_TEST(textblock_output_stdout_placeholder_cat
   -i "${MOCK_LINES_FILE}" -o "-" -C
)
SET_TESTS_PROPERTIES(textblock_output_stdout_placeholder_cat PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 5"
   LABELS "text-block;io"
)

# Test explicit stdout routing placeholder (-o -) in Extract mode
ADD_TEXT_BLOCK_TEST(textblock_output_stdout_placeholder_extract
   -i "${MOCK_TAGGED_FILE}" -o "-" -X -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_output_stdout_placeholder_extract PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 1.*Block line 2"
   FAIL_REGULAR_EXPRESSION "Header line|Footer line"
   LABELS "text-block;io"
)

# Test file appending using short option -a
ADD_TEXT_BLOCK_TEST(textblock_append_file_short
   -i "${MOCK_LINES_FILE}" -o "${TEST_OUT_FILE}" -a -C
)

# Test file appending using long option --append
ADD_TEXT_BLOCK_TEST(textblock_append_file_long
   -i "${MOCK_LINES_FILE}" -o "${TEST_OUT_FILE}" --append -C
)

# Test in-place file modification using short option -p
ADD_TEXT_BLOCK_TEST(textblock_inplace_short
   -i "${TEST_INPLACE_FILE}" -p -s 2 2 -D
)

# Test in-place file modification using long option --in-place
ADD_TEXT_BLOCK_TEST(textblock_inplace_long
   -i "${TEST_INPLACE_FILE}" --in-place -s 1 1 -D
)

# Test keeping broken output file on error via -k flag
SET(TEST_BROKEN_KEPT "${CMAKE_CURRENT_BINARY_DIR}/broken_kept.txt")
ADD_TEXT_BLOCK_FAIL_TEST(textblock_keep_broken_output
   -i "${MOCK_TAGGED_FILE}" -o "${TEST_BROKEN_KEPT}" -X -b "<BEGIN>" -e "<END>" -m 2 -k
)


#############################################################################
#### 13. Unclosed Tag Blocks at EOF (Cleanup Flush Logic)                ####
#############################################################################

# Test Highlight mode automatically flushing end marker symbol at EOF
ADD_TEXT_BLOCK_TEST(textblock_unclosed_tag_highlight
   -i "${MOCK_UNCLOSED_TAG_FILE}" -H -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_unclosed_tag_highlight PROPERTIES
   PASS_REGULAR_EXPRESSION "⭐.*Unclosed line 1.*Unclosed line 2.*🛑"
   LABELS "text-block;highlight;eof"
)

# Test Extract mode capturing open block through EOF
ADD_TEXT_BLOCK_TEST(textblock_unclosed_tag_extract
   -i "${MOCK_UNCLOSED_TAG_FILE}" -X -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_unclosed_tag_extract PROPERTIES
   PASS_REGULAR_EXPRESSION "Unclosed line 1.*Unclosed line 2"
   FAIL_REGULAR_EXPRESSION "Header line|<BEGIN>"
   LABELS "text-block;extract;eof"
)

# Test Insert Back mode flushing inserted file at EOF when tag is unclosed
ADD_TEXT_BLOCK_TEST(textblock_unclosed_tag_insert_back
   -i "${MOCK_UNCLOSED_TAG_FILE}" -B "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_unclosed_tag_insert_back PROPERTIES
   PASS_REGULAR_EXPRESSION "Unclosed line 2\nINSERTED_PAYLOAD"
   LABELS "text-block;insert;eof"
)

# Test Replace mode replacing block content through EOF when tag is unclosed
ADD_TEXT_BLOCK_TEST(textblock_unclosed_tag_replace
   -i "${MOCK_UNCLOSED_TAG_FILE}" -R "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -y
)
SET_TESTS_PROPERTIES(textblock_unclosed_tag_replace PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line\nINSERTED_PAYLOAD"
   FAIL_REGULAR_EXPRESSION "Unclosed line"
   LABELS "text-block;replace;eof"
)

# Test Delete mode removing unclosed block through EOF
ADD_TEXT_BLOCK_TEST(textblock_unclosed_tag_delete
   -i "${MOCK_UNCLOSED_TAG_FILE}" -D -b "<BEGIN>" -e "<END>" -y
)
SET_TESTS_PROPERTIES(textblock_unclosed_tag_delete PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line"
   FAIL_REGULAR_EXPRESSION "Unclosed line|<BEGIN>"
   LABELS "text-block;delete;eof"
)


#############################################################################
#### 14. Long Line Buffering (> 4096-Byte Internal Buffer)               ####
#############################################################################

# Test reading and reassembling an 8192-byte line in Cat mode
ADD_TEXT_BLOCK_TEST(textblock_longline_cat
   -i "${MOCK_LONGLINE_FILE}" -C
)
SET_TESTS_PROPERTIES(textblock_longline_cat PROPERTIES
   PASS_REGULAR_EXPRESSION "Start line.*0123456789ABCDEF.*Finish line"
   LABELS "text-block;buffer"
)

# Test extracting an 8192-byte line without buffer truncation
ADD_TEXT_BLOCK_TEST(textblock_longline_extract
   -i "${MOCK_LONGLINE_FILE}" -X -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_longline_extract PROPERTIES
   PASS_REGULAR_EXPRESSION "0123456789ABCDEF"
   FAIL_REGULAR_EXPRESSION "Start line|Finish line|<BEGIN>|<END>"
   LABELS "text-block;buffer;extract"
)

# Test deleting full tag lines containing an 8192-byte line
ADD_TEXT_BLOCK_TEST(textblock_longline_delete
   -i "${MOCK_LONGLINE_FILE}" -D -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_longline_delete PROPERTIES
   PASS_REGULAR_EXPRESSION "Start line\nFinish line"
   FAIL_REGULAR_EXPRESSION "0123456789ABCDEF|<BEGIN>|<END>"
   LABELS "text-block;buffer;delete"
)

# Test enumerating an 8192-byte line cleanly
ADD_TEXT_BLOCK_TEST(textblock_longline_enumerate
   -i "${MOCK_LONGLINE_FILE}" -E --enumerate-format 2 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_longline_enumerate PROPERTIES
   PASS_REGULAR_EXPRESSION "# 1: Start line.*# 3: 0123456789ABCDEF.*# 5: Finish line"
   LABELS "text-block;buffer;enumerate"
)


#############################################################################
#### 15. Standard Input Operations (Pipelines & Stream Runners)          ####
#############################################################################

# Test Insert Front receiving insert payload streamed from stdin (-F -)
ADD_TEST(
   NAME textblock_stdin_insert_front
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:text-block>"
      -D "INPUT_FILE=${MOCK_TAGGED_FILE}"
      -D "INSERT_PAYLOAD_FILE=${MOCK_INSERT_FILE}"
      -D "MODE_OPT=-F"
      -D "EXTRA_ARGS=-b;<BEGIN>;-e;<END>"
      -D "EXPECTED_MATCH=<BEGIN>INSERTED_PAYLOAD.*Block line 1"
      -P "${RUN_STDIN_INSERT_SCRIPT}"
)
SET_TESTS_PROPERTIES(textblock_stdin_insert_front PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   LABELS "text-block;insert;stdin"
)

# Test Insert Back receiving insert payload streamed from stdin (-B -)
ADD_TEST(
   NAME textblock_stdin_insert_back
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:text-block>"
      -D "INPUT_FILE=${MOCK_TAGGED_FILE}"
      -D "INSERT_PAYLOAD_FILE=${MOCK_INSERT_FILE}"
      -D "MODE_OPT=-B"
      -D "EXTRA_ARGS=-b;<BEGIN>;-e;<END>"
      -D "EXPECTED_MATCH=Block line 2.*INSERTED_PAYLOAD.*<END>"
      -P "${RUN_STDIN_INSERT_SCRIPT}"
)
SET_TESTS_PROPERTIES(textblock_stdin_insert_back PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   LABELS "text-block;insert;stdin"
)

# Test Replace mode receiving replacement payload from stdin (-R -)
ADD_TEST(
   NAME textblock_stdin_replace
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:text-block>"
      -D "INPUT_FILE=${MOCK_TAGGED_FILE}"
      -D "INSERT_PAYLOAD_FILE=${MOCK_INSERT_FILE}"
      -D "MODE_OPT=-R"
      -D "EXTRA_ARGS=-b;<BEGIN>;-e;<END>;-f;-y"
      -D "EXPECTED_MATCH=Header line\nINSERTED_PAYLOAD\nFooter line"
      -P "${RUN_STDIN_INSERT_SCRIPT}"
)
SET_TESTS_PROPERTIES(textblock_stdin_replace PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   LABELS "text-block;replace;stdin"
)

# Test Cat mode reading primary stream from stdin using explicit placeholder (-i -)
ADD_TEST(
   NAME textblock_stdin_primary_cat_dash
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:text-block>"
      -D "CMD_ARGS=-i;-;-C"
      -D "STDIN_SOURCE_FILE=${MOCK_LINES_FILE}"
      -D "EXPECTED_CODE=0"
      -D "EXPECTED_MATCH=Line 1.*Line 2.*Line 3.*Line 4.*Line 5"
      -P "${RUN_STDIN_INPUT_SCRIPT}"
)
SET_TESTS_PROPERTIES(textblock_stdin_primary_cat_dash PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   LABELS "text-block;cat;stdin"
)

# Test Cat mode reading primary stream from stdin by default (omitting -i)
ADD_TEST(
   NAME textblock_stdin_primary_cat_default
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:text-block>"
      -D "CMD_ARGS=-C"
      -D "STDIN_SOURCE_FILE=${MOCK_LINES_FILE}"
      -D "EXPECTED_CODE=0"
      -D "EXPECTED_MATCH=Line 1.*Line 2.*Line 3.*Line 4.*Line 5"
      -P "${RUN_STDIN_INPUT_SCRIPT}"
)
SET_TESTS_PROPERTIES(textblock_stdin_primary_cat_default PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   LABELS "text-block;cat;stdin"
)

# Test Enumerate mode processing text streamed through stdin
ADD_TEST(
   NAME textblock_stdin_primary_enumerate
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:text-block>"
      -D "CMD_ARGS=-i;-;-E"
      -D "STDIN_SOURCE_FILE=${MOCK_LINES_FILE}"
      -D "EXPECTED_CODE=0"
      -D "EXPECTED_MATCH=000001.*Line 1.*000005.*Line 5"
      -P "${RUN_STDIN_INPUT_SCRIPT}"
)
SET_TESTS_PROPERTIES(textblock_stdin_primary_enumerate PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   LABELS "text-block;enumerate;stdin"
)

# Test Extract mode processing text streamed through stdin
ADD_TEST(
   NAME textblock_stdin_primary_extract
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:text-block>"
      -D "CMD_ARGS=-i;-;-X;-b;<BEGIN>;-e;<END>"
      -D "STDIN_SOURCE_FILE=${MOCK_TAGGED_FILE}"
      -D "EXPECTED_CODE=0"
      -D "EXPECTED_MATCH=Block line 1.*Block line 2"
      -P "${RUN_STDIN_INPUT_SCRIPT}"
)
SET_TESTS_PROPERTIES(textblock_stdin_primary_extract PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   LABELS "text-block;extract;stdin"
)

# Test Delete mode processing text streamed through stdin
ADD_TEST(
   NAME textblock_stdin_primary_delete
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:text-block>"
      -D "CMD_ARGS=-i;-;-D;-b;<BEGIN>;-e;<END>;-f;-y"
      -D "STDIN_SOURCE_FILE=${MOCK_TAGGED_FILE}"
      -D "EXPECTED_CODE=0"
      -D "EXPECTED_MATCH=Header line\nFooter line"
      -P "${RUN_STDIN_INPUT_SCRIPT}"
)
SET_TESTS_PROPERTIES(textblock_stdin_primary_delete PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   LABELS "text-block;delete;stdin"
)

# Test Highlight mode processing text streamed through stdin
ADD_TEST(
   NAME textblock_stdin_primary_highlight
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:text-block>"
      -D "CMD_ARGS=-i;-;-H;-b;<BEGIN>;-e;<END>"
      -D "STDIN_SOURCE_FILE=${MOCK_TAGGED_FILE}"
      -D "EXPECTED_CODE=0"
      -D "EXPECTED_MATCH=⭐.*Block line 1.*🛑"
      -P "${RUN_STDIN_INPUT_SCRIPT}"
)
SET_TESTS_PROPERTIES(textblock_stdin_primary_highlight PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   LABELS "text-block;highlight;stdin"
)

# Test positive line range selection working over stdin streaming
ADD_TEST(
   NAME textblock_stdin_primary_select_positive
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:text-block>"
      -D "CMD_ARGS=-i;-;-s;2;4;-X"
      -D "STDIN_SOURCE_FILE=${MOCK_LINES_FILE}"
      -D "EXPECTED_CODE=0"
      -D "EXPECTED_MATCH=Line 2.*Line 3.*Line 4"
      -P "${RUN_STDIN_INPUT_SCRIPT}"
)
SET_TESTS_PROPERTIES(textblock_stdin_primary_select_positive PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   LABELS "text-block;select;stdin"
)

# Test negative line selection failing over stdin because line counting requires a seekable file
ADD_TEST(
   NAME textblock_stdin_primary_select_negative_fails
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:text-block>"
      -D "CMD_ARGS=-i;-;-s;-2;-1;-X"
      -D "STDIN_SOURCE_FILE=${MOCK_LINES_FILE}"
      -D "EXPECTED_CODE=1"
      -P "${RUN_STDIN_INPUT_SCRIPT}"
)
SET_TESTS_PROPERTIES(textblock_stdin_primary_select_negative_fails PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   LABELS "text-block;select;stdin;negative"
)


#############################################################################
#### 16. C/C++ Preprocessor & XML/Comment Style Tags                     ####
#############################################################################

# Test extracting C preprocessor conditional block
ADD_TEXT_BLOCK_TEST(textblock_code_extract
   -i "${MOCK_CODE_FILE}" -X -b "#ifdef DEBUG" -e "#endif"
)
SET_TESTS_PROPERTIES(textblock_code_extract PROPERTIES
   PASS_REGULAR_EXPRESSION "debug_log\\(\\);"
   FAIL_REGULAR_EXPRESSION "Header|Footer|#ifdef|#endif"
   LABELS "text-block;extract;code"
)

# Test deleting C preprocessor block including full tag lines
ADD_TEXT_BLOCK_TEST(textblock_code_delete
   -i "${MOCK_CODE_FILE}" -D -b "#ifdef DEBUG" -e "#endif" -f -y
)
SET_TESTS_PROPERTIES(textblock_code_delete PROPERTIES
   PASS_REGULAR_EXPRESSION "// Header\n// Footer"
   FAIL_REGULAR_EXPRESSION "debug_log|#ifdef|#endif"
   LABELS "text-block;delete;code"
)

# Test replacing C preprocessor block with payload
ADD_TEXT_BLOCK_TEST(textblock_code_replace
   -i "${MOCK_CODE_FILE}" -R "${MOCK_INSERT_FILE}" -b "#ifdef DEBUG" -e "#endif" -f -y
)
SET_TESTS_PROPERTIES(textblock_code_replace PROPERTIES
   PASS_REGULAR_EXPRESSION "// Header\nINSERTED_PAYLOAD\n// Footer"
   FAIL_REGULAR_EXPRESSION "debug_log|#ifdef|#endif"
   LABELS "text-block;replace;code"
)

# Test inserting front into C preprocessor block with full tag lines
ADD_TEXT_BLOCK_TEST(textblock_code_insert_front
   -i "${MOCK_CODE_FILE}" -F "${MOCK_INSERT_FILE}" -b "#ifdef DEBUG" -e "#endif" -f
)
SET_TESTS_PROPERTIES(textblock_code_insert_front PROPERTIES
   PASS_REGULAR_EXPRESSION "#ifdef DEBUG\nINSERTED_PAYLOAD\ndebug_log"
   LABELS "text-block;insert;code"
)

# Test extracting block delineated by XML comment tags
ADD_TEXT_BLOCK_TEST(textblock_xml_extract
   -i "${MOCK_XML_FILE}" -X -b "<!--BEGIN-->" -e "<!--END-->"
)
SET_TESTS_PROPERTIES(textblock_xml_extract PROPERTIES
   PASS_REGULAR_EXPRESSION "<item>Payload</item>"
   FAIL_REGULAR_EXPRESSION "<root>|</root>|<!--BEGIN-->|<!--END-->"
   LABELS "text-block;extract;xml"
)

# Test deleting XML comment tags and interior payload
ADD_TEXT_BLOCK_TEST(textblock_xml_delete
   -i "${MOCK_XML_FILE}" -D -b "<!--BEGIN-->" -e "<!--END-->" -f -y
)
SET_TESTS_PROPERTIES(textblock_xml_delete PROPERTIES
   PASS_REGULAR_EXPRESSION "<root>\n</root>"
   FAIL_REGULAR_EXPRESSION "Payload|<!--BEGIN-->|<!--END-->"
   LABELS "text-block;delete;xml"
)

# Test replacing XML comment block with file payload
ADD_TEXT_BLOCK_TEST(textblock_xml_replace
   -i "${MOCK_XML_FILE}" -R "${MOCK_INSERT_FILE}" -b "<!--BEGIN-->" -e "<!--END-->" -f -y
)
SET_TESTS_PROPERTIES(textblock_xml_replace PROPERTIES
   PASS_REGULAR_EXPRESSION "<root>\nINSERTED_PAYLOAD\n</root>"
   FAIL_REGULAR_EXPRESSION "Payload|<!--BEGIN-->|<!--END-->"
   LABELS "text-block;replace;xml"
)

# Test highlighting XML comment block
ADD_TEXT_BLOCK_TEST(textblock_xml_highlight
   -i "${MOCK_XML_FILE}" -H -b "<!--BEGIN-->" -e "<!--END-->"
)
SET_TESTS_PROPERTIES(textblock_xml_highlight PROPERTIES
   PASS_REGULAR_EXPRESSION "<!--BEGIN-->.*⭐.*Payload.*🛑.*<!--END-->"
   LABELS "text-block;highlight;xml"
)


#############################################################################
#### 17. Advanced Tag Combinations & Modifiers                           ####
#############################################################################

# Test Insert Front tags-only with included tags (payload inserted before <BEGIN>)
ADD_TEXT_BLOCK_TEST(textblock_insert_front_tags_only_include
   -i "${MOCK_TAGGED_FILE}" -F "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -g -y
)
SET_TESTS_PROPERTIES(textblock_insert_front_tags_only_include PROPERTIES
   PASS_REGULAR_EXPRESSION "INSERTED_PAYLOAD.*<BEGIN>.*Block line 1"
   LABELS "text-block;insert"
)

# Test Insert Front tags-only with excluded tags (payload inserted after <BEGIN>)
ADD_TEXT_BLOCK_TEST(textblock_insert_front_tags_only_exclude
   -i "${MOCK_TAGGED_FILE}" -F "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -g -x
)
SET_TESTS_PROPERTIES(textblock_insert_front_tags_only_exclude PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>INSERTED_PAYLOAD.*Block line 1"
   LABELS "text-block;insert"
)

# Test Insert Back tags-only with included tags (payload inserted after <END>)
ADD_TEXT_BLOCK_TEST(textblock_insert_back_tags_only_include
   -i "${MOCK_TAGGED_FILE}" -B "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -g -y
)
SET_TESTS_PROPERTIES(textblock_insert_back_tags_only_include PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 2.*<END>INSERTED_PAYLOAD"
   LABELS "text-block;insert"
)

# Test Insert Back tags-only with excluded tags (payload inserted before <END>)
ADD_TEXT_BLOCK_TEST(textblock_insert_back_tags_only_exclude
   -i "${MOCK_TAGGED_FILE}" -B "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -g -x
)
SET_TESTS_PROPERTIES(textblock_insert_back_tags_only_exclude PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 2.*INSERTED_PAYLOAD.*<END>"
   LABELS "text-block;insert"
)

# Test Replace tags-only with included tags
ADD_TEXT_BLOCK_TEST(textblock_replace_tags_only_include
   -i "${MOCK_TAGGED_FILE}" -R "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -g -y
)
SET_TESTS_PROPERTIES(textblock_replace_tags_only_include PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line.*INSERTED_PAYLOAD.*Footer line"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block line"
   LABELS "text-block;replace"
)

# Test Replace tags-only with excluded tags
ADD_TEXT_BLOCK_TEST(textblock_replace_tags_only_exclude
   -i "${MOCK_TAGGED_FILE}" -R "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -g -x
)
SET_TESTS_PROPERTIES(textblock_replace_tags_only_exclude PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>INSERTED_PAYLOAD.*<END>"
   FAIL_REGULAR_EXPRESSION "Block line"
   LABELS "text-block;replace"
)

# Test Delete mode on full tag lines while excluding tags
ADD_TEXT_BLOCK_TEST(textblock_delete_full_lines_exclude_tags
   -i "${MOCK_TAGGED_FILE}" -D -b "<BEGIN>" -e "<END>" -f -x
)
SET_TESTS_PROPERTIES(textblock_delete_full_lines_exclude_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line.*<BEGIN>\n<END>.*Footer line"
   FAIL_REGULAR_EXPRESSION "Block line"
   LABELS "text-block;delete"
)

# Test Highlight mode tags-only with included tags
ADD_TEXT_BLOCK_TEST(textblock_highlight_tags_only_include
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" -g -y
)
SET_TESTS_PROPERTIES(textblock_highlight_tags_only_include PROPERTIES
   PASS_REGULAR_EXPRESSION "⭐.*<BEGIN>.*Block line 1.*<END>.*🛑"
   LABELS "text-block;highlight"
)

# Test Highlight mode tags-only with excluded tags
ADD_TEXT_BLOCK_TEST(textblock_highlight_tags_only_exclude
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" -g -x
)
SET_TESTS_PROPERTIES(textblock_highlight_tags_only_exclude PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>.*⭐.*Block line 1.*🛑.*<END>"
   LABELS "text-block;highlight"
)

# Test Highlight mode full tag lines with included tags
ADD_TEXT_BLOCK_TEST(textblock_highlight_full_lines_include
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_highlight_full_lines_include PROPERTIES
   PASS_REGULAR_EXPRESSION "⭐.*<BEGIN>.*Block line 1.*<END>.*🛑"
   LABELS "text-block;highlight"
)

# Test Highlight mode full tag lines with excluded tags
ADD_TEXT_BLOCK_TEST(textblock_highlight_full_lines_exclude
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" -f -x
)
SET_TESTS_PROPERTIES(textblock_highlight_full_lines_exclude PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>.*⭐.*Block line 1.*🛑.*<END>"
   LABELS "text-block;highlight"
)

# Test single marker tag with Insert Front and full tag lines (-t -f)
ADD_TEXT_BLOCK_TEST(textblock_single_tag_insert_front_full_lines
   -i "${MOCK_SINGLE_TAG_FILE}" -F "${MOCK_INSERT_FILE}" -t "<MARKER>" -f
)
SET_TESTS_PROPERTIES(textblock_single_tag_insert_front_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "Line A\n<MARKER>\nINSERTED_PAYLOAD\nLine B"
   LABELS "text-block;insert"
)

# Test single marker tag with Insert Back and full tag lines (-t -f)
ADD_TEXT_BLOCK_TEST(textblock_single_tag_insert_back_full_lines
   -i "${MOCK_SINGLE_TAG_FILE}" -B "${MOCK_INSERT_FILE}" -t "<MARKER>" -f
)
SET_TESTS_PROPERTIES(textblock_single_tag_insert_back_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "Line A\n<MARKER>\nINSERTED_PAYLOAD\nLine B"
   LABELS "text-block;insert"
)

# Test single marker tag with Replace and full tag lines (-t -f -y)
ADD_TEXT_BLOCK_TEST(textblock_single_tag_replace_full_lines
   -i "${MOCK_SINGLE_TAG_FILE}" -R "${MOCK_INSERT_FILE}" -t "<MARKER>" -f -y
)
SET_TESTS_PROPERTIES(textblock_single_tag_replace_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "Line A\nINSERTED_PAYLOAD\nLine B"
   FAIL_REGULAR_EXPRESSION "<MARKER>"
   LABELS "text-block;replace"
)


#############################################################################
#### 18. Extended Line Range Operations on MOCK_LONG_FILE                ####
#############################################################################

# Test extracting row 1 only on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_long_select_row_1 -i "${MOCK_LONG_FILE}" -s 1 1 -X)
SET_TESTS_PROPERTIES(textblock_long_select_row_1 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 01"
   FAIL_REGULAR_EXPRESSION "Row 02|Row 10"
   LABELS "text-block;select"
)

# Test extracting row 5 only on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_long_select_row_5 -i "${MOCK_LONG_FILE}" -s 5 5 -X)
SET_TESTS_PROPERTIES(textblock_long_select_row_5 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 05"
   FAIL_REGULAR_EXPRESSION "Row 01|Row 04|Row 06|Row 10"
   LABELS "text-block;select"
)

# Test extracting row 10 only on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_long_select_row_10 -i "${MOCK_LONG_FILE}" -s 10 10 -X)
SET_TESTS_PROPERTIES(textblock_long_select_row_10 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 10"
   FAIL_REGULAR_EXPRESSION "Row 01|Row 09"
   LABELS "text-block;select"
)

# Test extracting all rows 1 through 10 on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_long_select_rows_1_to_10_all -i "${MOCK_LONG_FILE}" -s 1 10 -X)
SET_TESTS_PROPERTIES(textblock_long_select_rows_1_to_10_all PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 01.*Row 05.*Row 10"
   LABELS "text-block;select"
)

# Test deleting row 1 only on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_long_delete_row_1 -i "${MOCK_LONG_FILE}" -s 1 1 -D)
SET_TESTS_PROPERTIES(textblock_long_delete_row_1 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 02.*Row 10"
   FAIL_REGULAR_EXPRESSION "Row 01"
   LABELS "text-block;select"
)

# Test deleting row 5 only on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_long_delete_row_5 -i "${MOCK_LONG_FILE}" -s 5 5 -D)
SET_TESTS_PROPERTIES(textblock_long_delete_row_5 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 04\nRow 06"
   FAIL_REGULAR_EXPRESSION "Row 05"
   LABELS "text-block;select"
)

# Test deleting row 10 only on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_long_delete_row_10 -i "${MOCK_LONG_FILE}" -s 10 10 -D)
SET_TESTS_PROPERTIES(textblock_long_delete_row_10 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 01.*Row 09"
   FAIL_REGULAR_EXPRESSION "Row 10"
   LABELS "text-block;select"
)

# Test deleting all rows 1 through 10 on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_long_delete_rows_1_to_10_all -i "${MOCK_LONG_FILE}" -s 1 10 -D)
SET_TESTS_PROPERTIES(textblock_long_delete_rows_1_to_10_all PROPERTIES
   FAIL_REGULAR_EXPRESSION "Row [0-9]"
   LABELS "text-block;select"
)

# Test negative line index selecting row 10 on 10-line file (-s -1 -1)
ADD_TEXT_BLOCK_TEST(textblock_long_select_neg_row_10 -i "${MOCK_LONG_FILE}" -s -1 -1 -X)
SET_TESTS_PROPERTIES(textblock_long_select_neg_row_10 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 10"
   FAIL_REGULAR_EXPRESSION "Row 01|Row 09"
   LABELS "text-block;select"
)

# Test negative line index selecting row 1 on 10-line file (-s -10 -10)
ADD_TEXT_BLOCK_TEST(textblock_long_select_neg_row_1 -i "${MOCK_LONG_FILE}" -s -10 -10 -X)
SET_TESTS_PROPERTIES(textblock_long_select_neg_row_1 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 01"
   FAIL_REGULAR_EXPRESSION "Row 02|Row 10"
   LABELS "text-block;select"
)

# Test negative line index selecting first half of 10-line file (-s -10 -6)
ADD_TEXT_BLOCK_TEST(textblock_long_select_neg_first_half -i "${MOCK_LONG_FILE}" -s -10 -6 -X)
SET_TESTS_PROPERTIES(textblock_long_select_neg_first_half PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 01.*Row 05"
   FAIL_REGULAR_EXPRESSION "Row 06|Row 10"
   LABELS "text-block;select"
)

# Test negative line index selecting second half of 10-line file (-s -5 -1)
ADD_TEXT_BLOCK_TEST(textblock_long_select_neg_second_half -i "${MOCK_LONG_FILE}" -s -5 -1 -X)
SET_TESTS_PROPERTIES(textblock_long_select_neg_second_half PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 06.*Row 10"
   FAIL_REGULAR_EXPRESSION "Row 01|Row 05"
   LABELS "text-block;select"
)

# Test negative line index deleting first half of 10-line file (-s -10 -6 -D)
ADD_TEXT_BLOCK_TEST(textblock_long_delete_neg_first_half -i "${MOCK_LONG_FILE}" -s -10 -6 -D)
SET_TESTS_PROPERTIES(textblock_long_delete_neg_first_half PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 06.*Row 10"
   FAIL_REGULAR_EXPRESSION "Row 01|Row 05"
   LABELS "text-block;select"
)

# Test negative line index deleting second half of 10-line file (-s -5 -1 -D)
ADD_TEXT_BLOCK_TEST(textblock_long_delete_neg_second_half -i "${MOCK_LONG_FILE}" -s -5 -1 -D)
SET_TESTS_PROPERTIES(textblock_long_delete_neg_second_half PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 01.*Row 05"
   FAIL_REGULAR_EXPRESSION "Row 06|Row 10"
   LABELS "text-block;select"
)

# Test mixed positive and negative indexing covering entire 10-line file (-s 1 -1)
ADD_TEXT_BLOCK_TEST(textblock_long_select_mixed_pos1_neg1 -i "${MOCK_LONG_FILE}" -s 1 -1 -X)
SET_TESTS_PROPERTIES(textblock_long_select_mixed_pos1_neg1 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 01.*Row 10"
   LABELS "text-block;select"
)

# Test mixed positive and negative indexing covering rows 3 to 8 (-s 3 -3)
ADD_TEXT_BLOCK_TEST(textblock_long_select_mixed_pos3_neg3 -i "${MOCK_LONG_FILE}" -s 3 -3 -X)
SET_TESTS_PROPERTIES(textblock_long_select_mixed_pos3_neg3 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 03.*Row 08"
   FAIL_REGULAR_EXPRESSION "Row 01|Row 02|Row 09|Row 10"
   LABELS "text-block;select"
)

# Test mixed positive and negative indexing covering rows 5 to 10 (-s 5 -1)
ADD_TEXT_BLOCK_TEST(textblock_select_mixed_pos5_neg1 -i "${MOCK_LONG_FILE}" -s 5 -1 -X)
SET_TESTS_PROPERTIES(textblock_select_mixed_pos5_neg1 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 05.*Row 10"
   FAIL_REGULAR_EXPRESSION "Row 01|Row 04"
   LABELS "text-block;select"
)

# Test mixed positive and negative indexing deleting rows 3 to 8 (-s 3 -3 -D)
ADD_TEXT_BLOCK_TEST(textblock_long_delete_mixed_pos3_neg3 -i "${MOCK_LONG_FILE}" -s 3 -3 -D)
SET_TESTS_PROPERTIES(textblock_long_delete_mixed_pos3_neg3 PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 01.*Row 02\nRow 09.*Row 10"
   FAIL_REGULAR_EXPRESSION "Row 03|Row 08"
   LABELS "text-block;select"
)

# Test Insert Front driven by line range selection on 10-line file (-s 5 7 -F)
ADD_TEXT_BLOCK_TEST(textblock_long_insert_front_pos
   -i "${MOCK_LONG_FILE}" -s 5 7 -F "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_long_insert_front_pos PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 04\nINSERTED_PAYLOAD\nRow 05"
   LABELS "text-block;select;insert"
)

# Test Insert Back driven by line range selection on 10-line file (-s 5 7 -B)
ADD_TEXT_BLOCK_TEST(textblock_long_insert_back_pos
   -i "${MOCK_LONG_FILE}" -s 5 7 -B "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_long_insert_back_pos PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 07\nINSERTED_PAYLOAD\nRow 08"
   LABELS "text-block;select;insert"
)


#############################################################################
#### 19. More Line Selection Insert & Replace Variations                 ####
#############################################################################

# Test replacing the entire file with payload using line selection (-s 1 0 -R)
ADD_TEXT_BLOCK_TEST(textblock_select_replace_entire_file
   -i "${MOCK_LINES_FILE}" -s 1 0 -R "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_replace_entire_file PROPERTIES
   PASS_REGULAR_EXPRESSION "INSERTED_PAYLOAD"
   FAIL_REGULAR_EXPRESSION "Line [1-5]"
   LABELS "text-block;select;replace"
)

# Test inserting front on the entire file using line selection (-s 1 0 -F)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_front_entire_file
   -i "${MOCK_LINES_FILE}" -s 1 0 -F "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_front_entire_file PROPERTIES
   PASS_REGULAR_EXPRESSION "INSERTED_PAYLOAD\nLine 1.*Line 5"
   LABELS "text-block;select;insert"
)

# Test inserting back on the entire file using line selection (-s 1 0 -B)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_back_entire_file
   -i "${MOCK_LINES_FILE}" -s 1 0 -B "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_back_entire_file PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 5\nINSERTED_PAYLOAD"
   LABELS "text-block;select;insert"
)

# Test inserting front on the first line only (-s 1 1 -F)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_front_first_line
   -i "${MOCK_LINES_FILE}" -s 1 1 -F "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_front_first_line PROPERTIES
   PASS_REGULAR_EXPRESSION "INSERTED_PAYLOAD\nLine 1\nLine 2"
   LABELS "text-block;select;insert"
)

# Test inserting back on the first line only (-s 1 1 -B)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_back_first_line
   -i "${MOCK_LINES_FILE}" -s 1 1 -B "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_back_first_line PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nINSERTED_PAYLOAD\nLine 2"
   LABELS "text-block;select;insert"
)

# Test inserting front on the last line only (-s 5 5 -F)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_front_last_line
   -i "${MOCK_LINES_FILE}" -s 5 5 -F "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_front_last_line PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 4\nINSERTED_PAYLOAD\nLine 5"
   LABELS "text-block;select;insert"
)

# Test inserting back on the last line only (-s 5 5 -B)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_back_last_line
   -i "${MOCK_LINES_FILE}" -s 5 5 -B "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_back_last_line PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 5\nINSERTED_PAYLOAD"
   LABELS "text-block;select;insert"
)

# Test inserting front on the single negative last line (-s -1 -1 -F)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_front_neg_single
   -i "${MOCK_LINES_FILE}" -s -1 -1 -F "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_front_neg_single PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 4\nINSERTED_PAYLOAD\nLine 5"
   LABELS "text-block;select;insert"
)

# Test inserting back on the single negative last line (-s -1 -1 -B)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_back_neg_single
   -i "${MOCK_LINES_FILE}" -s -1 -1 -B "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_back_neg_single PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 5\nINSERTED_PAYLOAD"
   LABELS "text-block;select;insert"
)

# Test replacing the single negative last line (-s -1 -1 -R)
ADD_TEXT_BLOCK_TEST(textblock_select_replace_neg_single
   -i "${MOCK_LINES_FILE}" -s -1 -1 -R "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_replace_neg_single PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 4\nINSERTED_PAYLOAD"
   FAIL_REGULAR_EXPRESSION "Line 5"
   LABELS "text-block;select;replace"
)

# Test replacing the first two lines using negative offsets (-s -5 -4 -R)
ADD_TEXT_BLOCK_TEST(textblock_select_replace_neg_first_two
   -i "${MOCK_LINES_FILE}" -s -5 -4 -R "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_replace_neg_first_two PROPERTIES
   PASS_REGULAR_EXPRESSION "INSERTED_PAYLOAD\nLine 3"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2"
   LABELS "text-block;select;replace"
)

# Test replacing the last two lines using negative offsets (-s -2 -1 -R)
ADD_TEXT_BLOCK_TEST(textblock_select_replace_neg_last_two
   -i "${MOCK_LINES_FILE}" -s -2 -1 -R "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_replace_neg_last_two PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 3\nINSERTED_PAYLOAD"
   FAIL_REGULAR_EXPRESSION "Line 4|Line 5"
   LABELS "text-block;select;replace"
)


#############################################################################
#### 20. Enumerate Variations on Long and Tagged Files                   ####
#############################################################################

# Test default 6-digit enumeration on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_enumerate_long_default -i "${MOCK_LONG_FILE}" -E)
SET_TESTS_PROPERTIES(textblock_enumerate_long_default PROPERTIES
   PASS_REGULAR_EXPRESSION "000001.*Row 01.*000010.*Row 10"
   LABELS "text-block;enumerate"
)

# Test width-1 enumeration on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_enumerate_long_fmt_1 -i "${MOCK_LONG_FILE}" -E
   --enumerate-format 1 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_long_fmt_1 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1: Row 01.*#10: Row 10"
   LABELS "text-block;enumerate"
)

# Test 2-digit zero-padded enumeration on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_enumerate_long_fmt_02 -i "${MOCK_LONG_FILE}" -E
   --enumerate-format 02 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_long_fmt_02 PROPERTIES
   PASS_REGULAR_EXPRESSION "#01: Row 01.*#10: Row 10"
   LABELS "text-block;enumerate"
)

# Test 3-digit zero-padded enumeration on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_enumerate_long_fmt_03 -i "${MOCK_LONG_FILE}" -E
   --enumerate-format 03 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_long_fmt_03 PROPERTIES
   PASS_REGULAR_EXPRESSION "#001: Row 01.*#010: Row 10"
   LABELS "text-block;enumerate"
)

# Test 2-digit space-padded enumeration on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_enumerate_long_fmt_2 -i "${MOCK_LONG_FILE}" -E
   --enumerate-format 2 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_long_fmt_2 PROPERTIES
   PASS_REGULAR_EXPRESSION "# 1: Row 01.*#10: Row 10"
   LABELS "text-block;enumerate"
)

# Test 3-digit space-padded enumeration on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_enumerate_long_fmt_3 -i "${MOCK_LONG_FILE}" -E
   --enumerate-format 3 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_long_fmt_3 PROPERTIES
   PASS_REGULAR_EXPRESSION "#  1: Row 01.*# 10: Row 10"
   LABELS "text-block;enumerate"
)

# Test left-adjusted 2-digit enumeration on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_enumerate_long_fmt_left_2 -i "${MOCK_LONG_FILE}" -E
   --enumerate-format -2 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_long_fmt_left_2 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1 : Row 01.*#10: Row 10"
   LABELS "text-block;enumerate"
)

# Test left-adjusted 3-digit enumeration on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_enumerate_long_fmt_left_3 -i "${MOCK_LONG_FILE}" -E
   --enumerate-format -3 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_long_fmt_left_3 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1  : Row 01.*#10 : Row 10"
   LABELS "text-block;enumerate"
)

# Test default enumeration on tagged file fixture
ADD_TEXT_BLOCK_TEST(textblock_enumerate_tagged_default -i "${MOCK_TAGGED_FILE}" -E)
SET_TESTS_PROPERTIES(textblock_enumerate_tagged_default PROPERTIES
   PASS_REGULAR_EXPRESSION "000001.*Header.*000006.*Footer"
   LABELS "text-block;enumerate"
)

# Test 2-digit enumeration with custom labels on tagged file
ADD_TEXT_BLOCK_TEST(textblock_enumerate_tagged_fmt_2 -i "${MOCK_TAGGED_FILE}" -E
   --enumerate-format 2 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_tagged_fmt_2 PROPERTIES
   PASS_REGULAR_EXPRESSION "# 1: Header.*# 6: Footer"
   LABELS "text-block;enumerate"
)

# Test period/dot label suffix in enumeration mode
ADD_TEXT_BLOCK_TEST(textblock_enumerate_dot_label -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 2 --enumerate-label1 "#" --enumerate-label2 ". "
)
SET_TESTS_PROPERTIES(textblock_enumerate_dot_label PROPERTIES
   PASS_REGULAR_EXPRESSION "# 1\\. Line 1.*# 5\\. Line 5"
   LABELS "text-block;enumerate"
)

# Test dash-padded labels in enumeration mode
ADD_TEXT_BLOCK_TEST(textblock_enumerate_dash_label -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 2 --enumerate-label1 "- " --enumerate-label2 " - "
)
SET_TESTS_PROPERTIES(textblock_enumerate_dash_label PROPERTIES
   PASS_REGULAR_EXPRESSION "-  1 - Line 1.*-  5 - Line 5"
   LABELS "text-block;enumerate"
)


#############################################################################
#### 21. Highlight Style Variants & Symbols                              ####
#############################################################################

# Test bracket symbol markers in Highlight mode
ADD_TEXT_BLOCK_TEST(textblock_highlight_symbols_brackets
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-begin ">>>" --highlight-end "<<<"
)
SET_TESTS_PROPERTIES(textblock_highlight_symbols_brackets PROPERTIES
   PASS_REGULAR_EXPRESSION ">>>.*Block line 1.*<<<"
   LABELS "text-block;highlight"
)

# Test star symbol markers in Highlight mode
ADD_TEXT_BLOCK_TEST(textblock_highlight_symbols_stars
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-begin "***" --highlight-end "***"
)
SET_TESTS_PROPERTIES(textblock_highlight_symbols_stars PROPERTIES
   PASS_REGULAR_EXPRESSION "\\*\\*\\*.*Block line 1.*\\*\\*\\*"
   LABELS "text-block;highlight"
)

# Test HTML bold tag markers in Highlight mode
ADD_TEXT_BLOCK_TEST(textblock_highlight_symbols_html
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-begin "<b>" --highlight-end "</b>"
)
SET_TESTS_PROPERTIES(textblock_highlight_symbols_html PROPERTIES
   PASS_REGULAR_EXPRESSION "<b>.*Block line 1.*</b>"
   LABELS "text-block;highlight"
)

# Test Markdown bold syntax markers in Highlight mode
ADD_TEXT_BLOCK_TEST(textblock_highlight_symbols_markdown
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-begin "**" --highlight-end "**"
)
SET_TESTS_PROPERTIES(textblock_highlight_symbols_markdown PROPERTIES
   PASS_REGULAR_EXPRESSION "\\*\\*.*Block line 1.*\\*\\*"
   LABELS "text-block;highlight"
)

# Test lightning emoji symbol markers in Highlight mode
ADD_TEXT_BLOCK_TEST(textblock_highlight_symbols_lightning
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-begin "⚡" --highlight-end "⚡"
)
SET_TESTS_PROPERTIES(textblock_highlight_symbols_lightning PROPERTIES
   PASS_REGULAR_EXPRESSION "⚡.*Block line 1.*⚡"
   LABELS "text-block;highlight"
)

# Test highlighting positive line range in 10-line file (-s 4 7)
ADD_TEXT_BLOCK_TEST(textblock_highlight_long_select -i "${MOCK_LONG_FILE}" -H -s 4 7)
SET_TESTS_PROPERTIES(textblock_highlight_long_select PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 03.*⭐.*Row 04.*Row 07.*🛑.*Row 08"
   LABELS "text-block;highlight"
)

# Test highlighting negative line range in 10-line file (-s -4 -2)
ADD_TEXT_BLOCK_TEST(textblock_highlight_long_select_neg -i "${MOCK_LONG_FILE}" -H -s -4 -2)
SET_TESTS_PROPERTIES(textblock_highlight_long_select_neg PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 06.*⭐.*Row 07.*Row 09.*🛑.*Row 10"
   LABELS "text-block;highlight"
)

# Test highlighting top lines in 10-line file (-s 1 3)
ADD_TEXT_BLOCK_TEST(textblock_highlight_long_select_top -i "${MOCK_LONG_FILE}" -H -s 1 3)
SET_TESTS_PROPERTIES(textblock_highlight_long_select_top PROPERTIES
   PASS_REGULAR_EXPRESSION "⭐.*Row 01.*Row 03.*🛑.*Row 04"
   LABELS "text-block;highlight"
)

# Test highlighting bottom lines in 10-line file (-s 8 10)
ADD_TEXT_BLOCK_TEST(textblock_highlight_long_select_bottom -i "${MOCK_LONG_FILE}" -H -s 8 10)
SET_TESTS_PROPERTIES(textblock_highlight_long_select_bottom PROPERTIES
   PASS_REGULAR_EXPRESSION "Row 07.*⭐.*Row 08.*Row 10.*🛑"
   LABELS "text-block;highlight"
)

# Test highlighting 3 blocks across file with full tag lines
ADD_TEXT_BLOCK_TEST(textblock_highlight_three_blocks
   -i "${MOCK_THREE_BLOCKS_FILE}" -H -b "<B>" -e "</B>" -f
)
SET_TESTS_PROPERTIES(textblock_highlight_three_blocks PROPERTIES
   PASS_REGULAR_EXPRESSION "⭐.*One.*🛑.*⭐.*Two.*🛑.*⭐.*Three.*🛑"
   LABELS "text-block;highlight"
)


#############################################################################
#### 22. Action Limits Combinations                                      ####
#############################################################################

# Test zero minimum and zero maximum on non-matching tags
ADD_TEXT_BLOCK_TEST(textblock_actions_zero_min_zero_max
   -i "${MOCK_LINES_FILE}" -X -b "<NOTFOUND>" -e "</NOTFOUND>" -m 0 -M 0
)

# Test action range 1 to 2 matching a single block
ADD_TEXT_BLOCK_TEST(textblock_actions_single_range_1_to_2
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -m 1 -M 2
)

# Test action range 0 to 1 matching a single block
ADD_TEXT_BLOCK_TEST(textblock_actions_single_range_0_to_1
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -m 0 -M 1
)

# Test exact matching on multi-block file (m=2, M=2)
ADD_TEXT_BLOCK_TEST(textblock_actions_multi_exact_2_min_max
   -i "${MOCK_MULTI_TAG_FILE}" -X -b "<BEGIN>" -e "<END>" -m 2 -M 2
)

# Test broad action range 0 to 5 matching multi-block file
ADD_TEXT_BLOCK_TEST(textblock_actions_multi_range_0_to_5
   -i "${MOCK_MULTI_TAG_FILE}" -X -b "<BEGIN>" -e "<END>" -m 0 -M 5
)

# Test action range 1 to 5 matching 3-block file
ADD_TEXT_BLOCK_TEST(textblock_actions_three_range_1_to_5
   -i "${MOCK_THREE_BLOCKS_FILE}" -X -b "<B>" -e "</B>" -m 1 -M 5
)

# Test action range 1 to 2 matching inline multi-block line
ADD_TEXT_BLOCK_TEST(textblock_actions_inline_range_1_to_2
   -i "${MOCK_MULTI_BLOCK_FILE}" -X -b "[B]" -e "[/B]" -m 1 -M 2
)

# Test action limits 1 to 1 matching line selection range
ADD_TEXT_BLOCK_TEST(textblock_actions_select_range_1_to_1
   -i "${MOCK_LINES_FILE}" -s 1 5 -X -m 1 -M 1
)

# Test action limits 0 to 2 matching line selection range
ADD_TEXT_BLOCK_TEST(textblock_actions_select_range_0_to_2
   -i "${MOCK_LINES_FILE}" -s 1 5 -X -m 0 -M 2
)


#############################################################################
#### 23. In-Place Update Multi-Target Testing                            ####
#############################################################################

# Test in-place Delete mode on tag-delimited file
ADD_TEXT_BLOCK_TEST(textblock_inplace_delete_tags
   -i "${TEST_INPLACE_DEL_FILE}" -p -D -b "<B>" -e "</B>" -f -y
)

# Test in-place Replace mode on tag-delimited file
ADD_TEXT_BLOCK_TEST(textblock_inplace_replace_tags
   -i "${TEST_INPLACE_REP_FILE}" -p -R "${MOCK_INSERT_FILE}" -b "<B>" -e "</B>" -f -y
)

# Test in-place Insert Front mode on tag-delimited file
ADD_TEXT_BLOCK_TEST(textblock_inplace_insert_front
   -i "${TEST_INPLACE_INSF_FILE}" -p -F "${MOCK_INSERT_FILE}" -b "<B>" -e "</B>" -f
)

# Test in-place Insert Back mode on tag-delimited file
ADD_TEXT_BLOCK_TEST(textblock_inplace_insert_back
   -i "${TEST_INPLACE_INSB_FILE}" -p -B "${MOCK_INSERT_FILE}" -b "<B>" -e "</B>" -f
)

# Test in-place Replace mode driven by line range selection
ADD_TEXT_BLOCK_TEST(textblock_inplace_select_replace
   -i "${TEST_INPLACE_FILE}" -p -s 2 3 -R "${MOCK_INSERT_FILE}"
)

# Test in-place Delete mode on inline multi-block file
ADD_TEXT_BLOCK_TEST(textblock_inplace_multiblock
   -i "${TEST_INPLACE_MB_FILE}" -p -D -b "[B]" -e "[/B]" -y
)


#############################################################################
#### 24. File Output Redirection & Append Variants                       ####
#############################################################################

SET(TEST_OUT_LONG_FILE "${CMAKE_CURRENT_BINARY_DIR}/out_long.txt")
SET(TEST_OUT_3B_FILE   "${CMAKE_CURRENT_BINARY_DIR}/out_3b.txt")

# Test redirecting Cat output from 10-line file
ADD_TEXT_BLOCK_TEST(textblock_out_redirect_long_cat
   -i "${MOCK_LONG_FILE}" -o "${TEST_OUT_LONG_FILE}" -C
)

# Test appending Enumerate output from 10-line file (-a)
ADD_TEXT_BLOCK_TEST(textblock_out_redirect_long_enumerate
   -i "${MOCK_LONG_FILE}" -o "${TEST_OUT_LONG_FILE}" -a -E
)

# Test redirecting extraction output from 3-block file
ADD_TEXT_BLOCK_TEST(textblock_out_redirect_three_blocks_extract
   -i "${MOCK_THREE_BLOCKS_FILE}" -o "${TEST_OUT_3B_FILE}" -X -b "<B>" -e "</B>" -f -y
)

# Test appending deletion output from 3-block file (-a)
ADD_TEXT_BLOCK_TEST(textblock_out_redirect_three_blocks_delete
   -i "${MOCK_THREE_BLOCKS_FILE}" -o "${TEST_OUT_3B_FILE}" -a -D -b "<B>" -e "</B>" -f -y
)

# Test stdout placeholder (-o -) in Enumerate mode
ADD_TEXT_BLOCK_TEST(textblock_out_stdout_placeholder_enumerate
   -i "${MOCK_LINES_FILE}" -o "-" -E
)
SET_TESTS_PROPERTIES(textblock_out_stdout_placeholder_enumerate PROPERTIES
   PASS_REGULAR_EXPRESSION "000001.*Line 1.*000005.*Line 5"
   LABELS "text-block;io"
)

# Test stdout placeholder (-o -) in Delete mode
ADD_TEXT_BLOCK_TEST(textblock_out_stdout_placeholder_delete
   -i "${MOCK_TAGGED_FILE}" -o "-" -D -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_out_stdout_placeholder_delete PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line\nFooter line"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block line"
   LABELS "text-block;io"
)


#############################################################################
#### 25. Warnings on Tag / Mode Mismatches & Suppression                 ####
#############################################################################

# Test non-fatal warning when begin-tag is passed to Cat mode
ADD_TEXT_BLOCK_TEST(textblock_warning_begin_tag_cat
   -i "${MOCK_LINES_FILE}" -C -b "<B>"
)
SET_TESTS_PROPERTIES(textblock_warning_begin_tag_cat PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Begin/end tags.*have no effect for Cat"
   LABELS "text-block;warning"
)

# Test non-fatal warning when end-tag is passed to Cat mode
ADD_TEXT_BLOCK_TEST(textblock_warning_end_tag_cat
   -i "${MOCK_LINES_FILE}" -C -e "<E>"
)
SET_TESTS_PROPERTIES(textblock_warning_end_tag_cat PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Begin/end tags.*have no effect for Cat"
   LABELS "text-block;warning"
)

# Test non-fatal warning when single tag is passed to Cat mode
ADD_TEXT_BLOCK_TEST(textblock_warning_single_tag_cat
   -i "${MOCK_LINES_FILE}" -C -t "<T>"
)
SET_TESTS_PROPERTIES(textblock_warning_single_tag_cat PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Begin/end tags.*have no effect for Cat"
   LABELS "text-block;warning"
)

# Test non-fatal warning when begin-tag is passed to Discard mode
ADD_TEXT_BLOCK_TEST(textblock_warning_begin_tag_discard
   -i "${MOCK_LINES_FILE}" -0 -b "<B>"
)
SET_TESTS_PROPERTIES(textblock_warning_begin_tag_discard PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Begin/end tags.*have no effect for Cat, Discard, or Enumerate"
   LABELS "text-block;warning"
)

# Test non-fatal warning when begin-tag is passed to Enumerate mode
ADD_TEXT_BLOCK_TEST(textblock_warning_begin_tag_enumerate
   -i "${MOCK_LINES_FILE}" -E -b "<B>"
)
SET_TESTS_PROPERTIES(textblock_warning_begin_tag_enumerate PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Begin/end tags.*have no effect for Cat, Discard, or Enumerate"
   LABELS "text-block;warning"
)

# Test non-fatal warning on inverted line selection on 10-line file
ADD_TEXT_BLOCK_TEST(textblock_warning_inverted_select_long
   -i "${MOCK_LONG_FILE}" -s 8 3 -X
)
SET_TESTS_PROPERTIES(textblock_warning_inverted_select_long PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Line selection range.*not useful"
   LABELS "text-block;warning"
)

# Test suppressing Cat mode tag warning via short flag -q
ADD_TEXT_BLOCK_TEST(textblock_suppress_warning_begin_cat
   -i "${MOCK_LINES_FILE}" -C -b "<B>" -q
)
SET_TESTS_PROPERTIES(textblock_suppress_warning_begin_cat PROPERTIES
   FAIL_REGULAR_EXPRESSION "WARNING:"
   LABELS "text-block;warning"
)

# Test suppressing inverted line selection warning on 10-line file via -q
ADD_TEXT_BLOCK_TEST(textblock_suppress_warning_inverted_long
   -i "${MOCK_LONG_FILE}" -s 8 3 -X -q
)
SET_TESTS_PROPERTIES(textblock_suppress_warning_inverted_long PROPERTIES
   FAIL_REGULAR_EXPRESSION "WARNING:"
   LABELS "text-block;warning"
)


#############################################################################
#### 26. CLI Error Handling & Negative Boundaries                        ####
#############################################################################

# Test failure when minimum actions exceeds maximum actions (-m > -M)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_min_greater_than_max -m 5 -M 2)

# Test mutual exclusivity between line selection and begin-tag
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_select_and_begin_tag -i "${MOCK_LINES_FILE}" -s 1 3 -b "<BEGIN>")

# Test mutual exclusivity between line selection and end-tag
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_select_and_end_tag   -i "${MOCK_LINES_FILE}" -s 1 3 -e "<END>")

# Test mutual exclusivity between line selection and both begin/end tags
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_select_and_both_tags -i "${MOCK_LINES_FILE}" -s 1 3 -b "<BEGIN>" -e "<END>")

# Test mutual exclusivity between line selection and single tag (-t)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_select_and_single_tag -i "${MOCK_LINES_FILE}" -s 1 3 -t "<MARKER>")

# Test error when line selection (-s) is missing both arguments
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_select_missing_both_args -i "${MOCK_LINES_FILE}" -s)

# Test error when line selection (-s) is missing the second (to_line) argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_select_missing_to_line   -i "${MOCK_LINES_FILE}" -s 2)

# Test error when negative line numbers are used without an input file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_negative_lines_without_input_file
   -s -2 -1 -D
)

# Test mutual exclusivity between in-place update (-p) and output redirection (-o)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_inplace_and_output
   -i "${MOCK_LINES_FILE}" -p -o out.txt
)

# Test error when in-place update (-p) is used without an input file (-i)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_inplace_without_input -p)

# Test error when Insert Front is invoked without an insert file argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_insert_front_no_file_arg -F)

# Test error when Insert Back is invoked without an insert file argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_insert_back_no_file_arg  -B)

# Test error when Replace is invoked without an insert file argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_replace_no_file_arg      -R)

# Test error when specifying a non-existent input file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_nonexistent_input
   -i "/nonexistent/path/file.txt"
)

# Test error when specifying a non-existent insert file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_nonexistent_insert_file
   -i "${MOCK_TAGGED_FILE}" -F "/nonexistent/insert_file.txt" -b "<BEGIN>" -e "<END>"
)

# Test error when inserting from stdin ("-") while the main input is also stdin
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_insert_stdin_without_input_file
   -F "-" -b "<BEGIN>" -e "<END>"
)

# Test error when enumeration format contains disallowed '%' character
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_invalid_enum_percent
   -E --enumerate-format "%d"
)

# Test error when enumeration format contains disallowed punctuation character '!'
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_invalid_enum_punctuation
   -E --enumerate-format "4!"
)

# Test error when enumeration format contains disallowed whitespace
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_invalid_enum_space
   -E --enumerate-format "4 4"
)

# Test error when enumeration format contains disallowed special character '$'
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_invalid_enum_special
   -E --enumerate-format "4$"
)

# Test error when --highlight-param is called with zero arguments (requires 6)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_highlight_param_0_args
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" --highlight-param
)

# Test error when --highlight-param is called with only 1 argument (requires 6)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_highlight_param_1_arg
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" --highlight-param "A"
)

# Test error when --highlight-param is called with only 2 arguments (requires 6)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_highlight_param_2_args
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" --highlight-param "A" "B"
)

# Test error when --highlight-param is called with only 3 arguments (requires 6)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_highlight_param_3_args
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" --highlight-param "A" "B" "C"
)

# Test error when --highlight-param is called with only 4 arguments (requires 6)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_highlight_param_4_args
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" --highlight-param "A" "B" "C" "D"
)

# Test error when --highlight-param is called with only 5 arguments (requires 6)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_highlight_param_5_args
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" --highlight-param "A" "B" "C" "D" "E"
)

# Test error when -b is missing its required argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_b -b)

# Test error when -e is missing its required argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_e -e)

# Test error when -t is missing its required argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_t -t)

# Test error when -m is missing its required argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_m -m)

# Test error when -M is missing its required argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_M -M)

# Test error when -i is missing its required argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_i -i)

# Test error when -o is missing its required argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_o -o)

# Test error when --enumerate-format is missing its argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_enumerate_format --enumerate-format)

# Test error when --enumerate-label1 is missing its argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_enumerate_label1 --enumerate-label1)

# Test error when --enumerate-label2 is missing its argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_enumerate_label2 --enumerate-label2)

# Test error when --highlight-begin is missing its argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_highlight_begin  --highlight-begin)

# Test error when --highlight-end is missing its argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_highlight_end    --highlight-end)

# Test error when --highlight-unmarked1 is missing its argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_highlight_unmarked1 --highlight-unmarked1)

# Test error when --highlight-unmarked2 is missing its argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_highlight_unmarked2 --highlight-unmarked2)

# Test error when --highlight-marked1 is missing its argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_highlight_marked1   --highlight-marked1)

# Test error when --highlight-marked2 is missing its argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_opt_missing_highlight_marked2   --highlight-marked2)

# Test error when extraneous positional arguments are passed
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_extraneous_positional_args
   -i "${MOCK_LINES_FILE}" -C extra_positional_parameter
)

# Test error when an unrecognized short option flag is passed
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_unknown_short_opt -i "${MOCK_LINES_FILE}" -Z)

# Test error when an unrecognized long option flag is passed
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_unknown_long_opt  -i "${MOCK_LINES_FILE}" --definitely-not-an-option)

# Test error when invalid option flag is passed
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_unknown_flag      --invalid-flag)
