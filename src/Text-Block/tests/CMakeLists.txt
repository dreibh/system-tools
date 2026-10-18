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

SET(MOCK_INSERT_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_insert.txt")
FILE(WRITE "${MOCK_INSERT_FILE}"
"INSERTED_PAYLOAD\n"
)

SET(MOCK_EMPTY_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_empty.txt")
FILE(WRITE "${MOCK_EMPTY_FILE}" "")


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
#### 2. Cat and Discard Modes                                            ####
#############################################################################

# Cat mode explicit and default (no mode flag defaults to Cat)
ADD_TEXT_BLOCK_TEST(textblock_cat_short   -i "${MOCK_LINES_FILE}" -C)
ADD_TEXT_BLOCK_TEST(textblock_cat_long    -i "${MOCK_LINES_FILE}" --cat)
ADD_TEXT_BLOCK_TEST(textblock_cat_default -i "${MOCK_LINES_FILE}")
ADD_TEXT_BLOCK_TEST(textblock_cat_empty   -i "${MOCK_EMPTY_FILE}" -C)

SET_TESTS_PROPERTIES(textblock_cat_short textblock_cat_long textblock_cat_default PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 2.*Line 3.*Line 4.*Line 5"
   LABELS "text-block;cat"
)
SET_TESTS_PROPERTIES(textblock_cat_empty PROPERTIES
   FAIL_REGULAR_EXPRESSION "Line|Header|Footer"
   LABELS "text-block;cat"
)

# Discard mode (-0 / --discard: completely suppress output)
ADD_TEXT_BLOCK_TEST(textblock_discard_short -i "${MOCK_LINES_FILE}" -0)
ADD_TEXT_BLOCK_TEST(textblock_discard_long  -i "${MOCK_LINES_FILE}" --discard)
ADD_TEXT_BLOCK_TEST(textblock_discard_empty -i "${MOCK_EMPTY_FILE}" -0)

SET_TESTS_PROPERTIES(textblock_discard_short textblock_discard_long textblock_discard_empty PROPERTIES
   FAIL_REGULAR_EXPRESSION "Line [1-5]"
   LABELS "text-block;discard"
)


#############################################################################
#### 3. Enumerate Mode Formatting Variants                               ####
#############################################################################

# Default 6-digit zero-padded line numbering
ADD_TEXT_BLOCK_TEST(textblock_enumerate_default_short -i "${MOCK_LINES_FILE}" -E)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_default_long  -i "${MOCK_LINES_FILE}" --enumerate)
SET_TESTS_PROPERTIES(textblock_enumerate_default_short textblock_enumerate_default_long PROPERTIES
   PASS_REGULAR_EXPRESSION "000001.*Line 1.*000005.*Line 5"
   LABELS "text-block;enumerate"
)

# Width 1 (unpadded)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_1 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 1 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_1 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1: Line 1.*#5: Line 5"
   LABELS "text-block;enumerate"
)

# Zero-padded 2-digit (--enumerate-format 02)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_02 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 02 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_02 PROPERTIES
   PASS_REGULAR_EXPRESSION "#01: Line 1.*#05: Line 5"
   LABELS "text-block;enumerate"
)

# Zero-padded 4-digit (--enumerate-format 04)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_04 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 04 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_04 PROPERTIES
   PASS_REGULAR_EXPRESSION "#0001: Line 1.*#0005: Line 5"
   LABELS "text-block;enumerate"
)

# Space-padded 3-digit (--enumerate-format 3)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_3 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 3 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_3 PROPERTIES
   PASS_REGULAR_EXPRESSION "#  1: Line 1.*#  5: Line 5"
   LABELS "text-block;enumerate"
)

# Space-padded 4-digit (--enumerate-format 4)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_4 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 4 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_4 PROPERTIES
   PASS_REGULAR_EXPRESSION "#   1: Line 1.*#   5: Line 5"
   LABELS "text-block;enumerate"
)

# Space-padded 8-digit (--enumerate-format 8)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_8 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 8 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_8 PROPERTIES
   PASS_REGULAR_EXPRESSION "#       1: Line 1.*#       5: Line 5"
   LABELS "text-block;enumerate"
)

# Left-adjusted 2-digit (--enumerate-format -2)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_left_2 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format -2 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_left_2 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1 : Line 1.*#5 : Line 5"
   LABELS "text-block;enumerate"
)

# Left-adjusted 4-digit (--enumerate-format -4)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_left_4 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format -4 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_left_4 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1   : Line 1.*#5   : Line 5"
   LABELS "text-block;enumerate"
)

# Left-adjusted 6-digit (--enumerate-format -6)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_fmt_left_6 -i "${MOCK_LINES_FILE}" -E
   --enumerate-format -6 --enumerate-label1 "#" --enumerate-label2 ": "
)
SET_TESTS_PROPERTIES(textblock_enumerate_fmt_left_6 PROPERTIES
   PASS_REGULAR_EXPRESSION "#1     : Line 1.*#5     : Line 5"
   LABELS "text-block;enumerate"
)

# Custom prefix label only (--enumerate-label1)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_label1_only -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 2 --enumerate-label1 "LINE_"
)
SET_TESTS_PROPERTIES(textblock_enumerate_label1_only PROPERTIES
   PASS_REGULAR_EXPRESSION "LINE_ 1.*LINE_ 5"
   LABELS "text-block;enumerate"
)

