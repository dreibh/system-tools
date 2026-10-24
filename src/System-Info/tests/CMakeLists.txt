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
# System-Tools
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
STRING(ASCII 27 ESC)

#############################################################################
#### Deterministic Mock Data for System-Info Testing (-X and -Y)         ####
#############################################################################

SET(MOCK_SYSTEM_INFO_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock-system-info.txt")
SET(MOCK_CONFIG_FILE      "${CMAKE_CURRENT_BINARY_DIR}/mock-system-info.conf")
SET(MOCK_DATE_STRING      "Sat Oct 17 22:45:00 CEST 2026")

FILE(WRITE "${MOCK_SYSTEM_INFO_FILE}"
     "hostname_long=\"nordmarka.nntb.no\"\n"
     "uptime_days=12\n"
     "uptime_hours=3\n"
     "uptime_mins=45\n"
     "uptime_secs=30\n"
     "os_name=\"Ubuntu\"\n"
     "os_version=\"24.04.1 LTS\"\n"
     "os_version_id=\"24.04\"\n"
     "os_version_codename=\"Noble Numbat\"\n"
     "system_sysname=\"Linux\"\n"
     "system_release=\"6.8.0-45-generic\"\n"
     "system_machine=\"x86_64\"\n"
     "system_cores=8\n"
     "system_procs=312\n"
     "system_users=2\n"
     "system_load_avg1min=0.45\n"
     "system_load_avg5min=0.38\n"
     "system_load_avg15min=0.29\n"
     "battery_list=\"1\"\n"
     "battery_1_status=3\n"
     "battery_1_capacity=100\n"
     "system_mem_total=16777216000\n"
     "system_mem_used=8388608000\n"
     "system_mem_free=8388608000\n"
     "system_mem_freepct=50.0\n"
     "system_swap_total=4194304000\n"
     "system_swap_used=1048576000\n"
     "system_swap_free=3145728000\n"
     "system_swap_freepct=75.0\n"
     "disk_list=\"root home\"\n"
     "disk_root_pct=42.0\n"
     "disk_home_pct=85.0\n"
     "ssh_key_list=\"1 2\"\n"
     "ssh_key_type_1=\"ED25519\"\n"
     "ssh_key_bits_1=256\n"
     "ssh_key_fingerprint_1=\"SHA256:abcdef1234567890\"\n"
     "ssh_key_type_2=\"RSA\"\n"
     "ssh_key_bits_2=4096\n"
     "ssh_key_fingerprint_2=\"SHA256:0987654321fedcba\"\n"
     "netif_list=\"1\"\n"
     "netif_1_name=\"eth0\"\n"
     "netif_1_flags=\"<UP>\"\n"
     "netif_1_ipv4=\"192.168.1.100/24\"\n"
     "netif_1_ipv6=\"2001:db8::1/64 fe80::1/64\"\n"
)

FILE(WRITE "${MOCK_CONFIG_FILE}"
     "color_basic=\"\\e[31m\"\n"
     "color_network=\"\\e[32m\"\n"
     "color_none=\"\\e[0m\"\n"
)


#############################################################################
#### get-system-info                                                     ####
#############################################################################

# ###### Helper for deterministic commands (expected return code: 0) ########
FUNCTION(ADD_GETSYSTEMINFO_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:get-system-info> ${ARGN}
   )
   SET(TEST_LABELS "get-system-info;cli")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


# ###### Helper for expected failures (expected return code: non-zero) ######
FUNCTION(ADD_GETSYSTEMINFO_FAIL_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:get-system-info> ${ARGN}
   )
   SET(TEST_LABELS "get-system-info;negative;error")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


# === Basic CLI Options =====================================================
ADD_GETSYSTEMINFO_TEST(getsysteminfo_cli_version_short -v)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_cli_version_long  --version)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_cli_help_short    -h)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_cli_help_long     --help)

SET_TESTS_PROPERTIES(getsysteminfo_cli_version_short getsysteminfo_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "get-system-info [0-9]+\\.[0-9]+"
)

