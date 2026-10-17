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
# Fingerprint-SSH-Keys Test Suite
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

# ===========================================================================
# Deterministic Mock SSH Host Public Keys
# ===========================================================================
SET(MOCK_SSH_DIR "${CMAKE_CURRENT_BINARY_DIR}/mock-ssh-keys")
FILE(MAKE_DIRECTORY "${MOCK_SSH_DIR}")

# Deterministic RFC 4253 / OpenSSH public key files:
# RFC 8032 Section 7.1 Test Vector 1 (valid Curve25519 point)
FILE(WRITE "${MOCK_SSH_DIR}/ssh_host_ed25519_key.pub"
"ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAINdamAGCsQq31Uv+08lkBzoO4XLz2qYjJa8CGmj3B1Ea root@nordmarka\n"
)
# Valid NIST P-256 base generator point G
FILE(WRITE "${MOCK_SSH_DIR}/ssh_host_ecdsa_key.pub"
"ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBGsX0fLhLEJH+Lzm5WOkQPJ3A32BLeszoPShOUXYmMKWT+NC4v4af5uO5+tKfA+eFivOM1drMV7Oy7ZAaDe/UfU= root@nordmarka\n"
)


#############################################################################
#### Fingerprint-SSH-Keys                                                ####
#############################################################################

# ###### Helper for deterministic commands (expected return code: 0) ########
FUNCTION(ADD_FINGERPRINT_SSH_KEYS_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${FINGERPRINT_SSH_KEYS_EXECUTABLE} ${ARGN}
   )
   SET(TEST_LABELS "fingerprint-ssh-keys;cli")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


# ###### Helper for expected failures (expected return code: non-zero) ######
FUNCTION(ADD_FINGERPRINT_SSH_KEYS_FAIL_TEST TEST_NAME)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${FINGERPRINT_SSH_KEYS_EXECUTABLE} ${ARGN}
   )
   SET(TEST_LABELS "fingerprint-ssh-keys;negative;error")
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "${TEST_LABELS}"
   )
ENDFUNCTION()


# --- Basic CLI Options -----------------------------------------------------
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_cli_version_short -v)
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_cli_version_long  --version)
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_cli_help_short    -h)
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_cli_help_long     --help)

SET_TESTS_PROPERTIES(fpsshkeys_cli_version_short fpsshkeys_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Fingerprint-SSH-Keys [0-9]+\\.[0-9]+"
)

SET_TESTS_PROPERTIES(fpsshkeys_cli_help_short fpsshkeys_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# --- CLI Help Internationalization -----------------------------------------
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_cli_help_i18n_de  -h)
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_cli_help_i18n_nb  -h)

SET_TESTS_PROPERTIES(fpsshkeys_cli_help_i18n_de PROPERTIES
   ENVIRONMENT "LANGUAGE=de;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Aufruf:"
   LABELS "fingerprint-ssh-keys;cli;i18n;locale_de"
)

SET_TESTS_PROPERTIES(fpsshkeys_cli_help_i18n_nb PROPERTIES
   ENVIRONMENT "LANGUAGE=nb;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Bruk:"
   LABELS "fingerprint-ssh-keys;cli;i18n;locale_nb"
)


# --- Deterministic Output Tests: Raw Public Keys (-R) ----------------------
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_mock_raw_text
   -a -R -K "${MOCK_SSH_DIR}" -F text
)
SET_TESTS_PROPERTIES(fpsshkeys_mock_raw_text PROPERTIES
   PASS_REGULAR_EXPRESSION "Fingerprints of .* in raw format:.*#1: ecdsa-sha2-nistp256 AAAAE2.*#2: ssh-ed25519 AAAAC3"
   LABELS "fingerprint-ssh-keys;mock;raw;text"
)

ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_mock_raw_csv
   -a -R -K "${MOCK_SSH_DIR}" -F csv
)
SET_TESTS_PROPERTIES(fpsshkeys_mock_raw_csv PROPERTIES
   PASS_REGULAR_EXPRESSION "ecdsa-sha2-nistp256,AAAAE2.*ssh-ed25519,AAAAC3"
   LABELS "fingerprint-ssh-keys;mock;raw;csv"
)

ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_mock_raw_python
   -a -R -K "${MOCK_SSH_DIR}" -F python
)
SET_TESTS_PROPERTIES(fpsshkeys_mock_raw_python PROPERTIES
   PASS_REGULAR_EXPRESSION "'public-keys':.*'ecdsa-sha2-nistp256':.*'AAAAE2.*'ssh-ed25519':.*'AAAAC3"
   LABELS "fingerprint-ssh-keys;mock;raw;python"
)


# --- Deterministic Output Tests: SSH Fingerprint Records (-S) --------------
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_mock_ssh_text
   -a -S -K "${MOCK_SSH_DIR}" -F text
)
SET_TESTS_PROPERTIES(fpsshkeys_mock_ssh_text PROPERTIES
   PASS_REGULAR_EXPRESSION "Fingerprints of .* in SSH format:.*#1: SHA256:.*\\(ECDSA 256\\).*#2: SHA256:.*\\(ED25519 256\\)"
   LABELS "fingerprint-ssh-keys;mock;ssh;text"
)

ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_mock_ssh_csv
   -a -S -K "${MOCK_SSH_DIR}" -F csv
)
SET_TESTS_PROPERTIES(fpsshkeys_mock_ssh_csv PROPERTIES
   PASS_REGULAR_EXPRESSION "ECDSA,256,SHA256:.*ED25519,256,SHA256:"
   LABELS "fingerprint-ssh-keys;mock;ssh;csv"
)

ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_mock_ssh_python
   -a -S -K "${MOCK_SSH_DIR}" -F python
)
SET_TESTS_PROPERTIES(fpsshkeys_mock_ssh_python PROPERTIES
   PASS_REGULAR_EXPRESSION "'ssh-fingerprints':.*'ECDSA':.*'bits':[ ]*256.*'fingerprint':[ ]*'SHA256:.*'ED25519':.*'bits':[ ]*256.*'fingerprint':[ ]*'SHA256:"
   LABELS "fingerprint-ssh-keys;mock;ssh;python"
)