# Custom suffix label only (--enumerate-label2)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_label2_only -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 2 --enumerate-label2 " | "
)
SET_TESTS_PROPERTIES(textblock_enumerate_label2_only PROPERTIES
   PASS_REGULAR_EXPRESSION " 1 \\| Line 1.* 5 \\| Line 5"
   LABELS "text-block;enumerate"
)

# Both custom labels (Arrow formatting)
ADD_TEXT_BLOCK_TEST(textblock_enumerate_arrow_labels -i "${MOCK_LINES_FILE}" -E
   --enumerate-format 3 --enumerate-label1 "--> " --enumerate-label2 " <-- "
)
SET_TESTS_PROPERTIES(textblock_enumerate_arrow_labels PROPERTIES
   PASS_REGULAR_EXPRESSION "-->   1 <-- Line 1.*-->   5 <-- Line 5"
   LABELS "text-block;enumerate"
)

# Enumeration of an empty file
ADD_TEXT_BLOCK_TEST(textblock_enumerate_empty_file -i "${MOCK_EMPTY_FILE}" -E)
SET_TESTS_PROPERTIES(textblock_enumerate_empty_file PROPERTIES
   FAIL_REGULAR_EXPRESSION "000001|Line"
   LABELS "text-block;enumerate"
)


#############################################################################
#### 4. Tag-Based Extraction (-X / --extract)                            ####
#############################################################################

# Tags-only, exclude-tags (default extraction)
ADD_TEXT_BLOCK_TEST(textblock_extract_tags_only_exclude_short
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>"
)
ADD_TEXT_BLOCK_TEST(textblock_extract_tags_only_exclude_long
   -i "${MOCK_TAGGED_FILE}" --extract --begin-tag "<BEGIN>" --end-tag "<END>" --tags-only --exclude-tags
)
SET_TESTS_PROPERTIES(textblock_extract_tags_only_exclude_short textblock_extract_tags_only_exclude_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 1.*Block line 2"
   FAIL_REGULAR_EXPRESSION "Header line|Footer line|<BEGIN>|<END>"
   LABELS "text-block;extract"
)

# Tags-only, include-tags (-y)
ADD_TEXT_BLOCK_TEST(textblock_extract_tags_only_include
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -y -g
)
SET_TESTS_PROPERTIES(textblock_extract_tags_only_include PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>.*Block line 1.*Block line 2.*<END>"
   FAIL_REGULAR_EXPRESSION "Header line|Footer line"
   LABELS "text-block;extract"
)

# Full-tag-lines, exclude-tags (-f -x)
ADD_TEXT_BLOCK_TEST(textblock_extract_full_lines_exclude
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -f -x
)
SET_TESTS_PROPERTIES(textblock_extract_full_lines_exclude PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 1.*Block line 2"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Header|Footer"
   LABELS "text-block;extract"
)

# Full-tag-lines, include-tags (-f -y)
ADD_TEXT_BLOCK_TEST(textblock_extract_full_lines_include
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_extract_full_lines_include PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>.*Block line 1.*Block line 2.*<END>"
   FAIL_REGULAR_EXPRESSION "Header line|Footer line"
   LABELS "text-block;extract"
)

# Extraction across multiple separate blocks
ADD_TEXT_BLOCK_TEST(textblock_extract_multiple_blocks
   -i "${MOCK_MULTI_TAG_FILE}" -X -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_extract_multiple_blocks PROPERTIES
   PASS_REGULAR_EXPRESSION "Block 1.*Block 2"
   FAIL_REGULAR_EXPRESSION "Header|Middle|Footer"
   LABELS "text-block;extract"
)

# Extraction of multiple inline blocks on the same line
ADD_TEXT_BLOCK_TEST(textblock_extract_inline_multiblock
   -i "${MOCK_MULTI_BLOCK_FILE}" -X -b "[B]" -e "[/B]"
)
SET_TESTS_PROPERTIES(textblock_extract_inline_multiblock PROPERTIES
   PASS_REGULAR_EXPRESSION "FirstSecond"
   FAIL_REGULAR_EXPRESSION "Intro|Middle|Outro|\\[B\\]|\\[/B\\]"
   LABELS "text-block;extract"
)

# Extraction of single combined tag with include-tags (-t -y)
ADD_TEXT_BLOCK_TEST(textblock_extract_single_tag_include
   -i "${MOCK_SINGLE_TAG_FILE}" -X -t "<MARKER>" -y
)
SET_TESTS_PROPERTIES(textblock_extract_single_tag_include PROPERTIES
   PASS_REGULAR_EXPRESSION "<MARKER>"
   FAIL_REGULAR_EXPRESSION "Line A|Line B"
   LABELS "text-block;extract"
)

# Extraction with non-matching tags yields no output
ADD_TEXT_BLOCK_TEST(textblock_extract_no_matches
   -i "${MOCK_TAGGED_FILE}" -X -b "<ABSENT>" -e "</ABSENT>"
)
SET_TESTS_PROPERTIES(textblock_extract_no_matches PROPERTIES
   FAIL_REGULAR_EXPRESSION "Header|Block|Footer"
   LABELS "text-block;extract"
)


