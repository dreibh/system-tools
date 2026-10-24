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
# Print-UTF8 Test Suite
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

# ASCII control codes for unescape tests
STRING(ASCII 7  CHAR_BEL)
STRING(ASCII 8  CHAR_BS)
STRING(ASCII 11 CHAR_VT)
STRING(ASCII 12 CHAR_FF)

# ===========================================================================
# Deterministic Mock Input & Stdin Runner Scripts for Multiline Tests
# ===========================================================================
SET(MOCK_MULTILINE_INPUT "${CMAKE_CURRENT_BINARY_DIR}/mock_multiline_input.txt")
FILE(WRITE "${MOCK_MULTILINE_INPUT}"
"Alpha\n"
"Beta\n"
"Gamma\n"
)

# Dedicated runner script for multiline indent
SET(RUN_MULTILINE_INDENT_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/run_multiline_indent.cmake")
FILE(WRITE "${RUN_MULTILINE_INDENT_SCRIPT}"
"execute_process(\n"
"   COMMAND \"\${PROG}\" -n -I -16 \"[ \" \" ]\"\n"
"   INPUT_FILE \"\${INPUT_FILE}\"\n"
"   RESULT_VARIABLE RES\n"
")\n"
"if(NOT RES EQUAL 0)\n"
"   message(FATAL_ERROR \"Process exited with code \${RES}\")\n"
"endif()\n"
)

# Dedicated runner script for multiline center
SET(RUN_MULTILINE_CENTER_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/run_multiline_center.cmake")
FILE(WRITE "${RUN_MULTILINE_CENTER_SCRIPT}"
"execute_process(\n"
"   COMMAND \"\${PROG}\" -n -x 24 -C \"[ \" \" ]\"\n"
"   INPUT_FILE \"\${INPUT_FILE}\"\n"
"   RESULT_VARIABLE RES\n"
")\n"
"if(NOT RES EQUAL 0)\n"
"   message(FATAL_ERROR \"Process exited with code \${RES}\")\n"
"endif()\n"
)


#############################################################################
#### Helper Functions                                                    ####
#############################################################################

# ###### Helper for deterministic commands (expected return code: 0) ########
FUNCTION(ADD_PRINT_UTF8_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:print-utf8> ${ARGN}
   )
   SET(TEST_LABELS "print-utf8;cli")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()

# ###### Helper for expected failures (expected return code: non-zero) ######
FUNCTION(ADD_PRINT_UTF8_FAIL_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:print-utf8> ${ARGN}
   )
   SET(TEST_LABELS "print-utf8;negative;error")
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

ADD_PRINT_UTF8_TEST(printutf8_cli_version_short -v)
ADD_PRINT_UTF8_TEST(printutf8_cli_version_long  --version)
ADD_PRINT_UTF8_TEST(printutf8_cli_help_short    -h)
ADD_PRINT_UTF8_TEST(printutf8_cli_help_long     --help)

SET_TESTS_PROPERTIES(printutf8_cli_version_short printutf8_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "print-utf8 [0-9]+\\.[0-9]+"
)

SET_TESTS_PROPERTIES(printutf8_cli_help_short printutf8_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# === CLI Help Internationalization =========================================
ADD_PRINT_UTF8_TEST(printutf8_cli_help_i18n_de -h)
ADD_PRINT_UTF8_TEST(printutf8_cli_help_i18n_nb -h)

SET_TESTS_PROPERTIES(printutf8_cli_help_i18n_de PROPERTIES
   ENVIRONMENT "LC_ALL=en_US.UTF-8;LANGUAGE=de;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Aufruf:"
   LABELS "print-utf8;cli;i18n;locale_de"
)

SET_TESTS_PROPERTIES(printutf8_cli_help_i18n_nb PROPERTIES
   ENVIRONMENT "LC_ALL=en_US.UTF-8;LANGUAGE=nb;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Bruk:"
   LABELS "print-utf8;cli;i18n;locale_nb"
)


#############################################################################
#### Terminal Dimensions (-t / --terminal-info)                          ####
#############################################################################

ADD_PRINT_UTF8_TEST(printutf8_terminal_info_default -t)
SET_TESTS_PROPERTIES(printutf8_terminal_info_default PROPERTIES
   PASS_REGULAR_EXPRESSION "[0-9]+ [0-9]+"
   LABELS "print-utf8;terminal"
)

ADD_PRINT_UTF8_TEST(printutf8_terminal_info_env --terminal-info)
SET_TESTS_PROPERTIES(printutf8_terminal_info_env PROPERTIES
   PASS_REGULAR_EXPRESSION "[0-9]+ [0-9]+"
   LABELS "print-utf8;terminal"
)


#############################################################################
#### String Metrics: Byte Size (-b / --size)                             ####
#############################################################################

ADD_PRINT_UTF8_TEST(printutf8_size_ascii     -b "Hello")
ADD_PRINT_UTF8_TEST(printutf8_size_multibyte -b "äöü")
ADD_PRINT_UTF8_TEST(printutf8_size_emoji     -b "😀")
ADD_PRINT_UTF8_TEST(printutf8_size_ansi      --size "\\e[31mHello\\e[0m")

SET_TESTS_PROPERTIES(printutf8_size_ascii PROPERTIES
   PASS_REGULAR_EXPRESSION "5"
)
SET_TESTS_PROPERTIES(printutf8_size_multibyte PROPERTIES
   PASS_REGULAR_EXPRESSION "6"
)
SET_TESTS_PROPERTIES(printutf8_size_emoji PROPERTIES
   PASS_REGULAR_EXPRESSION "4"
)
SET_TESTS_PROPERTIES(printutf8_size_ansi PROPERTIES
   PASS_REGULAR_EXPRESSION "5"
)


#############################################################################
#### String Metrics: Codepoint Length (-l / --length)                    ####
#############################################################################

ADD_PRINT_UTF8_TEST(printutf8_length_ascii     -l "Hello")
ADD_PRINT_UTF8_TEST(printutf8_length_multibyte -l "äöü")
ADD_PRINT_UTF8_TEST(printutf8_length_emoji     -l "😀")
ADD_PRINT_UTF8_TEST(printutf8_length_ansi      --length "\\e[31mHello\\e[0m")

SET_TESTS_PROPERTIES(printutf8_length_ascii PROPERTIES
   PASS_REGULAR_EXPRESSION "5"
)
SET_TESTS_PROPERTIES(printutf8_length_multibyte PROPERTIES
   PASS_REGULAR_EXPRESSION "3"
)
SET_TESTS_PROPERTIES(printutf8_length_emoji PROPERTIES
   PASS_REGULAR_EXPRESSION "1"
)
SET_TESTS_PROPERTIES(printutf8_length_ansi PROPERTIES
   PASS_REGULAR_EXPRESSION "5"
)


#############################################################################
#### String Metrics: Visual Column Width (-w / --width)                  ####
#############################################################################

ADD_PRINT_UTF8_TEST(printutf8_width_ascii     -w "Hello")
ADD_PRINT_UTF8_TEST(printutf8_width_multibyte -w "äöü")
ADD_PRINT_UTF8_TEST(printutf8_width_emoji     -w "😀")
ADD_PRINT_UTF8_TEST(printutf8_width_cjk       -w "日本語")
ADD_PRINT_UTF8_TEST(printutf8_width_combining -w "e\\u0301")
ADD_PRINT_UTF8_TEST(printutf8_width_flag      -w "\\U0001F1F3\\U0001F1F4")
ADD_PRINT_UTF8_TEST(printutf8_width_ansi      --width "\\e[31mHello\\e[0m")

SET_TESTS_PROPERTIES(printutf8_width_ascii PROPERTIES
   PASS_REGULAR_EXPRESSION "5"
)
SET_TESTS_PROPERTIES(printutf8_width_multibyte PROPERTIES
   PASS_REGULAR_EXPRESSION "3"
)
SET_TESTS_PROPERTIES(printutf8_width_emoji PROPERTIES
   PASS_REGULAR_EXPRESSION "2"
)
SET_TESTS_PROPERTIES(printutf8_width_cjk PROPERTIES
   PASS_REGULAR_EXPRESSION "6"
)
SET_TESTS_PROPERTIES(printutf8_width_combining PROPERTIES
   PASS_REGULAR_EXPRESSION "1"
)
SET_TESTS_PROPERTIES(printutf8_width_flag PROPERTIES
   PASS_REGULAR_EXPRESSION "2"
)
SET_TESTS_PROPERTIES(printutf8_width_ansi PROPERTIES
   PASS_REGULAR_EXPRESSION "5"
)


#############################################################################
#### String Metrics: Combined (-a / --size-length-width)                 ####
#############################################################################

ADD_PRINT_UTF8_TEST(printutf8_all_metrics_ascii     -a "Hello")
ADD_PRINT_UTF8_TEST(printutf8_all_metrics_multibyte -a "äöü")
ADD_PRINT_UTF8_TEST(printutf8_all_metrics_emoji     -a "😀")
ADD_PRINT_UTF8_TEST(printutf8_all_metrics_cjk       -a "日本語")
ADD_PRINT_UTF8_TEST(printutf8_all_metrics_ansi      --size-length-width "\\e[1;32mTest\\e[0m")

SET_TESTS_PROPERTIES(printutf8_all_metrics_ascii PROPERTIES
   PASS_REGULAR_EXPRESSION "5 5 5"
)
SET_TESTS_PROPERTIES(printutf8_all_metrics_multibyte PROPERTIES
   PASS_REGULAR_EXPRESSION "6 3 3"
)
SET_TESTS_PROPERTIES(printutf8_all_metrics_emoji PROPERTIES
   PASS_REGULAR_EXPRESSION "4 1 2"
)
SET_TESTS_PROPERTIES(printutf8_all_metrics_cjk PROPERTIES
   PASS_REGULAR_EXPRESSION "9 3 6"
)
SET_TESTS_PROPERTIES(printutf8_all_metrics_ansi PROPERTIES
   PASS_REGULAR_EXPRESSION "4 4 4"
)


#############################################################################
#### Advanced Unicode Character Classes & Combining Sequences            ####
#############################################################################

# Nordic extended Latin characters (æ, ø, å): 2 bytes each, visual width 1
ADD_PRINT_UTF8_TEST(printutf8_metrics_nordic_lower -a "æøå")
ADD_PRINT_UTF8_TEST(printutf8_metrics_nordic_upper -a "ÆØÅ")
SET_TESTS_PROPERTIES(printutf8_metrics_nordic_lower printutf8_metrics_nordic_upper PROPERTIES
   PASS_REGULAR_EXPRESSION "6 3 3"
   LABELS "print-utf8;unicode;nordic"
)

# Decomposed Unicode (NFD): 'a' followed by combining diaeresis U+0308 (ä)
# Byte size: 1 ('a') + 2 (U+0308) = 3 bytes; Codepoints: 2; Visual width: 1
ADD_PRINT_UTF8_TEST(printutf8_combining_diaeresis -a "a\\u0308")
SET_TESTS_PROPERTIES(printutf8_combining_diaeresis PROPERTIES
   PASS_REGULAR_EXPRESSION "3 2 1"
   LABELS "print-utf8;unicode;combining"
)

# Zero-Width Non-Joiner (ZWNJ U+200C) and Zero-Width Joiner (ZWJ U+200D)
# 3 bytes each in UTF-8, 1 codepoint, visual width 0
ADD_PRINT_UTF8_TEST(printutf8_zero_width_zwnj -a "\\u200c")
ADD_PRINT_UTF8_TEST(printutf8_zero_width_zwj  -a "\\u200d")
SET_TESTS_PROPERTIES(printutf8_zero_width_zwnj printutf8_zero_width_zwj PROPERTIES
   PASS_REGULAR_EXPRESSION "3 1 0"
   LABELS "print-utf8;unicode;zerowidth"
)

# Fullwidth Latin Character U+FF21 ('Ａ'): East Asian Wide (visual width 2)
# UTF-8 byte size: 3; Codepoint: 1; Visual width: 2
ADD_PRINT_UTF8_TEST(printutf8_fullwidth_latin -a "\\uff21")
SET_TESTS_PROPERTIES(printutf8_fullwidth_latin PROPERTIES
   PASS_REGULAR_EXPRESSION "3 1 2"
   LABELS "print-utf8;unicode;cjk"
)

# Multi-codepoint flag emoji sequence (Germany DE: U+1F1E9 + U+1F1EA)
# Visual width for paired regional indicators collapses to 2
ADD_PRINT_UTF8_TEST(printutf8_flag_germany -a "\\U0001f1e9\\U0001f1ea")
SET_TESTS_PROPERTIES(printutf8_flag_germany PROPERTIES
   PASS_REGULAR_EXPRESSION "8 2 2"
   LABELS "print-utf8;unicode;emoji"
)

# Complex 4-byte astral plane emoji: Rocket U+1F680 (🚀)
ADD_PRINT_UTF8_TEST(printutf8_emoji_rocket -a "\\U0001f680")
SET_TESTS_PROPERTIES(printutf8_emoji_rocket PROPERTIES
   PASS_REGULAR_EXPRESSION "4 1 2"
   LABELS "print-utf8;unicode;emoji"
)


#############################################################################
#### ANSI Escape Code Stripping Metrics                                  ####
#############################################################################

# 24-bit TrueColor / RGB ANSI escape: \e[38;2;241;71;29m
# Stripped before computing metrics; raw string "RGB" has size 3, len 3, width 3
ADD_PRINT_UTF8_TEST(printutf8_ansi_truecolor -a "\\e[38;2;241;71;29mRGB\\e[0m")
SET_TESTS_PROPERTIES(printutf8_ansi_truecolor PROPERTIES
   PASS_REGULAR_EXPRESSION "3 3 3"
   LABELS "print-utf8;ansi;color"
)

# Composite ANSI text attributes: Bold, Underline, Inverted (\e[1;4;7m)
ADD_PRINT_UTF8_TEST(printutf8_ansi_attributes -a "\\e[1;4;7mFormatted\\e[0m")
SET_TESTS_PROPERTIES(printutf8_ansi_attributes PROPERTIES
   PASS_REGULAR_EXPRESSION "9 9 9"
   LABELS "print-utf8;ansi;style"
)

# Embedded ANSI color codes inside wide CJK characters
ADD_PRINT_UTF8_TEST(printutf8_ansi_embedded_cjk -a "\\e[32m日\\e[33m本\\e[34m語\\e[0m")
SET_TESTS_PROPERTIES(printutf8_ansi_embedded_cjk PROPERTIES
   PASS_REGULAR_EXPRESSION "9 3 6"
   LABELS "print-utf8;ansi;cjk"
)


#############################################################################
#### Extended Escape Sequence Parsing                                    ####
#############################################################################

ADD_PRINT_UTF8_TEST(printutf8_unescape_hex     -n "\\x48\\x65\\x6c\\x6c\\x6f")
ADD_PRINT_UTF8_TEST(printutf8_unescape_octal   -n "\\0110\\0145\\0154\\0154\\0157")
ADD_PRINT_UTF8_TEST(printutf8_unescape_unicode -n "\\u0048\\u0069")

SET_TESTS_PROPERTIES(printutf8_unescape_hex printutf8_unescape_octal PROPERTIES
   PASS_REGULAR_EXPRESSION "Hello"
)
SET_TESTS_PROPERTIES(printutf8_unescape_unicode PROPERTIES
   PASS_REGULAR_EXPRESSION "Hi"
)

# Escaped backslash: "\\" -> "\"
ADD_PRINT_UTF8_TEST(printutf8_unescape_backslash -n "\\\\")
SET_TESTS_PROPERTIES(printutf8_unescape_backslash PROPERTIES
   PASS_REGULAR_EXPRESSION "\\\\"
   LABELS "print-utf8;unescape"
)

# Control escapes: alert (\a), backspace (\b), form feed (\f), tab (\t), vertical tab (\v)
ADD_PRINT_UTF8_TEST(printutf8_unescape_controls -n "A\\aB\\bC\\fD\\tE\\vF")
SET_TESTS_PROPERTIES(printutf8_unescape_controls PROPERTIES
   PASS_REGULAR_EXPRESSION "A${CHAR_BEL}B${CHAR_BS}C${CHAR_FF}D\tE${CHAR_VT}F"
   LABELS "print-utf8;unescape"
)

# Octal sequence with ASCII capital 'A' (\0101) and 'B' (\0102)
ADD_PRINT_UTF8_TEST(printutf8_unescape_octal_chars -n "\\0101\\0102")
SET_TESTS_PROPERTIES(printutf8_unescape_octal_chars PROPERTIES
   PASS_REGULAR_EXPRESSION "AB"
   LABELS "print-utf8;unescape"
)


#############################################################################
#### Indentation Formatting (-i / --indent)                              ####
#############################################################################

# Right-justified (+10): 6 spaces + 4 chars "Test"
ADD_PRINT_UTF8_TEST(printutf8_indent_positive -n -i 10 "Test")
SET_TESTS_PROPERTIES(printutf8_indent_positive PROPERTIES
   PASS_REGULAR_EXPRESSION "      Test"
)

# Left-justified (-10): 4 chars "Test" + 6 spaces
ADD_PRINT_UTF8_TEST(printutf8_indent_negative -n --indent -10 "Test")
SET_TESTS_PROPERTIES(printutf8_indent_negative PROPERTIES
   PASS_REGULAR_EXPRESSION "Test      "
)

# Wide character right-justified (+10): 8 spaces + 2 cols "😀"
ADD_PRINT_UTF8_TEST(printutf8_indent_emoji -n -i 10 "😀")
SET_TESTS_PROPERTIES(printutf8_indent_emoji PROPERTIES
   PASS_REGULAR_EXPRESSION "        😀"
)


#############################################################################
#### Centering (-c / --center)                                           ####
#############################################################################

# Console width 20, "Test" (width 4): (20 - 4)/2 = 8 spaces
ADD_PRINT_UTF8_TEST(printutf8_center_ascii -n -x 20 -c "Test")
SET_TESTS_PROPERTIES(printutf8_center_ascii PROPERTIES
   PASS_REGULAR_EXPRESSION "        Test"
)

# Console width 20, "😀" (width 2): (20 - 2)/2 = 9 spaces
ADD_PRINT_UTF8_TEST(printutf8_center_emoji -n --columns 20 --center "😀")
SET_TESTS_PROPERTIES(printutf8_center_emoji PROPERTIES
   PASS_REGULAR_EXPRESSION "         😀"
)

# Centering with negative column offset: 80 - 40 = 40 columns
# Text "Centered" (width 8): (40 - 8)/2 = 16 spaces indentation
ADD_PRINT_UTF8_TEST(printutf8_center_negative_columns -n -x -40 -c "Centered")
SET_TESTS_PROPERTIES(printutf8_center_negative_columns PROPERTIES
   PASS_REGULAR_EXPRESSION "                Centered"
   LABELS "print-utf8;columns;center"
)


#############################################################################
#### Separator Formatting (-s / --separator)                             ####
#############################################################################

# Width 10: "<" + 8 "=" + ">"
ADD_PRINT_UTF8_TEST(printutf8_separator_ascii -n -x 10 -s "<" "=" ">")
SET_TESTS_PROPERTIES(printutf8_separator_ascii PROPERTIES
   PASS_REGULAR_EXPRESSION "<========>"
)

# Width 10: "╔" + 8 "═" + "╗"
ADD_PRINT_UTF8_TEST(printutf8_separator_box -n -x 10 -s "╔" "═" "╗")
SET_TESTS_PROPERTIES(printutf8_separator_box PROPERTIES
   PASS_REGULAR_EXPRESSION "╔════════╗"
)

# Width 12: "<*" + 8 "=" + "*>"
ADD_PRINT_UTF8_TEST(printutf8_separator_custom -n -x 12 --separator "<*" "=" "*>")
SET_TESTS_PROPERTIES(printutf8_separator_custom PROPERTIES
   PASS_REGULAR_EXPRESSION "<\\*========\\*>"
)

# Fallback: Zero-width separator string falls back to space (" ")
ADD_PRINT_UTF8_TEST(printutf8_separator_zerowidth_fallback -n -x 10 -s "<" "\\u200b" ">")
SET_TESTS_PROPERTIES(printutf8_separator_zerowidth_fallback PROPERTIES
   PASS_REGULAR_EXPRESSION "<        >"
)

# Negative columns subtract from default width (variable terminal width supported)
ADD_PRINT_UTF8_TEST(printutf8_separator_negative_columns -n -x -20 -s "<" "=" ">")
SET_TESTS_PROPERTIES(printutf8_separator_negative_columns PROPERTIES
   PASS_REGULAR_EXPRESSION "<===+>"
   LABELS "print-utf8;columns;separator"
)

# Multi-character separator pattern: left "<|", pattern "=-", right "|>"
# Width 20: 2 cols left + (8 repetitions of "=-" = 16 cols) + 2 cols right = 20
ADD_PRINT_UTF8_TEST(printutf8_separator_multichar_pattern -n -x 20 -s "<|" "=-" "|>")
SET_TESTS_PROPERTIES(printutf8_separator_multichar_pattern PROPERTIES
   PASS_REGULAR_EXPRESSION "<|=-=-=-=-=-=-=-=-|>"
   LABELS "print-utf8;separator"
)

# Wide emoji repeating separator
ADD_PRINT_UTF8_TEST(printutf8_separator_emoji_pattern -n -x 12 -s "<" "🌟" ">")
SET_TESTS_PROPERTIES(printutf8_separator_emoji_pattern PROPERTIES
   PASS_REGULAR_EXPRESSION "<🌟🌟🌟🌟🌟>"
   LABELS "print-utf8;separator;emoji"
)

# Colored ANSI borders in separator
# Direct ADD_TEST avoids CMake FUNCTION argument list serialization of escaped sequences
ADD_TEST(
   NAME printutf8_separator_ansi_borders
   COMMAND $<TARGET_FILE:print-utf8> -n -x 10 --separator "\\e[32m<\\e[0m" "=" "\\e[32m>\\e[0m"
)
SET_TESTS_PROPERTIES(printutf8_separator_ansi_borders PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "========"
   FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
   LABELS "print-utf8;separator;ansi"
)


#############################################################################
#### Multiline Processing (-I / -C)                                      ####
#############################################################################

ADD_TEST(
   NAME printutf8_multiline_indent
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:print-utf8>"
      -D "INPUT_FILE=${MOCK_MULTILINE_INPUT}"
      -P "${RUN_MULTILINE_INDENT_SCRIPT}"
)
SET_TESTS_PROPERTIES(printutf8_multiline_indent PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "\\[ Alpha[ ]+\\]"
   FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
   LABELS "print-utf8;multiline"
)

ADD_TEST(
   NAME printutf8_multiline_center
   COMMAND ${CMAKE_COMMAND}
      -D "PROG=$<TARGET_FILE:print-utf8>"
      -D "INPUT_FILE=${MOCK_MULTILINE_INPUT}"
      -P "${RUN_MULTILINE_CENTER_SCRIPT}"
)
SET_TESTS_PROPERTIES(printutf8_multiline_center PROPERTIES
   ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "\\[[ ]+Alpha[ ]+\\]"
   FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
   LABELS "print-utf8;multiline"
)


#############################################################################
#### CLI Error Handling & Validation                                     ####
#############################################################################

# Missing arguments for complex flags
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_missing_separator_args -s "<" "=")
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_missing_center_args    -C "[")
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_missing_indent_args    -I 10 "|")

# Missing argument for --indent / -i
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_missing_indent_arg     -i)

# Missing argument for --columns / -x
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_missing_columns_arg    -x)

# Invalid escape sequences (with complete 2-hex and 3-octal digit lengths)
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_invalid_hex_escape     -w "Test \\xGG")
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_invalid_hex_char       -w "Bad \\x1Z")
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_invalid_octal_escape   -w "Test \\0999")
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_invalid_octal_digit    -w "Bad \\0888")

# Invalid hexadecimal characters in Unicode escape sequences
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_invalid_unicode_u      -w "Bad \\u123Z")
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_invalid_unicode_U      -w "Bad \\U0001F60Z")

# Invalid console width (> 4096)
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_console_width_overflow -x 5000 -c "Test")

# Unrecognized arguments & flags
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_unknown_flag           --invalid-flag)
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_invalid_option_string  --bad-opt "Test")
ADD_PRINT_UTF8_FAIL_TEST(printutf8_fail_separator_extra_arg    -s "<" "=" ">" extra_string)
