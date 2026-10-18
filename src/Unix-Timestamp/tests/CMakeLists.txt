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
# Unix-Timestamp-Tools Test Suite
# Copyright (C) 2026 by Thomas Dreibholz
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

# Helper for unixts2time tests (expected exit code 0)
FUNCTION(ADD_UNIXTS2TIME_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:unixts2time> ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "unixts2time;cli"
   )
ENDFUNCTION()

# Helper for time2unixts tests (expected exit code 0)
FUNCTION(ADD_TIME2UNIXTS_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:time2unixts> ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "time2unixts;cli"
   )
ENDFUNCTION()

# Helper for expected failure tests (non-zero exit code)
FUNCTION(ADD_TIMESTAMP_FAIL_TEST TEST_NAME PROG)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:${PROG}> ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "${PROG};negative;error"
   )
ENDFUNCTION()

# Helper to register an individual check from check_float_unixts
FUNCTION(ADD_FLOAT_UNIXTS_CHECK TEST_NAME INPUT_TS EXPECTED_DATETIME) # Optional: UNIT
   SET(CMD_ARGS)
   IF(ARGN)
      LIST(APPEND CMD_ARGS "--${ARGN}")
   ENDIF()
   LIST(APPEND CMD_ARGS "--" "${INPUT_TS}")

   # Escape dots in expected date/time string for regex
   STRING(REPLACE "." "\\." EXPECTED_DATETIME_ESC "${EXPECTED_DATETIME}")

   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:unixts2time> ${CMD_ARGS}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      PASS_REGULAR_EXPRESSION "^${EXPECTED_DATETIME_ESC}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "unixts2time;float"
   )
ENDFUNCTION()


#############################################################################
#### 1. Basic CLI Options & Internationalization                         ####
#############################################################################

# Test unixts2time version flag (-v)
ADD_UNIXTS2TIME_TEST(unixts2time_cli_version_short -v)

# Test unixts2time version flag (--version)
ADD_UNIXTS2TIME_TEST(unixts2time_cli_version_long  --version)
SET_TESTS_PROPERTIES(unixts2time_cli_version_short unixts2time_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "unixts2time [0-9]+\\.[0-9]+"
)

# Test unixts2time help flag (-h)
ADD_UNIXTS2TIME_TEST(unixts2time_cli_help_short -h)