#############################################################################
#### 5. Tag-Based Deletion / Removal (-D / --delete / --remove)          ####
#############################################################################

# Delete body but keep tags (-x)
ADD_TEXT_BLOCK_TEST(textblock_delete_keep_tags_short -i "${MOCK_TAGGED_FILE}" -D -b "<BEGIN>" -e "<END>" -x)
ADD_TEXT_BLOCK_TEST(textblock_delete_keep_tags_long  -i "${MOCK_TAGGED_FILE}" --delete -b "<BEGIN>" -e "<END>" --exclude-tags)
ADD_TEXT_BLOCK_TEST(textblock_remove_keep_tags_long  -i "${MOCK_TAGGED_FILE}" --remove -b "<BEGIN>" -e "<END>" --exclude-tags)

SET_TESTS_PROPERTIES(textblock_delete_keep_tags_short textblock_delete_keep_tags_long textblock_remove_keep_tags_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line.*<BEGIN><END>.*Footer line"
   FAIL_REGULAR_EXPRESSION "Block line"
   LABELS "text-block;delete"
)

# Delete body and tags (-y)
ADD_TEXT_BLOCK_TEST(textblock_delete_include_tags
   -i "${MOCK_TAGGED_FILE}" -D -b "<BEGIN>" -e "<END>" -y
)
SET_TESTS_PROPERTIES(textblock_delete_include_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line.*Footer line"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block line"
   LABELS "text-block;delete"
)

# Delete full tag lines completely (-f -y)
ADD_TEXT_BLOCK_TEST(textblock_delete_full_lines
   -i "${MOCK_TAGGED_FILE}" --delete -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_delete_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line\nFooter line"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block line"
   LABELS "text-block;delete"
)

# Delete multiple separate blocks across file
ADD_TEXT_BLOCK_TEST(textblock_delete_multiple_blocks
   -i "${MOCK_MULTI_TAG_FILE}" -D -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_delete_multiple_blocks PROPERTIES
   PASS_REGULAR_EXPRESSION "Header\nMiddle\nFooter"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block 1|Block 2"
   LABELS "text-block;delete"
)

# Delete inline blocks including tags on same line
ADD_TEXT_BLOCK_TEST(textblock_delete_inline_multiblock
   -i "${MOCK_MULTI_BLOCK_FILE}" -D -b "[B]" -e "[/B]" -y
)
SET_TESTS_PROPERTIES(textblock_delete_inline_multiblock PROPERTIES
   PASS_REGULAR_EXPRESSION "Intro  Middle  Outro"
   FAIL_REGULAR_EXPRESSION "First|Second|\\[B\\]|\\[/B\\]"
   LABELS "text-block;delete"
)

# Delete single marker tag (-t -y)
ADD_TEXT_BLOCK_TEST(textblock_delete_single_tag
   -i "${MOCK_SINGLE_TAG_FILE}" -D -t "<MARKER>" -y -f
)
SET_TESTS_PROPERTIES(textblock_delete_single_tag PROPERTIES
   PASS_REGULAR_EXPRESSION "Line A\nLine B"
   FAIL_REGULAR_EXPRESSION "<MARKER>"
   LABELS "text-block;delete"
)


#############################################################################
#### 6. Tag-Based Insertion & Replacement (-F, -B, -R)                   ####
#############################################################################