SET_TESTS_PROPERTIES(getsysteminfo_cli_help_short getsysteminfo_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# === Subsystem Query Flags =================================================
ADD_GETSYSTEMINFO_TEST(getsysteminfo_query_all            -A)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_query_none           -a)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_with_hostname        -a -O)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_with_uptime          -a -T)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_with_os              -a -E)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_with_kernel          -a -K)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_with_load            -a -L)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_with_memory          -a -M)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_with_network         -a -N)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_with_storage         -a -S)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_with_battery         -a -B)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_with_ssh             -a -F)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_with_user            -a -U)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_compat_version_short -c 0)
ADD_GETSYSTEMINFO_TEST(getsysteminfo_compat_version_long  --compatibility 0)

# Long option equivalents
ADD_GETSYSTEMINFO_TEST(getsysteminfo_long_options
   --none --with-hostname --with-uptime --with-memory --with-load
)

# Follows alphabetical dictionary order: hostname -> netif -> system_mem -> uptime
SET_TESTS_PROPERTIES(getsysteminfo_query_all PROPERTIES
   PASS_REGULAR_EXPRESSION "hostname_long=.*netif_.*system_mem_.*uptime_"
)
SET_TESTS_PROPERTIES(getsysteminfo_with_hostname PROPERTIES
   PASS_REGULAR_EXPRESSION "hostname_long="
)
SET_TESTS_PROPERTIES(getsysteminfo_with_uptime PROPERTIES
   PASS_REGULAR_EXPRESSION "uptime_"
)
SET_TESTS_PROPERTIES(getsysteminfo_with_memory PROPERTIES
   PASS_REGULAR_EXPRESSION "system_mem_"
)
SET_TESTS_PROPERTIES(getsysteminfo_with_network PROPERTIES
   PASS_REGULAR_EXPRESSION "netif_"
)

# === Error Handling & Validation ===========================================
ADD_GETSYSTEMINFO_FAIL_TEST(getsysteminfo_fail_extra_arg       extra_argument)
ADD_GETSYSTEMINFO_FAIL_TEST(getsysteminfo_fail_invalid_compat  -c 9999)
ADD_GETSYSTEMINFO_FAIL_TEST(getsysteminfo_fail_unknown_flag    --invalid-option)


#############################################################################
#### System-Info                                                         ####
#############################################################################

# ###### Helper for deterministic commands (expected return code: 0) ########
FUNCTION(ADD_SYSTEMINFO_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:System-Info> ${ARGN}
   )
   SET(TEST_LABELS "system-info;cli")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


# ###### Helper for expected failures (expected return code: non-zero) ######
FUNCTION(ADD_SYSTEMINFO_FAIL_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:System-Info> ${ARGN}
   )
   SET(TEST_LABELS "system-info;negative;error")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


# === Basic CLI Options =====================================================
ADD_SYSTEMINFO_TEST(systeminfo_cli_version_short -v)
ADD_SYSTEMINFO_TEST(systeminfo_cli_version_long  --version)
ADD_SYSTEMINFO_TEST(systeminfo_cli_help_short    -h)
ADD_SYSTEMINFO_TEST(systeminfo_cli_help_long     --help)

SET_TESTS_PROPERTIES(systeminfo_cli_version_short systeminfo_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "System-Info [0-9]+\\.[0-9]+"
)