# Test unixts2time help flag (--help)
ADD_UNIXTS2TIME_TEST(unixts2time_cli_help_long  --help)
SET_TESTS_PROPERTIES(unixts2time_cli_help_short unixts2time_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test unixts2time localized help in German (de)
ADD_UNIXTS2TIME_TEST(unixts2time_cli_help_i18n_de -h)
SET_TESTS_PROPERTIES(unixts2time_cli_help_i18n_de PROPERTIES
   ENVIRONMENT "LANGUAGE=de;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Aufruf:"
   LABELS "unixts2time;cli;i18n;locale_de"
)

# Test unixts2time localized help in Norwegian Bokmål (nb)
ADD_UNIXTS2TIME_TEST(unixts2time_cli_help_i18n_nb -h)
SET_TESTS_PROPERTIES(unixts2time_cli_help_i18n_nb PROPERTIES
   ENVIRONMENT "LANGUAGE=nb;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Bruk:"
   LABELS "unixts2time;cli;i18n;locale_nb"
)

# Test time2unixts version flag (-v)
ADD_TIME2UNIXTS_TEST(time2unixts_cli_version_short -v)

# Test time2unixts version flag (--version)
ADD_TIME2UNIXTS_TEST(time2unixts_cli_version_long  --version)
SET_TESTS_PROPERTIES(time2unixts_cli_version_short time2unixts_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "time2unixts [0-9]+\\.[0-9]+"
)

# Test time2unixts help flag (-h)
ADD_TIME2UNIXTS_TEST(time2unixts_cli_help_short -h)

# Test time2unixts help flag (--help)
ADD_TIME2UNIXTS_TEST(time2unixts_cli_help_long  --help)
SET_TESTS_PROPERTIES(time2unixts_cli_help_short time2unixts_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test time2unixts localized help in German (de)
ADD_TIME2UNIXTS_TEST(time2unixts_cli_help_i18n_de -h)
SET_TESTS_PROPERTIES(time2unixts_cli_help_i18n_de PROPERTIES
   ENVIRONMENT "LANGUAGE=de;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Aufruf:"
   LABELS "time2unixts;cli;i18n;locale_de"
)

# Test time2unixts localized help in Norwegian Bokmål (nb)
ADD_TIME2UNIXTS_TEST(time2unixts_cli_help_i18n_nb -h)
SET_TESTS_PROPERTIES(time2unixts_cli_help_i18n_nb PROPERTIES
   ENVIRONMENT "LANGUAGE=nb;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Bruk:"
   LABELS "time2unixts;cli;i18n;locale_nb"
)

# Test unixts2time current time query (default when no argument is given)
ADD_UNIXTS2TIME_TEST(unixts2time_current_time_default)
SET_TESTS_PROPERTIES(unixts2time_current_time_default PROPERTIES
   PASS_REGULAR_EXPRESSION "^[0-9]+-[0-9]+-[0-9]+ [0-9]+:[0-9]+:[0-9]+"
)

# Test time2unixts current time query (default when no argument is given)
ADD_TIME2UNIXTS_TEST(time2unixts_current_time_default)
SET_TESTS_PROPERTIES(time2unixts_current_time_default PROPERTIES
   PASS_REGULAR_EXPRESSION "^[0-9]+"
)


#############################################################################
#### 2. Auto-Detection of Unix Timestamp Unit Boundaries                ####
#############################################################################

# ------ Positive Integer Boundaries: Manual Flag vs Auto-Detection --------
# Test positive seconds minimum boundary: manual flag -s vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_pos_manual_s_min 0 -s)
ADD_UNIXTS2TIME_TEST(autodetect_pos_auto_s_min   0)
SET_TESTS_PROPERTIES(autodetect_pos_manual_s_min autodetect_pos_auto_s_min PROPERTIES
   PASS_REGULAR_EXPRESSION "^1970-01-01 00:00:00"
)

# Test positive seconds maximum boundary: manual flag -s vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_pos_manual_s_max 4999999999 -s)
ADD_UNIXTS2TIME_TEST(autodetect_pos_auto_s_max   4999999999)
SET_TESTS_PROPERTIES(autodetect_pos_manual_s_max autodetect_pos_auto_s_max PROPERTIES
   PASS_REGULAR_EXPRESSION "^2128-06-11 08:53:19"
)

# Test positive milliseconds minimum boundary: manual flag -m vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_pos_manual_ms_min 5000000000 -m)
ADD_UNIXTS2TIME_TEST(autodetect_pos_auto_ms_min   5000000000)
SET_TESTS_PROPERTIES(autodetect_pos_manual_ms_min autodetect_pos_auto_ms_min PROPERTIES
   PASS_REGULAR_EXPRESSION "^1970-02-27 20:53:20\\.000"
)

# Test positive milliseconds maximum boundary: manual flag -m vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_pos_manual_ms_max 4999999999999 -m)
ADD_UNIXTS2TIME_TEST(autodetect_pos_auto_ms_max   4999999999999)
SET_TESTS_PROPERTIES(autodetect_pos_manual_ms_max autodetect_pos_auto_ms_max PROPERTIES
   PASS_REGULAR_EXPRESSION "^2128-06-11 08:53:19\\.999"
)

# Test positive microseconds minimum boundary: manual flag -u vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_pos_manual_us_min 5000000000000 -u)
ADD_UNIXTS2TIME_TEST(autodetect_pos_auto_us_min   5000000000000)
SET_TESTS_PROPERTIES(autodetect_pos_manual_us_min autodetect_pos_auto_us_min PROPERTIES
   PASS_REGULAR_EXPRESSION "^1970-02-27 20:53:20\\.000000"
)

# Test positive microseconds maximum boundary: manual flag -u vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_pos_manual_us_max 4999999999999999 -u)
ADD_UNIXTS2TIME_TEST(autodetect_pos_auto_us_max   4999999999999999)
SET_TESTS_PROPERTIES(autodetect_pos_manual_us_max autodetect_pos_auto_us_max PROPERTIES
   PASS_REGULAR_EXPRESSION "^2128-06-11 08:53:19\\.999999"
)

# Test positive nanoseconds minimum boundary: manual flag -n vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_pos_manual_ns_min 5000000000000000 -n)
ADD_UNIXTS2TIME_TEST(autodetect_pos_auto_ns_min   5000000000000000)
SET_TESTS_PROPERTIES(autodetect_pos_manual_ns_min autodetect_pos_auto_ns_min PROPERTIES
   PASS_REGULAR_EXPRESSION "^1970-02-27 20:53:20\\.000000000"
)

# Test positive nanoseconds maximum boundary: manual flag -n vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_pos_manual_ns_max 4999999999999999999 -n)
ADD_UNIXTS2TIME_TEST(autodetect_pos_auto_ns_max   4999999999999999999)
SET_TESTS_PROPERTIES(autodetect_pos_manual_ns_max autodetect_pos_auto_ns_max PROPERTIES
   PASS_REGULAR_EXPRESSION "^2128-06-11 08:53:19\\.999999999"
)

# ------ Negative Integer Boundaries: Manual Flag vs Auto-Detection --------
# Test negative seconds zero boundary: manual flag -s vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_neg_manual_s_zero -s -- 0)
ADD_UNIXTS2TIME_TEST(autodetect_neg_auto_s_zero   -- 0)
SET_TESTS_PROPERTIES(autodetect_neg_manual_s_zero autodetect_neg_auto_s_zero PROPERTIES
   PASS_REGULAR_EXPRESSION "^1970-01-01 00:00:00"
)

# Test negative seconds maximum boundary: manual flag -s vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_neg_manual_s_max -s -- -4999999999)
ADD_UNIXTS2TIME_TEST(autodetect_neg_auto_s_max   -- -4999999999)
SET_TESTS_PROPERTIES(autodetect_neg_manual_s_max autodetect_neg_auto_s_max PROPERTIES
   PASS_REGULAR_EXPRESSION "^1811-07-23 15:06:41"
)

# Test negative milliseconds minimum boundary: manual flag -m vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_neg_manual_ms_min -m -- -5000000000)
ADD_UNIXTS2TIME_TEST(autodetect_neg_auto_ms_min   -- -5000000000)
SET_TESTS_PROPERTIES(autodetect_neg_manual_ms_min autodetect_neg_auto_ms_min PROPERTIES
   PASS_REGULAR_EXPRESSION "^1969-11-04 03:06:40\\.000"
)

# Test negative milliseconds maximum boundary: manual flag -m vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_neg_manual_ms_max -m -- -4999999999999)
ADD_UNIXTS2TIME_TEST(autodetect_neg_auto_ms_max   -- -4999999999999)
SET_TESTS_PROPERTIES(autodetect_neg_manual_ms_max autodetect_neg_auto_ms_max PROPERTIES
   PASS_REGULAR_EXPRESSION "^1811-07-23 15:06:40\\.001"
)

# Test negative microseconds minimum boundary: manual flag -u vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_neg_manual_us_min -u -- -5000000000000)
ADD_UNIXTS2TIME_TEST(autodetect_neg_auto_us_min   -- -5000000000000)
SET_TESTS_PROPERTIES(autodetect_neg_manual_us_min autodetect_neg_auto_us_min PROPERTIES
   PASS_REGULAR_EXPRESSION "^1969-11-04 03:06:40\\.000000"
)

# Test negative microseconds maximum boundary: manual flag -u vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_neg_manual_us_max -u -- -4999999999999999)
ADD_UNIXTS2TIME_TEST(autodetect_neg_auto_us_max   -- -4999999999999999)
SET_TESTS_PROPERTIES(autodetect_neg_manual_us_max autodetect_neg_auto_us_max PROPERTIES
   PASS_REGULAR_EXPRESSION "^1811-07-23 15:06:40\\.000001"
)

# Test negative nanoseconds minimum boundary: manual flag -n vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_neg_manual_ns_min -n -- -5000000000000000)
ADD_UNIXTS2TIME_TEST(autodetect_neg_auto_ns_min   -- -5000000000000000)
SET_TESTS_PROPERTIES(autodetect_neg_manual_ns_min autodetect_neg_auto_ns_min PROPERTIES
   PASS_REGULAR_EXPRESSION "^1969-11-04 03:06:40\\.000000000"
)

# Test negative nanoseconds maximum boundary: manual flag -n vs auto-detection[cite: 12]
ADD_UNIXTS2TIME_TEST(autodetect_neg_manual_ns_max -n -- -4999999999999999999)
ADD_UNIXTS2TIME_TEST(autodetect_neg_auto_ns_max   -- -4999999999999999999)
SET_TESTS_PROPERTIES(autodetect_neg_manual_ns_max autodetect_neg_auto_ns_max PROPERTIES
   PASS_REGULAR_EXPRESSION "^1811-07-23 15:06:40\\.000000001"
)


#############################################################################
#### 3. Float Input Tests (Precision, Graduation & Boundaries)           ####
#############################################################################

# ------ 3.1 Multi-Unit Auto-Detection (Positive & Negative) ----------------
# Test float auto-detection for positive seconds[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_autodetect_pos_s  "1788550214.7787085"      "2026-09-04 19:30:14.7787085")

# Test float auto-detection for positive milliseconds[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_autodetect_pos_ms "1788550214778.7085"      "2026-09-04 19:30:14.7787085")

# Test float auto-detection for positive microseconds[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_autodetect_pos_us "1788550214778708.5"      "2026-09-04 19:30:14.7787085")

# Test float auto-detection for positive nanoseconds[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_autodetect_pos_ns "1788550214778708500.0"  "2026-09-04 19:30:14.778708500")

# Test float auto-detection for negative seconds[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_autodetect_neg_s  "-1788550214.7787085"     "1913-04-29 04:29:45.2212915")

# Test float auto-detection for negative milliseconds[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_autodetect_neg_ms "-1788550214778.7085"     "1913-04-29 04:29:45.2212915")

# Test float auto-detection for negative microseconds[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_autodetect_neg_us "-1788550214778708.5"     "1913-04-29 04:29:45.2212915")

# Test float auto-detection for negative nanoseconds[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_autodetect_neg_ns "-1788550214778708500.0" "1913-04-29 04:29:45.221291500")

# ------ 3.2 Explicit Zero Precision & Trailing Zero Retention --------------
# Test explicit single zero precision retention[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_zero_prec_1 "0.0"                "1970-01-01 00:00:00.0")

# Test explicit three zeroes precision retention[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_zero_prec_3 "0.000"              "1970-01-01 00:00:00.000")

# Test explicit nine zeroes precision retention[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_zero_prec_9 "0.000000000"        "1970-01-01 00:00:00.000000000")

# Test single trailing zero retention on non-zero timestamp[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_trailing_0  "1788550214.0"       "2026-09-04 19:30:14.0")

# Test multiple trailing zeroes retention[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_trailing_3  "1788550214.7787000" "2026-09-04 19:30:14.7787000")

# Test four trailing zeroes retention[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_trailing_4  "100.5000"           "1970-01-01 00:01:40.5000")

# ------ 3.3 Sub-Second Precision Graduation & Nanosecond Capping -----------
# Test 1 fractional digit precision[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_prec_grad_1 "1.5"                      "1970-01-01 00:00:01.5")

# Test 2 fractional digits precision[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_prec_grad_2 "1.25"                     "1970-01-01 00:00:01.25")

# Test 3 fractional digits precision[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_prec_grad_3 "1.123"                    "1970-01-01 00:00:01.123")

# Test 5 fractional digits precision[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_prec_grad_5 "1.12345"                  "1970-01-01 00:00:01.12345")

# Test 8 fractional digits precision[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_prec_grad_8 "1.12345678"               "1970-01-01 00:00:01.12345678")

# Test 9 fractional digits precision[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_prec_grad_9 "1.123456789"              "1970-01-01 00:00:01.123456789")

# Test fractional digits capped at 9 decimals[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_prec_capped "1788550214.1234567890123" "2026-09-04 19:30:14.123456789")

# ------ 3.4 Leading Fractional Zeros & High Sub-Second Scale ---------------
# Test leading fractional zero preservation (5 decimals)[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_leading_zero_5 "1.00001"     "1970-01-01 00:00:01.00001")

# Test leading fractional zero preservation (9 decimals)[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_leading_zero_9 "1.000000001" "1970-01-01 00:00:01.000000001")

# Test maximum fractional sub-second value[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_scale_9        "0.999999999" "1970-01-01 00:00:00.999999999")

# ------ 3.5 Negative Epoch Boundaries & Complement Borrow Arithmetic -------
# Test negative epoch boundary: -1.0[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_neg_epoch_1_0      "-1.0"        "1969-12-31 23:59:59.0")

# Test negative epoch boundary: -1.5[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_neg_epoch_1_5      "-1.5"        "1969-12-31 23:59:58.5")

# Test negative epoch boundary: -86400.123 (1 day back)[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_neg_epoch_day      "-86400.123"  "1969-12-30 23:59:59.877")

# Test negative epoch boundary: -31536000.1 (1 year back)[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_neg_epoch_year     "-31536000.1" "1968-12-31 23:59:59.9")

# ------ 3.6 Epoch & Calendar Edge Cases ------------------------------------
# Test Y2K38 signed 32-bit integer limit[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_edge_y2k38    "2147483647.5"      "2038-01-19 03:14:07.5")

# Test leap day calendar calculation (2024-02-29)[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_edge_leap_day "1709208000.123456" "2024-02-29 12:00:00.123456")

# Test Y2K99 calendar limit[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_edge_y2k99    "4102444799.999"    "2099-12-31 23:59:59.999")

# ------ 3.7 Auto-Detected Sub-Second Ranges --------------------------------
# Test sub-second auto-detection in millisecond range[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_autodetect_subsecond_ms "5000000000.12"   "1970-02-27 20:53:20.00012")

# Test sub-second auto-detection in microsecond range[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_autodetect_subsecond_us "5000000000000.5" "1970-02-27 20:53:20.0000005")

# ------ 3.8 Explicit Unit Flag Overrides -----------------------------------
# Test explicit seconds override on timestamp > 5e9[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_override_sec_5e9   "5000000000.5"   "2128-06-11 08:53:20.5"         seconds)

# Test explicit milliseconds override on small timestamp[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_override_ms_1000   "1000.5"         "1970-01-01 00:00:01.0005"      milliseconds)

# Test explicit milliseconds override with 4 decimals[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_override_ms_10     "10.1234"        "1970-01-01 00:00:00.0101234"   milliseconds)

# Test explicit milliseconds override with 1 decimal[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_override_ms_1      "1.1"            "1970-01-01 00:00:00.0011"      milliseconds)

# Test explicit microseconds override[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_override_us_1e6    "1000000.12"     "1970-01-01 00:00:01.00000012"  microseconds)

# Test explicit microseconds override on small timestamp[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_override_us_10     "10.1"           "1970-01-01 00:00:00.0000101"   microseconds)

# Test explicit nanoseconds override[cite: 12]
ADD_FLOAT_UNIXTS_CHECK(float_override_ns_1e9    "1000000000.123" "1970-01-01 00:00:01.000000000" nanoseconds)


#############################################################################
#### 4. Integer Input Tests (Comprehensive Conversion Table)             ####
#############################################################################

# Dataset from run-tests: Timestamp Value | Human-Readable Date | Time Unit[cite: 12]
SET(TIMESTAMP_DATASET
   "178524103|1975-08-29 06:01:43|seconds"
   "599224425|1988-12-27 11:13:45|seconds"
   "958596446|2000-05-17 20:47:26|seconds"
   "3163129385|2070-03-27 07:03:05|seconds"
   "2906315757|2062-02-04 21:55:57|seconds"
   "2653228291|2054-01-28 15:51:31|seconds"
   "3975144315|2095-12-19 14:45:15|seconds"
   "2290358181|2042-07-30 18:36:21|seconds"
   "3048911309|2066-08-13 07:48:29|seconds"
   "1571306751|2019-10-17 10:05:51|seconds"
   "2452336048|2047-09-17 12:27:28|seconds"
   "2515937522|2049-09-22 15:32:02|seconds"
   "3143464376|2069-08-11 16:32:56|seconds"
   "711621471|1992-07-20 08:37:51|seconds"
   "3329457035|2075-07-04 09:10:35|seconds"
   "3540658286|2082-03-13 20:11:26|seconds"
   "1429319277|2015-04-18 01:07:57|seconds"
   "1647999605|2022-03-23 01:40:05|seconds"
   "2726762725|2056-05-28 18:05:25|seconds"
   "3391361277|2077-06-19 20:47:57|seconds"
   "1545524063|2018-12-23 00:14:23|seconds"
   "3958785211|2095-06-13 06:33:31|seconds"
   "1321275334|2011-11-14 12:55:34|seconds"
   "1681700205|2023-04-17 02:56:45|seconds"
   "2994107253|2064-11-17 00:27:33|seconds"
   "880236376|1997-11-22 22:06:16|seconds"
   "3270773552|2073-08-24 04:12:32|seconds"
   "2818690955|2059-04-27 17:42:35|seconds"
   "3710121910|2087-07-27 05:25:10|seconds"
   "3918321625|2094-03-01 22:40:25|seconds"
   "1567117653|2019-08-29 22:27:33|seconds"
   "469390093|1984-11-15 18:08:13|seconds"
   "1827455576|2027-11-29 02:32:56|seconds"
   "3523752789|2081-08-30 04:13:09|seconds"
   "2728152499|2056-06-13 20:08:19|seconds"
   "1712571197|2024-04-08 10:13:17|seconds"
   "3866013976|2092-07-04 12:46:16|seconds"
   "280646616|1978-11-23 05:23:36|seconds"
   "2170646559|2038-10-14 05:22:39|seconds"
   "3003076916|2065-02-28 20:01:56|seconds"
   "2869562755|2060-12-06 12:45:55|seconds"
   "1495086441|2017-05-18 05:47:21|seconds"
   "3815353149|2090-11-26 04:19:09|seconds"
   "1733395257|2024-12-05 10:40:57|seconds"
   "1732459142|2024-11-24 14:39:02|seconds"
   "3442899665|2079-02-06 09:01:05|seconds"
   "3167729207|2070-05-19 12:46:47|seconds"
   "3436253583|2078-11-21 10:53:03|seconds"
   "1335090736|2012-04-22 10:32:16|seconds"
   "2850779746|2060-05-03 03:15:46|seconds"
   "76560682|1972-06-05 02:51:22|seconds"
   "3575588326|2083-04-22 02:58:46|seconds"
   "1516852069|2018-01-25 03:47:49|seconds"
   "2553172558|2050-11-27 14:35:58|seconds"
   "509114047|1986-02-18 12:34:07|seconds"
   "1979695093|2032-09-25 03:18:13|seconds"
   "1058196492|2003-07-14 15:28:12|seconds"
   "735373227|1993-04-21 06:20:27|seconds"
   "3417592494|2078-04-19 11:14:54|seconds"
   "3658651844|2085-12-08 12:10:44|seconds"
   "1347451066|2012-09-12 11:57:46|seconds"
   "2917410705|2062-06-13 07:51:45|seconds"
   "2965852581|2063-12-25 23:56:21|seconds"
   "3958991959|2095-06-15 15:59:19|seconds"
   "3930324547619|2094-07-18 20:49:07.619|milliseconds"
   "1963476881617|2032-03-21 10:14:41.617|milliseconds"
   "1137548661823|2006-01-18 01:44:21.823|milliseconds"
   "863208941560|1997-05-09 20:15:41.560|milliseconds"
   "289052463653|1979-02-28 12:21:03.653|milliseconds"
   "2256762315992|2041-07-06 22:25:15.992|milliseconds"
   "2657181220993|2054-03-15 09:53:40.993|milliseconds"
   "2479474942792|2048-07-27 15:02:22.792|milliseconds"
   "188464899073|1975-12-22 07:21:39.073|milliseconds"
   "1750070315985|2025-06-16 10:38:35.985|milliseconds"
   "2804281916923|2058-11-11 23:11:56.923|milliseconds"
   "2661786875055|2054-05-07 17:14:35.055|milliseconds"
   "2901675077017|2061-12-13 04:51:17.017|milliseconds"
   "2727295165201|2056-06-03 21:59:25.201|milliseconds"
   "846203264107|1996-10-25 00:27:44.107|milliseconds"
   "1223747138201|2008-10-11 17:45:38.201|milliseconds"
   "1942734845767|2031-07-25 08:34:05.767|milliseconds"
   "3936469883817|2094-09-27 23:51:23.817|milliseconds"
   "2442454678321|2047-05-26 03:37:58.321|milliseconds"
   "53538893814|1971-09-12 15:54:53.814|milliseconds"
   "4023820606625|2097-07-04 23:56:46.625|milliseconds"
   "1432766604442|2015-05-27 22:43:24.442|milliseconds"
   "2457189042809|2047-11-12 16:30:42.809|milliseconds"
   "982711321562|2001-02-20 23:22:01.562|milliseconds"
   "196849365794|1976-03-28 08:22:45.794|milliseconds"
   "2385468589743|2045-08-04 14:09:49.743|milliseconds"
   "2773503857314|2057-11-20 17:44:17.314|milliseconds"
   "3643978289912|2085-06-21 16:11:29.912|milliseconds"
   "917621919626|1999-01-29 14:58:39.626|milliseconds"
   "776505409541|1994-08-10 07:56:49.541|milliseconds"
   "2262218519541|2041-09-08 02:01:59.541|milliseconds"
   "794514901793|1995-03-06 18:35:01.793|milliseconds"
   "2846006348051|2060-03-08 21:19:08.051|milliseconds"
   "3205608269506|2071-07-31 22:44:29.506|milliseconds"
   "3022108090754|2065-10-07 02:28:10.754|milliseconds"
   "2507230353466|2049-06-13 20:52:33.466|milliseconds"
   "3860271217025|2092-04-29 01:33:37.025|milliseconds"
   "3457401572534|2079-07-24 05:19:32.534|milliseconds"
   "2094171666555|2036-05-12 02:21:06.555|milliseconds"
   "3230592745669|2072-05-16 02:52:25.669|milliseconds"
   "3130476195955|2069-03-14 08:43:15.955|milliseconds"
   "1514443863752|2017-12-28 06:51:03.752|milliseconds"
   "2398223141817|2045-12-30 05:05:41.817|milliseconds"
   "2443731518660|2047-06-09 22:18:38.660|milliseconds"
   "1105042079268|2005-01-06 20:07:59.268|milliseconds"
   "286853177725|1979-02-03 01:26:17.725|milliseconds"
   "560452419970|1987-10-05 17:13:39.970|milliseconds"
   "252972607925|1978-01-06 22:10:07.925|milliseconds"
   "730639503323|1993-02-25 11:25:03.323|milliseconds"
   "3521787900080|2081-08-07 10:25:00.080|milliseconds"
   "824056009450|1996-02-11 16:26:49.450|milliseconds"
   "4091671804553|2099-08-29 07:30:04.553|milliseconds"
   "3579966865259|2083-06-11 19:14:25.259|milliseconds"
   "3769190007054|2089-06-09 21:13:27.054|milliseconds"
   "2381711165153|2045-06-22 02:26:05.153|milliseconds"
   "1834190173357|2028-02-15 01:16:13.357|milliseconds"
   "2604437248734|2052-07-12 22:47:28.734|milliseconds"
   "3784448869928|2089-12-03 11:47:49.928|milliseconds"
   "3998696687043|2096-09-17 05:04:47.043|milliseconds"
   "1846597804294|2028-07-07 15:50:04.294|milliseconds"
   "3182922815994|2070-11-11 09:13:35.994|milliseconds"
   "668144104057|1991-03-05 03:35:04.057|milliseconds"
   "2722666735472|2056-04-11 08:18:55.472|milliseconds"
   "1491643388389|2017-04-08 09:23:08.389|milliseconds"
   "293815531218540|1979-04-24 15:25:31.218540|microseconds"
   "1513639072689084|2017-12-18 23:17:52.689084|microseconds"
   "2907105178675014|2062-02-14 01:12:58.675014|microseconds"
   "2581105743276471|2051-10-16 21:49:03.276471|microseconds"
   "3081113281224135|2067-08-21 00:48:01.224135|microseconds"
   "822579643142745|1996-01-25 14:20:43.142745|microseconds"
   "2079049588410911|2035-11-19 01:46:28.410911|microseconds"
   "2633554303450463|2053-06-14 22:51:43.450463|microseconds"
   "2374349589265152|2045-03-28 21:33:09.265152|microseconds"
   "686019379232200|1991-09-28 00:56:19.232200|microseconds"
   "3055427831883611|2066-10-27 17:57:11.883611|microseconds"
   "1433449102412038|2015-06-04 20:18:22.412038|microseconds"
   "2681632915203981|2054-12-23 10:01:55.203981|microseconds"
   "3444462029799780|2079-02-24 11:00:29.799780|microseconds"
   "2683919562974726|2055-01-18 21:12:42.974726|microseconds"
   "1793266976979744|2026-10-29 09:42:56.979744|microseconds"
   "683298226752780|1991-08-27 13:03:46.752780|microseconds"
   "2015242014652189|2033-11-10 13:26:54.652189|microseconds"
   "2996405180659739|2064-12-13 14:46:20.659739|microseconds"
   "1381571935041286|2013-10-12 09:58:55.041286|microseconds"
   "129864835566195|1974-02-12 01:33:55.566195|microseconds"
   "2576749459201452|2051-08-27 11:44:19.201452|microseconds"
   "2754388966458733|2057-04-13 12:02:46.458733|microseconds"
   "1318297467800315|2011-10-11 01:44:27.800315|microseconds"
   "4050050915572649|2098-05-04 14:08:35.572649|microseconds"
   "1353480419818729|2012-11-21 06:46:59.818729|microseconds"
   "1728278760916451|2024-10-07 05:26:00.916451|microseconds"
   "1540579954236028|2018-10-26 18:52:34.236028|microseconds"
   "3939580993320107|2094-11-03 00:03:13.320107|microseconds"
   "3830574860461442|2091-05-21 08:34:20.461442|microseconds"
   "2491675976397469|2048-12-15 20:12:56.397469|microseconds"
   "2733959353398675|2056-08-20 01:09:13.398675|microseconds"
   "618773037653960|1989-08-10 17:23:57.653960|microseconds"
   "3523376412289021|2081-08-25 19:40:12.289021|microseconds"
   "3275317884782545|2073-10-15 18:31:24.782545|microseconds"
   "1016668653404144|2002-03-20 23:57:33.404144|microseconds"
   "3492288985742766|2080-08-31 00:16:25.742766|microseconds"
   "368642508483155|1981-09-06 16:41:48.483155|microseconds"
   "2980861602003017|2064-06-16 17:06:42.003017|microseconds"
   "1471018971089953|2016-08-12 16:22:51.089953|microseconds"
   "3613188834977022|2084-06-30 07:33:54.977022|microseconds"
   "1182016813515610|2007-06-16 18:00:13.515610|microseconds"
   "2264138312859330|2041-09-30 07:18:32.859330|microseconds"
   "2552044458952411|2050-11-14 13:14:18.952411|microseconds"
   "1669701273146318|2022-11-29 05:54:33.146318|microseconds"
   "159541581900558|1975-01-21 13:06:21.900558|microseconds"
   "4051221693528773|2098-05-18 03:21:33.528773|microseconds"
   "2496062452397184|2049-02-04 14:40:52.397184|microseconds"
   "839908678670556|1996-08-13 03:57:58.670556|microseconds"
   "1908307132119206|2030-06-21 21:18:52.119206|microseconds"
   "2864729341254830|2060-10-11 14:09:01.254830|microseconds"
   "1294053579054790|2011-01-03 11:19:39.054790|microseconds"
   "2111588892829222|2036-11-29 16:28:12.829222|microseconds"
   "3079417745222790|2067-08-01 09:49:05.222790|microseconds"
   "1527527484560954|2018-05-28 17:11:24.560954|microseconds"
   "1410431502974055|2014-09-11 10:31:42.974055|microseconds"
   "1188703597608142|2007-09-02 03:26:37.608142|microseconds"
   "442225738713779|1984-01-06 08:28:58.713779|microseconds"
   "3678342538705735|2086-07-24 09:48:58.705735|microseconds"
   "4081321976159951|2099-05-01 12:32:56.159951|microseconds"
   "2172488312807746|2038-11-04 12:58:32.807746|microseconds"
   "2538145168704219|2050-06-06 16:19:28.704219|microseconds"
   "2766960655045323|2057-09-06 00:10:55.045323|microseconds"
   "2753522936587658|2057-04-03 11:28:56.587658|microseconds"
   "2131446869318110610|2037-07-17 12:34:29.318110610|nanoseconds"
   "68048933159647579|1972-02-27 14:28:53.159647579|nanoseconds"
   "636226707500268472|1990-02-28 17:38:27.500268472|nanoseconds"
   "345026917982693873|1980-12-07 08:48:37.982693873|nanoseconds"
   "3750552627697184457|2088-11-06 04:10:27.697184457|nanoseconds"
   "3147877333660639437|2069-10-01 18:22:13.660639437|nanoseconds"
   "2099889881999101536|2036-07-17 06:44:41.999101536|nanoseconds"
   "1965178657789404112|2032-04-10 02:57:37.789404112|nanoseconds"
   "3099751409022393975|2068-03-23 18:03:29.022393975|nanoseconds"
   "752786790849035200|1993-11-08 19:26:30.849035200|nanoseconds"
   "828092199174295015|1996-03-29 09:36:39.174295015|nanoseconds"
   "1018115075807262912|2002-04-06 17:44:35.807262912|nanoseconds"
   "957016200695553777|2000-04-29 13:50:00.695553777|nanoseconds"
   "181705039763281687|1975-10-05 01:37:19.763281687|nanoseconds"
   "2717100806858159812|2056-02-06 22:13:26.858159812|nanoseconds"
   "1892602888494950690|2029-12-22 03:01:28.494950690|nanoseconds"
   "3060551415864561720|2066-12-26 01:10:15.864561720|nanoseconds"
   "1743936506722874143|2025-04-06 10:48:26.722874143|nanoseconds"
   "1318171105807820895|2011-10-09 14:38:25.807820895|nanoseconds"
   "1343560813026905428|2012-07-29 11:20:13.026905428|nanoseconds"
   "2968697564457715534|2064-01-27 22:12:44.457715534|nanoseconds"
   "1516437827850636023|2018-01-20 08:43:47.850636023|nanoseconds"
   "2706837320110012493|2055-10-11 03:15:20.110012493|nanoseconds"
   "3371486950182429058|2076-11-01 20:09:10.182429058|nanoseconds"
   "348649918770680521|1981-01-18 07:11:58.770680521|nanoseconds"
   "65900463974521568|1972-02-02 17:41:03.974521568|nanoseconds"
   "1285327527676685932|2010-09-24 11:25:27.676685932|nanoseconds"
   "2833097381922220233|2059-10-11 11:29:41.922220233|nanoseconds"
   "832114687168494768|1996-05-14 22:58:07.168494768|nanoseconds"
   "2938851291211362613|2063-02-16 11:34:51.211362613|nanoseconds"
   "457587993084062197|1984-07-02 03:46:33.084062197|nanoseconds"
   "684885731590603968|1991-09-14 22:02:11.590603968|nanoseconds"
   "2759589439979285057|2057-06-12 16:37:19.979285057|nanoseconds"
   "1738106755620059512|2025-01-28 23:25:55.620059512|nanoseconds"
   "2558288847263636645|2051-01-25 19:47:27.263636645|nanoseconds"
   "3964269456981763216|2095-08-15 17:57:36.981763216|nanoseconds"
   "3842572707063210630|2091-10-07 05:18:27.063210630|nanoseconds"
   "1849535689244195220|2028-08-10 15:54:49.244195220|nanoseconds"
   "2923026858846213412|2062-08-17 07:54:18.846213412|nanoseconds"
   "1785420233352508140|2026-07-30 14:03:53.352508140|nanoseconds"
   "3657164672592274817|2085-11-21 07:04:32.592274817|nanoseconds"
   "1982850421507969864|2032-10-31 15:47:01.507969864|nanoseconds"
   "3197871001208493417|2071-05-03 09:30:01.208493417|nanoseconds"
   "317293341079213360|1980-01-21 09:02:21.079213360|nanoseconds"
   "290486180910794273|1979-03-17 02:36:20.910794273|nanoseconds"
   "2789497811710113372|2058-05-24 20:30:11.710113372|nanoseconds"
   "584789157387518404|1988-07-13 09:25:57.387518404|nanoseconds"
   "1172620868623090437|2007-02-28 00:01:08.623090437|nanoseconds"
   "1626273533029734307|2021-07-14 14:38:53.029734307|nanoseconds"
   "663010868211618|1970-01-08 16:10:10.868211618|nanoseconds"
   "2916712744980134650|2062-06-05 05:59:04.980134650|nanoseconds"
   "4089411564262575581|2099-08-03 03:39:24.262575581|nanoseconds"
   "254248560579690894|1978-01-21 16:36:00.579690894|nanoseconds"
   "2363133575123550993|2044-11-19 01:59:35.123550993|nanoseconds"
   "3629824904700120476|2085-01-08 20:41:44.700120476|nanoseconds"
   "1710473466298868261|2024-03-15 03:31:06.298868261|nanoseconds"
   "2242226067603620709|2041-01-19 16:34:27.603620709|nanoseconds"
   "3936730998308232132|2094-10-01 00:23:18.308232132|nanoseconds"
   "669662876740364355|1991-03-22 17:27:56.740364355|nanoseconds"
   "3945325489007545446|2095-01-08 11:44:49.007545446|nanoseconds"
   "3631088096914206505|2085-01-23 11:34:56.914206505|nanoseconds"
   "1779499196321467203|2026-05-23 01:19:56.321467203|nanoseconds"
   "2088253821398981200|2036-03-04 14:30:21.398981200|nanoseconds"
   "166097209823933295|1975-04-07 10:06:49.823933295|nanoseconds"
   # Boundary edge cases
   "0|1970-01-01 00:00:00|seconds"
   "1234567890|2009-02-13 23:31:30|seconds"
   "2147483647|2038-01-19 03:14:07|seconds"
   "2147483648|2038-01-19 03:14:08|seconds"
   "4102444799|2099-12-31 23:59:59|seconds"
   "1|1970-01-01 00:00:00.001|milliseconds"
   "315575999999|1980-01-01 11:59:59.999|milliseconds"
   "315576000000|1980-01-01 12:00:00.000|milliseconds"
   "951868799999|2000-02-29 23:59:59.999|milliseconds"
   "951868800000|2000-03-01 00:00:00.000|milliseconds"
   "1024567890452|2002-06-20 10:11:30.452|milliseconds"
   "1284562147850|2010-09-15 14:49:07.850|milliseconds"
   "1646092799999|2022-02-28 23:59:59.999|milliseconds"
   "1646092800000|2022-03-01 00:00:00.000|milliseconds"
   "2147483647999|2038-01-19 03:14:07.999|milliseconds"
   "1|1970-01-01 00:00:00.000001|microseconds"
   "946684800456789|2000-01-01 00:00:00.456789|microseconds"
   "1154214875123456|2006-07-29 23:14:35.123456|microseconds"
   "1325684200789000|2012-01-04 13:36:40.789000|microseconds"
   "2524607999999999|2049-12-31 23:59:59.999999|microseconds"
   "2524608000000000|2050-01-01 00:00:00.000000|microseconds"
   "315575999999999|1980-01-01 11:59:59.999999|microseconds"
   "4102444799999999|2099-12-31 23:59:59.999999|microseconds"
   "1|1970-01-01 00:00:00.000000001|nanoseconds"
   "1000000000|1970-01-01 00:00:01.000000000|nanoseconds"
   "1024567890123456789|2002-06-20 10:11:30.123456789|nanoseconds"
   "1254821478963258741|2009-10-06 09:31:18.963258741|nanoseconds"
   "2147483647999999999|2038-01-19 03:14:07.999999999|nanoseconds"
   "315575999999999999|1980-01-01 11:59:59.999999999|nanoseconds"
   "4102444799999999999|2099-12-31 23:59:59.999999999|nanoseconds"
   # Negative pre-epoch boundaries
   "-2147483648|1901-12-13 20:45:52|seconds"
   "-63115200|1968-01-01 12:00:00|seconds"
   "636249600|1990-03-01 00:00:00|seconds"
   "951868800|2000-03-01 00:00:00|seconds"
   "-1|1969-12-31 23:59:59|seconds"
   "-86400|1969-12-31 00:00:00|seconds"
   "-31536000|1969-01-01 00:00:00|seconds"
   "-1|1969-12-31 23:59:59.999|milliseconds"
   "-1024567890452|1937-07-14 13:48:29.548|milliseconds"
   "-315575999999|1960-01-01 12:00:00.001|milliseconds"
   "999|1970-01-01 00:00:00.999|milliseconds"
   "-1|1969-12-31 23:59:59.999999|microseconds"
   "-1154214875123456|1933-06-05 00:45:24.876544|microseconds"
   "1000000|1970-01-01 00:00:01.000000|microseconds"
   "-1000000|1969-12-31 23:59:59.000000|microseconds"
   "-1|1969-12-31 23:59:59.999999999|nanoseconds"
   "-315575999999999999|1960-01-01 12:00:00.000000001|nanoseconds"
   "-1000000000|1969-12-31 23:59:59.000000000|nanoseconds"
   "1623839208725063000|2021-06-16 10:26:48.725063000|nanoseconds"
)

SET(ENTRY_INDEX 0)
FOREACH(ROW ${TIMESTAMP_DATASET})
   MATH(EXPR ENTRY_INDEX "${ENTRY_INDEX} + 1")
   STRING(REPLACE "|" ";" ROW_FIELDS "${ROW}")
   LIST(GET ROW_FIELDS 0 TS_VALUE)
   LIST(GET ROW_FIELDS 1 DATE_STR)
   LIST(GET ROW_FIELDS 2 TIME_UNIT)

   # Escape dots in date strings for strict regex matching
   STRING(REPLACE "." "\\." DATE_STR_ESC "${DATE_STR}")

   # Test 1.A: unixts2time with integer timestamp -> ISO date string[cite: 12]
   ADD_TEST(
      NAME unixts2time_table_entry_${ENTRY_INDEX}
      COMMAND $<TARGET_FILE:unixts2time> --${TIME_UNIT} -- "${TS_VALUE}"
   )
   SET_TESTS_PROPERTIES(unixts2time_table_entry_${ENTRY_INDEX} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      PASS_REGULAR_EXPRESSION "^${DATE_STR_ESC}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "unixts2time;table"
   )

   # Test 1.B: time2unixts with ISO date string -> integer timestamp[cite: 12]
   ADD_TEST(
      NAME time2unixts_table_entry_${ENTRY_INDEX}
      COMMAND $<TARGET_FILE:time2unixts> --${TIME_UNIT} "${DATE_STR}"
   )
   SET_TESTS_PROPERTIES(time2unixts_table_entry_${ENTRY_INDEX} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      PASS_REGULAR_EXPRESSION "^${TS_VALUE}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "time2unixts;table"
   )

   # Prepare float representation[cite: 12]
   IF(TIME_UNIT STREQUAL "nanoseconds")
      SET(FLOAT_TS "${TS_VALUE}.000000000")
      SET(EXPECTED_DATE_FLOAT "${DATE_STR}")
   ELSEIF(TIME_UNIT STREQUAL "microseconds")
      SET(FLOAT_TS "${TS_VALUE}.000000")
      SET(EXPECTED_DATE_FLOAT "${DATE_STR}000")
   ELSEIF(TIME_UNIT STREQUAL "milliseconds")
      SET(FLOAT_TS "${TS_VALUE}.000")
      SET(EXPECTED_DATE_FLOAT "${DATE_STR}000")
   ELSE()
      SET(FLOAT_TS "${TS_VALUE}.0")
      SET(EXPECTED_DATE_FLOAT "${DATE_STR}.0")
   ENDIF()
   STRING(REPLACE "." "\\." EXPECTED_DATE_FLOAT_ESC "${EXPECTED_DATE_FLOAT}")

   # Test 2.B: unixts2time with float timestamp -> padded ISO date string[cite: 12]
   ADD_TEST(
      NAME unixts2time_float_table_entry_${ENTRY_INDEX}
      COMMAND $<TARGET_FILE:unixts2time> --${TIME_UNIT} -- "${FLOAT_TS}"
   )
   SET_TESTS_PROPERTIES(unixts2time_float_table_entry_${ENTRY_INDEX} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      PASS_REGULAR_EXPRESSION "^${EXPECTED_DATE_FLOAT_ESC}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "unixts2time;table;float"
   )

   # Test 2.C: time2unixts with --float -> matches numeric float pattern[cite: 12]
   ADD_TEST(
      NAME time2unixts_float_table_entry_${ENTRY_INDEX}
      COMMAND $<TARGET_FILE:time2unixts> --float --${TIME_UNIT} "${DATE_STR}"
   )
   SET_TESTS_PROPERTIES(time2unixts_float_table_entry_${ENTRY_INDEX} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      PASS_REGULAR_EXPRESSION "^${TS_VALUE}(\\.0+)?"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "time2unixts;table;float"
   )
ENDFOREACH()


#############################################################################
#### 5. Additional Output Format Variants & Templates                    ####
#############################################################################

# ------ time2unixts Numeric Format Flags (-I, -X, -0, -F) ------------------
# Test decimal integer output flag (-I / --integer-decimal)[cite: 16]
ADD_TIME2UNIXTS_TEST(time2unixts_fmt_dec -I -s "2026-03-17 10:52:35")
SET_TESTS_PROPERTIES(time2unixts_fmt_dec PROPERTIES
   PASS_REGULAR_EXPRESSION "^1773744755"
)

# Test raw hexadecimal integer output flag (-X / --integer-hexadecimal)
ADD_TIME2UNIXTS_TEST(time2unixts_fmt_hex -X -s "2026-03-17 10:52:35")
SET_TESTS_PROPERTIES(time2unixts_fmt_hex PROPERTIES
   PASS_REGULAR_EXPRESSION "^69b93273"
)

# Test prefixed hexadecimal integer output flag (-0 / --integer-0x-hexadecimal)
ADD_TIME2UNIXTS_TEST(time2unixts_fmt_0x_hex -0 -s "2026-03-17 10:52:35")
SET_TESTS_PROPERTIES(time2unixts_fmt_0x_hex PROPERTIES
   PASS_REGULAR_EXPRESSION "^0x69b93273"
)

# Test floating-point timestamp output flag (-F / --float)[cite: 16]
ADD_TIME2UNIXTS_TEST(time2unixts_fmt_float -F -s "2026-03-17 10:52:35")
SET_TESTS_PROPERTIES(time2unixts_fmt_float PROPERTIES
   PASS_REGULAR_EXPRESSION "^1773744755\\.000000000"
)

# ------ Human-Readable Suffix Flags (-H / --human-readable) ----------------
# Test unixts2time human-readable explanation output (-H)
ADD_UNIXTS2TIME_TEST(unixts2time_human_readable -H -s 1773744755)
SET_TESTS_PROPERTIES(unixts2time_human_readable PROPERTIES
   PASS_REGULAR_EXPRESSION "2026-03-17 10:52:35 is 1773744755 \\(0x69b93273\\) s since the Unix Epoch"
)

# Test time2unixts human-readable explanation output (-H)[cite: 13, 16]
ADD_TIME2UNIXTS_TEST(time2unixts_human_readable -H -s "2026-03-17 10:52:35")
SET_TESTS_PROPERTIES(time2unixts_human_readable PROPERTIES
   PASS_REGULAR_EXPRESSION "1773744755 s since the Unix Epoch"
)

# ------ Custom Time Format Templates (-T / --template) --------------------
# Test unixts2time custom strftime template formatting (-T)[cite: 14, 15]
ADD_UNIXTS2TIME_TEST(unixts2time_custom_template -T "%Y/%m/%d - %H:%M:%S" -s 1773744755)
SET_TESTS_PROPERTIES(unixts2time_custom_template PROPERTIES
   PASS_REGULAR_EXPRESSION "^2026/03/17 - 10:52:35"
)

# Test time2unixts custom strptime template parsing (-T)[cite: 13, 16]
ADD_TIME2UNIXTS_TEST(time2unixts_custom_template -T "%Y/%m/%d - %H:%M:%S" -s "2026/03/17 - 10:52:35")
SET_TESTS_PROPERTIES(time2unixts_custom_template PROPERTIES
   PASS_REGULAR_EXPRESSION "^1773744755"
)

# ------ Hexadecimal and Octal Integer Parsing in unixts2time ---------------
# Test unixts2time parsing hexadecimal integer input (0x prefix)[cite: 14, 15]
ADD_UNIXTS2TIME_TEST(unixts2time_input_hex -s 0x69b93273)
SET_TESTS_PROPERTIES(unixts2time_input_hex PROPERTIES
   PASS_REGULAR_EXPRESSION "^2026-03-17 10:52:35"
)

# Test unixts2time parsing octal integer input (0 prefix)[cite: 14, 15]
ADD_UNIXTS2TIME_TEST(unixts2time_input_octal -s 015156231163)
SET_TESTS_PROPERTIES(unixts2time_input_octal PROPERTIES
   PASS_REGULAR_EXPRESSION "^2026-03-17 10:52:35"
)

# ------ Multi-Argument Batch Conversions -----------------------------------
# Test unixts2time converting multiple positional arguments in batch[cite: 14, 15]
ADD_UNIXTS2TIME_TEST(unixts2time_batch_convert -s 0 1000000000 2000000000)
SET_TESTS_PROPERTIES(unixts2time_batch_convert PROPERTIES
   PASS_REGULAR_EXPRESSION "1970-01-01 00:00:00.*2001-09-09 01:46:40.*2033-05-18 03:33:20"
)

# Test time2unixts converting multiple positional arguments in batch[cite: 13, 16]
ADD_TIME2UNIXTS_TEST(time2unixts_batch_convert -s "1970-01-01 00:00:00" "2001-09-09 01:46:40")
SET_TESTS_PROPERTIES(time2unixts_batch_convert PROPERTIES
   PASS_REGULAR_EXPRESSION "0.*1000000000"
)


#############################################################################
#### 6. Error Handling & Negative Boundaries                             ####
#############################################################################

# Test unixts2time failure on non-numeric input string[cite: 14]
ADD_TIMESTAMP_FAIL_TEST(unixts2time_fail_non_numeric unixts2time "not-a-number")

# Test unixts2time failure on trailing garbage characters[cite: 14]
ADD_TIMESTAMP_FAIL_TEST(unixts2time_fail_trailing_garbage unixts2time "12345abc")

# Test time2unixts failure on invalid date string format[cite: 13]
ADD_TIMESTAMP_FAIL_TEST(time2unixts_fail_invalid_date time2unixts "invalid-date-string")

# Test time2unixts failure on format template mismatch[cite: 13]
ADD_TIMESTAMP_FAIL_TEST(time2unixts_fail_mismatched_template time2unixts -T "%d.%m.%Y" "2026-03-17")

# Test unixts2time failure on unknown command-line option[cite: 14]
ADD_TIMESTAMP_FAIL_TEST(unixts2time_fail_unknown_option unixts2time --invalid-option)

# Test time2unixts failure on unknown command-line option[cite: 13]
ADD_TIMESTAMP_FAIL_TEST(time2unixts_fail_unknown_option time2unixts --invalid-option)