# Insert Front (-F / --insert-front) tags-only
ADD_TEXT_BLOCK_TEST(textblock_insert_front_short
   -i "${MOCK_TAGGED_FILE}" -F "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)
ADD_TEXT_BLOCK_TEST(textblock_insert_front_long
   -i "${MOCK_TAGGED_FILE}" --insert-front "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_insert_front_short textblock_insert_front_long PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>INSERTED_PAYLOAD.*Block line 1"
   LABELS "text-block;insert"
)

# Insert Front with full tag lines (-f)
ADD_TEXT_BLOCK_TEST(textblock_insert_front_full_lines
   -i "${MOCK_TAGGED_FILE}" -F "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -f
)
SET_TESTS_PROPERTIES(textblock_insert_front_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>\nINSERTED_PAYLOAD\nBlock line 1"
   LABELS "text-block;insert"
)

# Insert Back (-B / --insert-back) tags-only
ADD_TEXT_BLOCK_TEST(textblock_insert_back_short
   -i "${MOCK_TAGGED_FILE}" -B "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)
ADD_TEXT_BLOCK_TEST(textblock_insert_back_long
   -i "${MOCK_TAGGED_FILE}" --insert-back "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>"
)
SET_TESTS_PROPERTIES(textblock_insert_back_short textblock_insert_back_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 2.*INSERTED_PAYLOAD.*<END>"
   LABELS "text-block;insert"
)

# Insert Back with full tag lines (-f)
ADD_TEXT_BLOCK_TEST(textblock_insert_back_full_lines
   -i "${MOCK_TAGGED_FILE}" -B "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -f
)
SET_TESTS_PROPERTIES(textblock_insert_back_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "Block line 2\nINSERTED_PAYLOAD\n<END>"
   LABELS "text-block;insert"
)

# Replace block content keeping tags (-R -x)
ADD_TEXT_BLOCK_TEST(textblock_replace_keep_tags
   -i "${MOCK_TAGGED_FILE}" -R "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -x
)
SET_TESTS_PROPERTIES(textblock_replace_keep_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>INSERTED_PAYLOAD.*<END>"
   FAIL_REGULAR_EXPRESSION "Block line"
   LABELS "text-block;replace"
)
# Replace block content and tags (-R -y)
ADD_TEXT_BLOCK_TEST(textblock_replace_include_tags
   -i "${MOCK_TAGGED_FILE}" --replace "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -y
)
SET_TESTS_PROPERTIES(textblock_replace_include_tags PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line.*INSERTED_PAYLOAD.*Footer line"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block line"
   LABELS "text-block;replace"
)

# Replace full tag lines completely (-R -f -y)
ADD_TEXT_BLOCK_TEST(textblock_replace_full_lines
   -i "${MOCK_TAGGED_FILE}" -R "${MOCK_INSERT_FILE}" -b "<BEGIN>" -e "<END>" -f -y
)
SET_TESTS_PROPERTIES(textblock_replace_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "Header line\nINSERTED_PAYLOAD\nFooter line"
   FAIL_REGULAR_EXPRESSION "<BEGIN>|<END>|Block line"
   LABELS "text-block;replace"
)

# Insert Front at single marker tag position (-t)
ADD_TEXT_BLOCK_TEST(textblock_single_tag_insert_front
   -i "${MOCK_SINGLE_TAG_FILE}" -F "${MOCK_INSERT_FILE}" -t "<MARKER>"
)
SET_TESTS_PROPERTIES(textblock_single_tag_insert_front PROPERTIES
   PASS_REGULAR_EXPRESSION "Line A.*<MARKER>INSERTED_PAYLOAD.*Line B"
   LABELS "text-block;insert"
)

# Insert Back at single marker tag position (-t)
ADD_TEXT_BLOCK_TEST(textblock_single_tag_insert_back
   -i "${MOCK_SINGLE_TAG_FILE}" -B "${MOCK_INSERT_FILE}" -t "<MARKER>"
)
SET_TESTS_PROPERTIES(textblock_single_tag_insert_back PROPERTIES
   PASS_REGULAR_EXPRESSION "Line A.*<MARKER>INSERTED_PAYLOAD.*Line B"
   LABELS "text-block;insert"
)

# Replace single marker tag with file content (-t -y)
ADD_TEXT_BLOCK_TEST(textblock_single_tag_replace_tag
   -i "${MOCK_SINGLE_TAG_FILE}" -R "${MOCK_INSERT_FILE}" -t "<MARKER>" -y
)
SET_TESTS_PROPERTIES(textblock_single_tag_replace_tag PROPERTIES
   PASS_REGULAR_EXPRESSION "Line A.*INSERTED_PAYLOAD.*Line B"
   FAIL_REGULAR_EXPRESSION "<MARKER>"
   LABELS "text-block;replace"
)

# Insert Back on multiple inline blocks on same line
ADD_TEXT_BLOCK_TEST(textblock_insert_back_inline_multiblock
   -i "${MOCK_MULTI_BLOCK_FILE}" -B "${MOCK_INSERT_FILE}" -b "[B]" -e "[/B]"
)
SET_TESTS_PROPERTIES(textblock_insert_back_inline_multiblock PROPERTIES
   PASS_REGULAR_EXPRESSION "FirstINSERTED_PAYLOAD.*\\[/B\\].*SecondINSERTED_PAYLOAD"
   LABELS "text-block;insert"
)


#############################################################################
#### 7. Highlight Mode (-H / --highlight)                                ####
#############################################################################

# Default highlight markers (⭐ for begin, 🛑 for end)
ADD_TEXT_BLOCK_TEST(textblock_highlight_default_short -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>")
ADD_TEXT_BLOCK_TEST(textblock_highlight_default_long  -i "${MOCK_TAGGED_FILE}" --highlight -b "<BEGIN>" -e "<END>")
SET_TESTS_PROPERTIES(textblock_highlight_default_short textblock_highlight_default_long PROPERTIES
   PASS_REGULAR_EXPRESSION "⭐.*Block line 1.*Block line 2.*🛑"
   LABELS "text-block;highlight"
)

# Custom begin/end markers (--highlight-begin, --highlight-end)
ADD_TEXT_BLOCK_TEST(textblock_highlight_custom_begin_end
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-begin "[[BEGIN]]"
   --highlight-end "[[END]]"
)
SET_TESTS_PROPERTIES(textblock_highlight_custom_begin_end PROPERTIES
   PASS_REGULAR_EXPRESSION "\\[\\[BEGIN\\]\\].*Block line 1.*\\[\\[END\\]\\]"
   LABELS "text-block;highlight"
)

# Custom unmarked styling labels (--highlight-unmarked1, --highlight-unmarked2)
ADD_TEXT_BLOCK_TEST(textblock_highlight_custom_unmarked
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-unmarked1 "<U>"
   --highlight-unmarked2 "</U>"
)
SET_TESTS_PROPERTIES(textblock_highlight_custom_unmarked PROPERTIES
   PASS_REGULAR_EXPRESSION "<U>Header line\n</U>.*<U>Footer line\n</U>"
   LABELS "text-block;highlight"
)

# Custom marked styling labels (--highlight-marked1, --highlight-marked2)
ADD_TEXT_BLOCK_TEST(textblock_highlight_custom_marked
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-marked1 "<M>"
   --highlight-marked2 "</M>"
)
SET_TESTS_PROPERTIES(textblock_highlight_custom_marked PROPERTIES
   PASS_REGULAR_EXPRESSION "<M>Block line 1\n</M><M>Block line 2\n</M>"
   LABELS "text-block;highlight"
)

# 6-parameter shortcut (--highlight-param)
ADD_TEXT_BLOCK_TEST(textblock_highlight_param_shortcut
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-param "{START}" "{STOP}" "[u]" "[/u]" "[m]" "[/m]"
)
SET_TESTS_PROPERTIES(textblock_highlight_param_shortcut PROPERTIES
   PASS_REGULAR_EXPRESSION "\\[u\\]Header line.*\\[u\\]<BEGIN>\\[/u\\]{START}\\[m\\].*{STOP}\\[u\\]<END>"
   LABELS "text-block;highlight"
)

# 6-parameter shortcut alias (--highlight-params)
ADD_TEXT_BLOCK_TEST(textblock_highlight_params_shortcut
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>"
   --highlight-params "{START}" "{STOP}" "[u]" "[/u]" "[m]" "[/m]"
)
SET_TESTS_PROPERTIES(textblock_highlight_params_shortcut PROPERTIES
   PASS_REGULAR_EXPRESSION "\\[u\\]Header line.*\\[u\\]<BEGIN>\\[/u\\]{START}\\[m\\].*{STOP}\\[u\\]<END>"
   LABELS "text-block;highlight"
)

# Highlight with single combined tag (-t)
ADD_TEXT_BLOCK_TEST(textblock_highlight_single_tag
   -i "${MOCK_SINGLE_TAG_FILE}" -H -t "<MARKER>"
)
SET_TESTS_PROPERTIES(textblock_highlight_single_tag PROPERTIES
   PASS_REGULAR_EXPRESSION "<MARKER>.*⭐🛑.*Line B"
   LABELS "text-block;highlight"
)

# Highlight with line selection range (-s)
ADD_TEXT_BLOCK_TEST(textblock_highlight_line_selection
   -i "${MOCK_LINES_FILE}" -H -s 2 3
)
SET_TESTS_PROPERTIES(textblock_highlight_line_selection PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*⭐.*Line 2.*Line 3.*🛑.*Line 4"
   LABELS "text-block;highlight"
)

# Highlight with full tag lines (-f)
ADD_TEXT_BLOCK_TEST(textblock_highlight_full_lines
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" -f
)
SET_TESTS_PROPERTIES(textblock_highlight_full_lines PROPERTIES
   PASS_REGULAR_EXPRESSION "<BEGIN>.*⭐.*Block line.*🛑.*<END>"
   LABELS "text-block;highlight"
)


#############################################################################
#### 8. Line Range Selection: Boundaries & Operations                    ####
#############################################################################

# Select entire file (line 1 to 0) with Extract
ADD_TEXT_BLOCK_TEST(textblock_select_entire_file_extract
   -i "${MOCK_LINES_FILE}" -s 1 0 -X
)
SET_TESTS_PROPERTIES(textblock_select_entire_file_extract PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 2.*Line 3.*Line 4.*Line 5"
   LABELS "text-block;select"
)

# Select entire file (line 1 to 0) with Delete (leaves empty output)
ADD_TEXT_BLOCK_TEST(textblock_select_entire_file_delete
   -i "${MOCK_LINES_FILE}" -s 1 0 -D
)
SET_TESTS_PROPERTIES(textblock_select_entire_file_delete PROPERTIES
   FAIL_REGULAR_EXPRESSION "Line [1-5]"
   LABELS "text-block;select"
)

# Select single first line (-s 1 1)
ADD_TEXT_BLOCK_TEST(textblock_select_first_line_only
   -i "${MOCK_LINES_FILE}" -s 1 1 -X
)
SET_TESTS_PROPERTIES(textblock_select_first_line_only PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1"
   FAIL_REGULAR_EXPRESSION "Line 2|Line 3|Line 4|Line 5"
   LABELS "text-block;select"
)

# Select single middle line (-s 3 3)
ADD_TEXT_BLOCK_TEST(textblock_select_middle_line_only
   -i "${MOCK_LINES_FILE}" -s 3 3 -X
)
SET_TESTS_PROPERTIES(textblock_select_middle_line_only PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 3"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2|Line 4|Line 5"
   LABELS "text-block;select"
)

# Select single last line using positive number (-s 5 5)
ADD_TEXT_BLOCK_TEST(textblock_select_last_line_positive
   -i "${MOCK_LINES_FILE}" -s 5 5 -X
)
SET_TESTS_PROPERTIES(textblock_select_last_line_positive PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2|Line 3|Line 4"
   LABELS "text-block;select"
)

# Select middle range (lines 2 to 4) with Extract
ADD_TEXT_BLOCK_TEST(textblock_select_range_extract
   -i "${MOCK_LINES_FILE}" -s 2 4 -X
)
SET_TESTS_PROPERTIES(textblock_select_range_extract PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 2.*Line 3.*Line 4"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 5"
   LABELS "text-block;select"
)

# Select middle range (lines 2 to 4) with Delete
ADD_TEXT_BLOCK_TEST(textblock_select_range_delete
   -i "${MOCK_LINES_FILE}" -s 2 4 -D
)
SET_TESTS_PROPERTIES(textblock_select_range_delete PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nLine 5"
   FAIL_REGULAR_EXPRESSION "Line 2|Line 3|Line 4"
   LABELS "text-block;select"
)

# Select from line 3 to end of file (-s 3 0)
ADD_TEXT_BLOCK_TEST(textblock_select_to_end_extract
   -i "${MOCK_LINES_FILE}" -s 3 0 -X
)
SET_TESTS_PROPERTIES(textblock_select_to_end_extract PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 3.*Line 4.*Line 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2"
   LABELS "text-block;select"
)

# Select negative indexing: last line only (-s -1 -1)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_last_line
   -i "${MOCK_LINES_FILE}" -s -1 -1 -X
)
SET_TESTS_PROPERTIES(textblock_select_neg_last_line PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2|Line 3|Line 4"
   LABELS "text-block;select"
)

# Select negative indexing: last two lines (-s -2 -1)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_last_two
   -i "${MOCK_LINES_FILE}" -s -2 -1 -X
)
SET_TESTS_PROPERTIES(textblock_select_neg_last_two PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 4\nLine 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2|Line 3"
   LABELS "text-block;select"
)

# Select negative indexing: delete last two lines (-s -2 -1 -D)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_delete_last_two
   -i "${MOCK_LINES_FILE}" -s -2 -1 -D
)
SET_TESTS_PROPERTIES(textblock_select_neg_delete_last_two PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1.*Line 2.*Line 3"
   FAIL_REGULAR_EXPRESSION "Line 4|Line 5"
   LABELS "text-block;select"
)