SET_TESTS_PROPERTIES(systeminfo_cli_help_short systeminfo_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# === CLI Help Internationalization =========================================
ADD_SYSTEMINFO_TEST(systeminfo_cli_help_i18n_de  -h)
ADD_SYSTEMINFO_TEST(systeminfo_cli_help_i18n_nb  -h)
ADD_SYSTEMINFO_TEST(systeminfo_cli_help_i18n_ar  -h)
ADD_SYSTEMINFO_TEST(systeminfo_cli_help_i18n_he  -h)

SET_TESTS_PROPERTIES(systeminfo_cli_help_i18n_de PROPERTIES
   ENVIRONMENT "LC_ALL=en_US.UTF-8;LANGUAGE=de;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Aufruf:"
   LABELS "system-info;cli;i18n;locale_de"
)

SET_TESTS_PROPERTIES(systeminfo_cli_help_i18n_nb PROPERTIES
   ENVIRONMENT "LC_ALL=en_US.UTF-8;LANGUAGE=nb;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Bruk:"
   LABELS "system-info;cli;i18n;locale_nb"
)

SET_TESTS_PROPERTIES(systeminfo_cli_help_i18n_ar PROPERTIES
   ENVIRONMENT "LC_ALL=en_US.UTF-8;LANGUAGE=ar;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "(الاستخدام|الاستعمال|طريقة الاستخدام)"
   LABELS "system-info;cli;i18n;locale_ar;rtl"
)

SET_TESTS_PROPERTIES(systeminfo_cli_help_i18n_he PROPERTIES
   ENVIRONMENT "LC_ALL=en_US.UTF-8;LANGUAGE=he;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "שימוש:"
   LABELS "system-info;cli;i18n;locale_he;rtl"
)

# === Deterministic Mock Data Tests (=X and =Y) =============================
ADD_SYSTEMINFO_TEST(systeminfo_mock_deterministic_en
   -S /nonexistent
   -X "${MOCK_DATE_STRING}"
   -Y "${MOCK_SYSTEM_INFO_FILE}"
)
SET_TESTS_PROPERTIES(systeminfo_mock_deterministic_en PROPERTIES
   PASS_REGULAR_EXPRESSION "System information on ${MOCK_DATE_STRING}.*Host:[ ]+nordmarka\\.nntb\\.no.*Uptime:[ ]+12 days, 3 hours, 45 minutes, and 30 seconds.*Operating System:[ ]+Ubuntu 24\\.04.*Noble Numbat.*with kernel 6\\.8\\.0-45-generic.*Processor:[ ]+8 cores \\(x86_64\\); 312 processes; 2 users.*Battery:[ ]+#1 full, 100 %.*Used Memory:[ ]+8,000 MiB of 16,000 MiB.*Used Swap:[ ]+1,000 MiB of 4,000 MiB.*Used Diskspace:[ ]+42 % on / \\|  85 % on /home.*SSH Keys:[ ]+#1 SHA256:abcdef1234567890 \\(ED25519 256\\).*#2 SHA256:0987654321fedcba \\(RSA 4096\\).*Network:.*eth0:[ ]+192\\.168\\.1\\.100 / 24.*2001:db8::1 / 64"
   LABELS "system-info;mock;deterministic"
)

# Test custom config file color application (-C)
ADD_SYSTEMINFO_TEST(systeminfo_mock_custom_config
   -C "${MOCK_CONFIG_FILE}"
   -S /nonexistent
   -X "${MOCK_DATE_STRING}"
   -Y "${MOCK_SYSTEM_INFO_FILE}"
)
SET_TESTS_PROPERTIES(systeminfo_mock_custom_config PROPERTIES
   PASS_REGULAR_EXPRESSION "${ESC}\\[31m.*Host:[ ]+nordmarka\\.nntb\\.no"
   LABELS "system-info;mock;config"
)

# === Deterministic RTL Tests (Arabic & Hebrew) ==============================
ADD_SYSTEMINFO_TEST(systeminfo_mock_deterministic_ar
   -S /nonexistent
   -X "${MOCK_DATE_STRING}"
   -Y "${MOCK_SYSTEM_INFO_FILE}"
)
SET_TESTS_PROPERTIES(systeminfo_mock_deterministic_ar PROPERTIES
   ENVIRONMENT "LC_ALL=en_US.UTF-8;LANGUAGE=ar;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "nordmarka\\.nntb\\.no.*Ubuntu.*192\\.168\\.1\\.100.*2001:db8::1"
   LABELS "system-info;mock;i18n;locale_ar;rtl"
)

ADD_SYSTEMINFO_TEST(systeminfo_mock_deterministic_he
   -S /nonexistent
   -X "${MOCK_DATE_STRING}"
   -Y "${MOCK_SYSTEM_INFO_FILE}"
)
SET_TESTS_PROPERTIES(systeminfo_mock_deterministic_he PROPERTIES
   ENVIRONMENT "LC_ALL=en_US.UTF-8;LANGUAGE=he;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "nordmarka\\.nntb\\.no.*Ubuntu.*192\\.168\\.1\\.100.*2001:db8::1"
   LABELS "system-info;mock;i18n;locale_he;rtl"
)

# === Error Handling & Validation ===========================================
ADD_SYSTEMINFO_FAIL_TEST(systeminfo_fail_config_not_found  -C /nonexistent/system-info.conf)
ADD_SYSTEMINFO_FAIL_TEST(systeminfo_fail_input_not_found   -Y /nonexistent/input.txt)
ADD_SYSTEMINFO_FAIL_TEST(systeminfo_fail_extra_arg         extra_argument)
ADD_SYSTEMINFO_FAIL_TEST(systeminfo_fail_unknown_flag      --invalid-flag)