# --- Deterministic Output Tests: DNS SSHFP Records (-D) --------------------
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_mock_dns_text
   -a -D -K "${MOCK_SSH_DIR}" -F text
)
SET_TESTS_PROPERTIES(fpsshkeys_mock_dns_text PROPERTIES
   PASS_REGULAR_EXPRESSION "Fingerprints of .* in DNS RR format:.*IN[ \t]+SSHFP[ \t]+[0-9]+[ \t]+[0-9]+[ \t]+[a-f0-9]+"
   LABELS "fingerprint-ssh-keys;mock;dns;text"
)

ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_mock_dns_csv
   -a -D -K "${MOCK_SSH_DIR}" -F csv
)
SET_TESTS_PROPERTIES(fpsshkeys_mock_dns_csv PROPERTIES
   PASS_REGULAR_EXPRESSION ".*,IN,SSHFP,[0-9]+,[0-9]+,[a-f0-9]+"
   LABELS "fingerprint-ssh-keys;mock;dns;csv"
)

ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_mock_dns_python
   -a -D -K "${MOCK_SSH_DIR}" -F python
)
SET_TESTS_PROPERTIES(fpsshkeys_mock_dns_python PROPERTIES
   PASS_REGULAR_EXPRESSION "'dns-records':.*'rr':[ ]*'SSHFP'.*'algorithm':[ ]*[0-9]+.*'hash_type':[ ]*[0-9]+.*'fingerprint':[ ]*'[a-f0-9]+'"
   LABELS "fingerprint-ssh-keys;mock;dns;python"
)


# --- Combined & All Flag Tests (-A, -a, --none) ----------------------------
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_mock_all_csv
   -A -K "${MOCK_SSH_DIR}" -F csv
)
SET_TESTS_PROPERTIES(fpsshkeys_mock_all_csv PROPERTIES
   PASS_REGULAR_EXPRESSION "ecdsa-sha2-nistp256,AAAAE2.*ECDSA,256,SHA256:.*IN,SSHFP"
   LABELS "fingerprint-ssh-keys;mock;all;csv"
)

# Test query none (-a / --none) produces completely empty output
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_mock_none_empty
   -a -K "${MOCK_SSH_DIR}"
)
SET_TESTS_PROPERTIES(fpsshkeys_mock_none_empty PROPERTIES
   FAIL_REGULAR_EXPRESSION "Fingerprints of|ssh-|SHA256:|SSHFP"
   LABELS "fingerprint-ssh-keys;mock;none"
)


# --- CLI Flag Variants & Case-Insensitivity --------------------------------
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_format_case_insensitive_csv
   -a -R -K "${MOCK_SSH_DIR}" --format CsV
)
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_format_case_insensitive_python
   -a -R -K "${MOCK_SSH_DIR}" -F PyThOn
)
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_legacy_raw_format
   -a -R -K "${MOCK_SSH_DIR}" --raw-format text
)
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_legacy_include_raw
   -a -K "${MOCK_SSH_DIR}" --include-raw
)
ADD_FINGERPRINT_SSH_KEYS_TEST(fpsshkeys_legacy_include_raw_python
   -a -K "${MOCK_SSH_DIR}" --include-raw-python
)

SET_TESTS_PROPERTIES(fpsshkeys_format_case_insensitive_csv PROPERTIES
   PASS_REGULAR_EXPRESSION "ssh-ed25519,AAAAC3"
)
SET_TESTS_PROPERTIES(fpsshkeys_format_case_insensitive_python PROPERTIES
   PASS_REGULAR_EXPRESSION "'public-keys':"
)
SET_TESTS_PROPERTIES(fpsshkeys_legacy_raw_format fpsshkeys_legacy_include_raw PROPERTIES
   PASS_REGULAR_EXPRESSION "Fingerprints of .* in raw format:"
)
SET_TESTS_PROPERTIES(fpsshkeys_legacy_include_raw_python PROPERTIES
   PASS_REGULAR_EXPRESSION "'public-keys':"
)


# --- Error Handling & Validation -------------------------------------------
ADD_FINGERPRINT_SSH_KEYS_FAIL_TEST(fpsshkeys_fail_invalid_format_xml   -F xml)
ADD_FINGERPRINT_SSH_KEYS_FAIL_TEST(fpsshkeys_fail_invalid_format_json  --format json)
ADD_FINGERPRINT_SSH_KEYS_FAIL_TEST(fpsshkeys_fail_nonexistent_key_dir  -K /nonexistent/ssh/directory)
ADD_FINGERPRINT_SSH_KEYS_FAIL_TEST(fpsshkeys_fail_extra_argument       extra_argument)
ADD_FINGERPRINT_SSH_KEYS_FAIL_TEST(fpsshkeys_fail_unknown_flag         --invalid-option)
ADD_FINGERPRINT_SSH_KEYS_FAIL_TEST(fpsshkeys_fail_missing_format_arg   -F)
ADD_FINGERPRINT_SSH_KEYS_FAIL_TEST(fpsshkeys_fail_missing_dir_arg      -K)