# Select negative start to EOF (-s -3 0)
ADD_TEXT_BLOCK_TEST(textblock_select_neg_start_to_eof
   -i "${MOCK_LINES_FILE}" -s -3 0 -X
)
SET_TESTS_PROPERTIES(textblock_select_neg_start_to_eof PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 3.*Line 4.*Line 5"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 2"
   LABELS "text-block;select"
)

# Select mixed positive start and negative end (-s 2 -2: lines 2, 3, 4)
ADD_TEXT_BLOCK_TEST(textblock_select_mixed_pos_neg
   -i "${MOCK_LINES_FILE}" -s 2 -2 -X
)
SET_TESTS_PROPERTIES(textblock_select_mixed_pos_neg PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 2.*Line 3.*Line 4"
   FAIL_REGULAR_EXPRESSION "Line 1|Line 5"
   LABELS "text-block;select"
)

# Line selection range with Insert Front (-s with -F)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_front
   -i "${MOCK_LINES_FILE}" -s 2 3 -F "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_front PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nINSERTED_PAYLOAD\nLine 2\nLine 3\nLine 4"
   LABELS "text-block;select"
)

# Line selection range with Insert Back (-s with -B)
ADD_TEXT_BLOCK_TEST(textblock_select_insert_back
   -i "${MOCK_LINES_FILE}" -s 2 3 -B "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_insert_back PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nLine 2\nLine 3\nINSERTED_PAYLOAD\nLine 4"
   LABELS "text-block;select"
)

