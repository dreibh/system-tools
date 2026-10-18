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

SET(MOCK_TAGGED_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_tagged.txt")
FILE(WRITE "${MOCK_TAGGED_FILE}"
"Header line\n"
"<BEGIN>\n"
"Block line 1\n"
"Block line 2\n"
"<END>\n"
"Footer line\n"
)

SET(MOCK_INSERT_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_insert.txt")
FILE(WRITE "${MOCK_INSERT_FILE}"
"INSERTED_PAYLOAD\n"
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
#### Basic CLI Options                                                   ####
#############################################################################

ADD_TEXT_BLOCK_TEST(textblock_cli_version_short -v)
ADD_TEXT_BLOCK_TEST(textblock_cli_version_long  --version)
ADD_TEXT_BLOCK_TEST(textblock_cli_help_short    -h)
ADD_TEXT_BLOCK_TEST(textblock_cli_help_long     --help)

SET_TESTS_PROPERTIES(textblock_cli_version_short textblock_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "text-block [0-9]+\\.[0-9]+"
)

SET_TESTS_PROPERTIES(textblock_cli_help_short textblock_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# === CLI Help Internationalization =========================================
ADD_TEXT_BLOCK_TEST(textblock_cli_help_i18n_de -h)
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
#### Cat and Discard Modes                                               ####
#############################################################################

# Cat mode (explicit -C and --cat)
ADD_TEXT_BLOCK_TEST(textblock_cat_short -i "${MOCK_LINES_FILE}" -C)
ADD_TEXT_BLOCK_TEST(textblock_cat_long  -i "${MOCK_LINES_FILE}" --cat)

SET_TESTS_PROPERTIES(textblock_cat_short textblock_cat_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 2.*Line 3.*Line 4.*Line 5"
   LABELS "text-block;cat"
)

# Discard mode (-0 / --discard: produces no output)
ADD_TEXT_BLOCK_TEST(textblock_discard_short -i "${MOCK_LINES_FILE}" -0)
ADD_TEXT_BLOCK_TEST(textblock_discard_long  -i "${MOCK_LINES_FILE}" --discard)

SET_TESTS_PROPERTIES(textblock_discard_short textblock_discard_long PROPERTIES
   FAIL_REGULAR_EXPRESSION "Line [1-5]"
   LABELS "text-block;discard"
)


#############################################################################
#### Enumerate Mode (-E / --enumerate)                                   ####
#############################################################################

# Default 6-digit zero-padded line numbering
ADD_TEXT_BLOCK_TEST(textblock_enumerate_default -i "${MOCK_LINES_FILE}" -E)
SET_TESTS_PROPERTIES(textblock_enumerate_default PROPERTIES
   PASS_REGULAR_EXPRESSION "000001.*Line 1.*000005.*Line 5"
   LABELS "text-block;enumerate"
)

# Custom formatting and non-ANSI labels
ADD_TEXT_BLOCK_TEST(textblock_enumerate_custom -i "${MOCK_LINES_FILE}" --enumerate
   --enumerate-format 3
   --enumerate-label1 "#"
   --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_custom PROPERTIES
   PASS_REGULAR_EXPRESSION "#  1.*Line 1"
   LABELS "text-block;enumerate"
)


#############################################################################
#### Tag-Based Extraction (-X / --extract)                               ####
#############################################################################

# Extract body excluding tags (default behavior)
ADD_TEXT_BLOCK_TEST(textblock_extract_exclude_tags
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_extract_exclude_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 1.*Block line 2"
   FAIL_REGULAR_EXPRESSION "Header line|Footer line|<BEGIN>|<END>"
   LABELS "text-block;extract"
)

# Extract body including tags (-y / --include-tags)
ADD_TEXT_BLOCK_TEST(textblock_extract_include_tags
   -i "${MOCK_TAGGED_FILE}" --extract -b "<BEGIN>" -e "<END>" --include-tags
)
SET_TESTS_PROPERTIES(textblock_extract_include_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>.*Block line 1.*Block line 2.*<END>"
   FAIL_REGULAR_EXPRESSION "Header line|Footer line"
   LABELS "text-block;extract"
)


#############################################################################
#### Tag-Based Deletion (-D / --delete / --remove)                       ####
#############################################################################

# Delete body but leave tags
ADD_TEXT_BLOCK_TEST(textblock_delete_keep_tags
   -i "${MOCK_TAGGED_FILE}" -D -b "<BEGIN>" -e "<END>" -x
)
SET_TESTS_PROPERTIES(textblock_delete_keep_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line.*<BEGIN><END>.*Footer line"
   FAIL_REGULAR_EXPRESSION "Block line"
   LABELS "text-block;delete"
)

# Delete full tag lines completely (-f -y)
ADD_TEXT_BLOCK_TEST(textblock_delete_full_lines
   -i "${MOCK_TAGGED_FILE}" --delete -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_delete_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line.*Footer line"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block line"
   LABELS "text-block;delete"
)


#############################################################################
#### Tag-Based Insertion & Replacement (-F, -B, -R)                      ####
#############################################################################

# Insert Front (-F / --insert-front)
ADD_TEXT_BLOCK_TEST(textblock_insert_front
   -i "${MOCK_TAGGED_FILE}" -F "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_insert_front PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>INSERTED_PAYLOAD.*Block line 1"
   LABELS "text-block;insert"
)

# Insert Back (-B / --insert-back)
ADD_TEXT_BLOCK_TEST(textblock_insert_back
   -i "${MOCK_TAGGED_FILE}" --insert-back "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_insert_back PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 2.*INSERTED_PAYLOAD.*<END>"
   LABELS "text-block;insert"
)

# Replace block content and tags (-R -f -y)
ADD_TEXT_BLOCK_TEST(textblock_replace_full
   -i "${MOCK_TAGGED_FILE}" -R "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_replace_full PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line.*INSERTED_PAYLOAD.*Footer line"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block line"
   LABELS "text-block;replace"
)


#############################################################################
#### Highlight Mode (-H / --highlight)                                   ####
#############################################################################

# Visual highlight markers with custom text labels
ADD_TEXT_BLOCK_TEST(textblock_highlight_custom_labels
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-begin "{START}"
   --highlight-end "{STOP}"
   --highlight-unmarked1 "(u:"
   --highlight-unmarked2 ":u)"
   --highlight-marked1 "(m:"
   --highlight-marked2 ":m)"
)
SET_TESTS_PROPERTIES(textblock_highlight_custom_labels PROPERTIES
   PASS_REGULAR_EXPRESSION "\\(u:Header line.*{START}.*\\(m:Block line 1.*{STOP}.*\\(u:Footer line"
   LABELS "text-block;highlight"
)


#############################################################################
#### Line Range Selection (-s / --select)                                ####
#############################################################################

# Positive line range deletion (lines 2 through 4)
ADD_TEXT_BLOCK_TEST(textblock_select_delete_middle
   -i "${MOCK_LINES_FILE}" -s 2 4 -D
)
SET_TESTS_PROPERTIES(textblock_select_delete_middle PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 5"
   FAIL_REGULAR_EXPRESSION "Line 2|Line 3|Line 4"
   LABELS "text-block;select"
)

# Open-ended line range extraction (line 3 to end of file: 0)
ADD_TEXT_BLOCK_TEST(textblock_select_extract_to_end
   -i "${MOCK_LINES_FILE}" --select 3 0 -X
)
SET_TESTS_PROPERTIES(textblock_select_extract_to_end PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 3.*Line 4.*Line 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2"
   LABELS "text-block;select"
)

# Negative line numbers (from end of file: -2 through -1 removes lines 4 and 5)
ADD_TEXT_BLOCK_TEST(textblock_select_negative_lines
   -i "${MOCK_LINES_FILE}" -s -2 -1 -D
)
SET_TESTS_PROPERTIES(textblock_select_negative_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 2.*Line 3"
   FAIL_REGULAR_EXPRESSION "Line 4|Line 5"
   LABELS "text-block;select"
)


#############################################################################
#### Action Limits (-m / --min-actions, -M / --max-actions)              ####
#############################################################################

# Exactly 1 action expected and executed -> success
ADD_TEXT_BLOCK_TEST(textblock_actions_exact_match
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -m 1 -M 1
)

# Under min-actions: file has 1 block, min required is 2 -> failure
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_under_min_actions
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" --min-actions 2
)

# Over max-actions: file has 1 block, max allowed is 0 -> failure
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_over_max_actions
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" --max-actions 0
)


#############################################################################
#### File I/O Operations (-o, -a, -p)                                    ####
#############################################################################

SET(TEST_OUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/out_redirect.txt")

# Output redirection (-o / --output)
ADD_TEXT_BLOCK_TEST(textblock_output_file
   -i "${MOCK_LINES_FILE}" -o "${TEST_OUT_FILE}" -C
)

# Append mode (-a / --append)
ADD_TEXT_BLOCK_TEST(textblock_append_file
   -i "${MOCK_LINES_FILE}" -o "${TEST_OUT_FILE}" -a -C
)

# In-place file update (-p / --in-place)
SET(TEST_INPLACE_FILE "${CMAKE_CURRENT_BINARY_DIR}/inplace_target.txt")
FILE(WRITE "${TEST_INPLACE_FILE}" "Alpha\nBeta\nGamma\n")

ADD_TEXT_BLOCK_TEST(textblock_inplace_update
   -i "${TEST_INPLACE_FILE}" -p -s 2 2 -D
)


#############################################################################
#### CLI Error Handling & Mutual Exclusivity                             ####
#############################################################################

# Inverted min/max actions (-m > -M)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_min_greater_than_max -m 5 -M 2)

# Line selection and tags are mutually exclusive
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_select_and_tags -s 1 3 -b "<BEGIN>")

# In-place update and output file redirection are mutually exclusive
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_inplace_and_output -i "${MOCK_LINES_FILE}" -p -o out.txt)

# In-place update without an input file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_inplace_without_input -p)

# Missing insert file for insert modes
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_insert_front_no_file -F)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_insert_back_no_file  -B)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_replace_no_file      -R)

# Invalid enumeration format (disallowed characters)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_invalid_enum_format -E --enumerate-format "%d")

# Non-existent input file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_nonexistent_input -i "/nonexistent/path/file.txt")

# Unknown flag
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_unknown_flag --invalid-flag)