# Line selection range with Replace (-s with -R)
ADD_TEXT_BLOCK_TEST(textblock_select_replace
   -i "${MOCK_LINES_FILE}" -s 2 4 -R "${MOCK_INSERT_FILE}"
)
SET_TESTS_PROPERTIES(textblock_select_replace PROPERTIES
   PASS_REGULAR_EXPRESSION "Line 1\nINSERTED_PAYLOAD\nLine 5"
   FAIL_REGULAR_EXPRESSION "Line 2|Line 3|Line 4"
   LABELS "text-block;select"
)


#############################################################################
#### 9. Action Limits (-m / --min-actions, -M / --max-actions)           ####
#############################################################################

# Exactly 1 action expected and executed
ADD_TEXT_BLOCK_TEST(textblock_actions_exact_1
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -m 1 -M 1
)

# Min actions 1 satisfied
ADD_TEXT_BLOCK_TEST(textblock_actions_min_1_pass
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -m 1
)

# Min actions 2 fails when only 1 action exists
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_min_2_fail
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" --min-actions 2
)

# Max actions 1 satisfied
ADD_TEXT_BLOCK_TEST(textblock_actions_max_1_pass
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" -M 1
)

# Max actions 0 fails when 1 action exists
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_max_0_fail
   -i "${MOCK_TAGGED_FILE}" -X -b "<BEGIN>" -e "<END>" --max-actions 0
)

# Exactly 2 actions on separate blocks across file
ADD_TEXT_BLOCK_TEST(textblock_actions_multi_exact_2
   -i "${MOCK_MULTI_TAG_FILE}" -X -b "<BEGIN>" -e "<END>" -m 2 -M 2
)

# Under min actions for multiple blocks (file has 2, requires 3)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_multi_under_min
   -i "${MOCK_MULTI_TAG_FILE}" -X -b "<BEGIN>" -e "<END>" -m 3
)

# Over max actions for multiple blocks (file has 2, max allowed is 1)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_multi_over_max
   -i "${MOCK_MULTI_TAG_FILE}" -X -b "<BEGIN>" -e "<END>" -M 1
)

# Inline multiblock counting: line has 2 blocks
ADD_TEXT_BLOCK_TEST(textblock_actions_inline_exact_2
   -i "${MOCK_MULTI_BLOCK_FILE}" -X -b "[B]" -e "[/B]" -m 2 -M 2
)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_inline_under_min
   -i "${MOCK_MULTI_BLOCK_FILE}" -X -b "[B]" -e "[/B]" -m 3
)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_inline_over_max
   -i "${MOCK_MULTI_BLOCK_FILE}" -X -b "[B]" -e "[/B]" -M 1
)

# Zero matches pass when max-actions is 0
ADD_TEXT_BLOCK_TEST(textblock_actions_zero_matches_pass_max_0
   -i "${MOCK_LINES_FILE}" -X -b "<ABSENT>" -e "</ABSENT>" -M 0
)

# Zero matches fail when min-actions is 1
ADD_TEXT_BLOCK_FAIL_TEST(textblock_actions_zero_matches_fail_min_1
   -i "${MOCK_LINES_FILE}" -X -b "<ABSENT>" -e "</ABSENT>" -m 1
)

# Min actions 0 always succeeds on zero matches
ADD_TEXT_BLOCK_TEST(textblock_actions_min_0_always_pass
   -i "${MOCK_LINES_FILE}" -X -b "<ABSENT>" -e "</ABSENT>" -m 0
)


#############################################################################
#### 10. Warnings and Suppression (-q / --suppress-warnings)             ####
#############################################################################

# Warning: tags passed in Cat mode (-C)
ADD_TEXT_BLOCK_TEST(textblock_warning_tags_cat
   -i "${MOCK_LINES_FILE}" -C -b "<TAG>"
)
SET_TESTS_PROPERTIES(textblock_warning_tags_cat PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Begin/end tags.*have no effect for Cat"
   LABELS "text-block;warning"
)

# Warning: tags passed in Discard mode (-0)
ADD_TEXT_BLOCK_TEST(textblock_warning_tags_discard
   -i "${MOCK_LINES_FILE}" -0 -b "<TAG>"
)
SET_TESTS_PROPERTIES(textblock_warning_tags_discard PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Begin/end tags.*have no effect for Cat, Discard, or Enumerate"
   LABELS "text-block;warning"
)

# Warning: tags passed in Enumerate mode (-E)
ADD_TEXT_BLOCK_TEST(textblock_warning_tags_enumerate
   -i "${MOCK_LINES_FILE}" -E -b "<TAG>"
)
SET_TESTS_PROPERTIES(textblock_warning_tags_enumerate PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Begin/end tags.*have no effect for Cat, Discard, or Enumerate"
   LABELS "text-block;warning"
)

# Warning: identical tags with excluded tags in Extract mode (-X -t -x)
ADD_TEXT_BLOCK_TEST(textblock_warning_identical_tags_extract
   -i "${MOCK_LINES_FILE}" -X -t "<TAG>" -x
)
SET_TESTS_PROPERTIES(textblock_warning_identical_tags_extract PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Identical begin/end tags.*not useful for Extract, Remove, or Replace"
   LABELS "text-block;warning"
)

# Warning: identical tags with excluded tags in Delete mode (-D -t -x)
ADD_TEXT_BLOCK_TEST(textblock_warning_identical_tags_delete
   -i "${MOCK_LINES_FILE}" -D -t "<TAG>" -x
)
SET_TESTS_PROPERTIES(textblock_warning_identical_tags_delete PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Identical begin/end tags.*not useful for Extract, Remove, or Replace"
   LABELS "text-block;warning"
)

# Warning: identical tags with excluded tags in Replace mode (-R -t -x)
ADD_TEXT_BLOCK_TEST(textblock_warning_identical_tags_replace
   -i "${MOCK_LINES_FILE}" -R "${MOCK_INSERT_FILE}" -t "<TAG>" -x
)
SET_TESTS_PROPERTIES(textblock_warning_identical_tags_replace PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Identical begin/end tags.*not useful for Extract, Remove, or Replace"
   LABELS "text-block;warning"
)

# Warning: inverted line selection range (start > end, e.g. 5 2)
ADD_TEXT_BLOCK_TEST(textblock_warning_inverted_selection
   -i "${MOCK_LINES_FILE}" -s 5 2 -X
)
SET_TESTS_PROPERTIES(textblock_warning_inverted_selection PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Line selection range.*not useful"
   LABELS "text-block;warning"
)

# Warning suppression (-q): tags in Cat mode
ADD_TEXT_BLOCK_TEST(textblock_suppress_warning_cat
   -i "${MOCK_LINES_FILE}" -C -b "<TAG>" -q
)
SET_TESTS_PROPERTIES(textblock_suppress_warning_cat PROPERTIES
   FAIL_REGULAR_EXPRESSION "WARNING:"
   LABELS "text-block;warning"
)

# Warning suppression (--suppress-warnings): inverted selection
ADD_TEXT_BLOCK_TEST(textblock_suppress_warning_inverted_select
   -i "${MOCK_LINES_FILE}" -s 5 2 -X --suppress-warnings
)
SET_TESTS_PROPERTIES(textblock_suppress_warning_inverted_select PROPERTIES
   FAIL_REGULAR_EXPRESSION "WARNING:"
   LABELS "text-block;warning"
)


#############################################################################
#### 11. File I/O Operations (-o, -a, -p, -k)                            ####
#############################################################################

SET(TEST_OUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/out_redirect.txt")

# Output redirection (-o / --output)
ADD_TEXT_BLOCK_TEST(textblock_output_file_short
   -i "${MOCK_LINES_FILE}" -o "${TEST_OUT_FILE}" -C
)
ADD_TEXT_BLOCK_TEST(textblock_output_file_long
   -i "${MOCK_LINES_FILE}" --output "${TEST_OUT_FILE}" -C
)

# Append mode (-a / --append)
ADD_TEXT_BLOCK_TEST(textblock_append_file_short
   -i "${MOCK_LINES_FILE}" -o "${TEST_OUT_FILE}" -a -C
)
ADD_TEXT_BLOCK_TEST(textblock_append_file_long
   -i "${MOCK_LINES_FILE}" -o "${TEST_OUT_FILE}" --append -C
)

# In-place file update (-p / --in-place)
SET(TEST_INPLACE_FILE "${CMAKE_CURRENT_BINARY_DIR}/inplace_target.txt")
FILE(WRITE "${TEST_INPLACE_FILE}" "Alpha\nBeta\nGamma\n")

ADD_TEXT_BLOCK_TEST(textblock_inplace_short
   -i "${TEST_INPLACE_FILE}" -p -s 2 2 -D
)
ADD_TEXT_BLOCK_TEST(textblock_inplace_long
   -i "${TEST_INPLACE_FILE}" --in-place -s 1 1 -D
)

# Keep broken output on error (-k / --keep-broken)
SET(TEST_BROKEN_KEPT "${CMAKE_CURRENT_BINARY_DIR}/broken_kept.txt")
ADD_TEXT_BLOCK_FAIL_TEST(textblock_keep_broken_output
   -i "${MOCK_TAGGED_FILE}" -o "${TEST_BROKEN_KEPT}" -X -b "<BEGIN>" -e "<END>" -m 2 -k
)


#############################################################################
#### 12. CLI Error Handling & Negative Boundaries                        ####
#############################################################################

# Inverted min/max actions (-m > -M)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_min_greater_than_max -m 5 -M 2)

# Line selection and begin-tag are mutually exclusive
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_select_and_begin_tag -i "${MOCK_LINES_FILE}" -s 1 3 -b "<BEGIN>")

# Line selection and end-tag are mutually exclusive
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_select_and_end_tag -i "${MOCK_LINES_FILE}" -s 1 3 -e "<END>")

# Line selection and single tag are mutually exclusive
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_select_and_single_tag -i "${MOCK_LINES_FILE}" -s 1 3 -t "<MARKER>")

# Line selection missing the to_line argument
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_select_missing_to_line -i "${MOCK_LINES_FILE}" -s 2)

# Line selection missing both arguments
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_select_missing_both_args -i "${MOCK_LINES_FILE}" -s)

# Negative line numbers without an input file (stdin cannot rewind/line-count)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_negative_lines_without_input_file
   -s -2 -1 -D
)

# In-place update and output file redirection are mutually exclusive
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_inplace_and_output
   -i "${MOCK_LINES_FILE}" -p -o out.txt
)

# In-place update without an input file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_inplace_without_input -p)

# Missing insert file argument for insert modes
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_insert_front_no_file_arg -F)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_insert_back_no_file_arg  -B)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_replace_no_file_arg      -R)

# Non-existent input file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_nonexistent_input
   -i "/nonexistent/path/file.txt"
)

# Non-existent insert file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_nonexistent_insert_file
   -i "${MOCK_TAGGED_FILE}" -F "/nonexistent/insert_file.txt" -b "<BEGIN>" -e "<END>"
)

# Insert from stdin ("-") without an input file
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_insert_stdin_without_input_file
   -F "-" -b "<BEGIN>" -e "<END>"
)

# Invalid enumeration format: % character disallowed
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_invalid_enum_percent
   -E --enumerate-format "%d"
)

# Invalid enumeration format: special punctuation disallowed
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_invalid_enum_punctuation
   -E --enumerate-format "4!"
)

# Missing arguments for --highlight-param (requires 6 strings)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_highlight_param_0_args
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" --highlight-param
)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_highlight_param_1_arg
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" --highlight-param "A"
)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_highlight_param_3_args
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" --highlight-param "A" "B" "C"
)
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_highlight_param_5_args
   -i "${MOCK_TAGGED_FILE}" -H -b "<BEGIN>" -e "<END>" --highlight-param "A" "B" "C" "D" "E"
)

# Extraneous unrecognized positional parameters after options
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_extraneous_positional_args
   -i "${MOCK_LINES_FILE}" -C extra_positional_parameter
)

# Unknown flag
ADD_TEXT_BLOCK_FAIL_TEST(textblock_fail_unknown_flag --invalid-flag)
