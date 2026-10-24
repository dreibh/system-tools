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
# X.509 Tools Test Suite
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

# Find required system tools
FIND_PROGRAM(BASH_EXECUTABLE bash REQUIRED)
FIND_PROGRAM(OPENSSL_EXECUTABLE openssl REQUIRED)

# Resolve paths to configured script binaries
MACRO(RESOLVE_SCRIPT_PATH VAR_NAME SCRIPT_NAME)
   IF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/../${SCRIPT_NAME}.versioned")
      SET(${VAR_NAME} "${CMAKE_CURRENT_BINARY_DIR}/../${SCRIPT_NAME}.versioned")
   ELSEIF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${SCRIPT_NAME}.versioned")
      SET(${VAR_NAME} "${CMAKE_CURRENT_BINARY_DIR}/${SCRIPT_NAME}.versioned")
   ELSEIF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/../${SCRIPT_NAME}")
      SET(${VAR_NAME} "${CMAKE_CURRENT_BINARY_DIR}/../${SCRIPT_NAME}")
   ELSEIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../${SCRIPT_NAME}")
      SET(${VAR_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/../${SCRIPT_NAME}")
   ELSE()
      SET(${VAR_NAME} "${SCRIPT_NAME}")
   ENDIF()
ENDMACRO()

RESOLVE_SCRIPT_PATH(CHECK_CERTIFICATE_BIN   check-certificate)
RESOLVE_SCRIPT_PATH(EXTRACT_PEM_BIN         extract-pem)
RESOLVE_SCRIPT_PATH(VIEW_CERTIFICATE_BIN    view-certificate)
RESOLVE_SCRIPT_PATH(VIEW_CRL_BIN            view-crl)
RESOLVE_SCRIPT_PATH(DER2PEM_BIN             der2pem)
RESOLVE_SCRIPT_PATH(PEM2DER_BIN             pem2der)
RESOLVE_SCRIPT_PATH(TEST_TLS_CONNECTION_BIN test-tls-connection)

# Construct runtime PATH so scripts can call companion tools (print-utf8, unixts2time, time2unixts, text-block)
SET(DEP_PATHS
   "${CMAKE_CURRENT_BINARY_DIR}/.."
   "${CMAKE_CURRENT_BINARY_DIR}/../../print-utf8"
   "${CMAKE_CURRENT_BINARY_DIR}/../../Unix-Timestamp"
   "${CMAKE_CURRENT_BINARY_DIR}/../../text-block"
   "${CMAKE_BINARY_DIR}/bin"
   "$ENV{PATH}"
)
IF(TARGET print-utf8)
   LIST(INSERT DEP_PATHS 0 "$<TARGET_FILE_DIR:print-utf8>")
ENDIF()
IF(TARGET unixts2time)
   LIST(INSERT DEP_PATHS 0 "$<TARGET_FILE_DIR:unixts2time>")
ENDIF()
IF(TARGET time2unixts)
   LIST(INSERT DEP_PATHS 0 "$<TARGET_FILE_DIR:time2unixts>")
ENDIF()
IF(TARGET text-block)
   LIST(INSERT DEP_PATHS 0 "$<TARGET_FILE_DIR:text-block>")
ENDIF()
STRING(REPLACE ";" ":" TEST_ENV_PATH "${DEP_PATHS}")


# ===========================================================================
# Deterministic Test Fixtures (PKI Hierarchy Generation)
# ===========================================================================

SET(PKI_DIR "${CMAKE_CURRENT_BINARY_DIR}/mock_pki")
FILE(MAKE_DIRECTORY "${PKI_DIR}")

SET(ROOT_KEY        "${PKI_DIR}/root_ca.key")
SET(ROOT_CERT       "${PKI_DIR}/root_ca.crt")
SET(INTER_KEY       "${PKI_DIR}/inter_ca.key")
SET(INTER_CSR       "${PKI_DIR}/inter_ca.csr")
SET(INTER_CERT      "${PKI_DIR}/inter_ca.crt")
SET(SERVER_KEY      "${PKI_DIR}/server.key")
SET(SERVER_CSR      "${PKI_DIR}/server.csr")
SET(SERVER_CERT     "${PKI_DIR}/server.crt")
SET(SERVER_DER      "${PKI_DIR}/server.der")
SET(CHAIN_CERT      "${PKI_DIR}/server_chain.crt")
SET(MULTI_BUNDLE    "${PKI_DIR}/bundle.pem")
SET(MOCK_CRL        "${PKI_DIR}/sample.crl")
SET(MOCK_CRL_DER    "${PKI_DIR}/sample.crl.der")
SET(MOCK_CRL_NO_EXT "${PKI_DIR}/crl_no_extension.pem")
SET(UNTRUSTED_KEY   "${PKI_DIR}/untrusted.key")
SET(UNTRUSTED_CERT  "${PKI_DIR}/untrusted.crt")

# OpenSSL configuration for extensions and non-interactive CSR/cert generation
SET(OPENSSL_CONFIG "${PKI_DIR}/openssl.cnf")
FILE(WRITE "${OPENSSL_CONFIG}"
"[ req ]\n"
"default_bits = 2048\n"
"default_md = sha256\n"
"distinguished_name = req_distinguished_name\n"
"[ req_distinguished_name ]\n"
"commonName = Common Name\n"
"organizationName = Organization Name\n"
"countryName = Country Name\n"
"[ ca ]\n"
"default_ca = CA_default\n"
"[ CA_default ]\n"
"dir = ${PKI_DIR}\n"
"database = ${PKI_DIR}/index.txt\n"
"crlnumber = ${PKI_DIR}/crlnumber\n"
"default_md = sha256\n"
"default_crl_days = 30\n"
"[ v3_ca ]\n"
"basicConstraints = critical,CA:TRUE\n"
"keyUsage = critical,cRLSign,keyCertSign\n"
"subjectKeyIdentifier = hash\n"
"authorityKeyIdentifier = keyid:always,issuer\n"
"[ v3_inter ]\n"
"basicConstraints = critical,CA:TRUE,pathlen:0\n"
"keyUsage = critical,cRLSign,keyCertSign\n"
"subjectKeyIdentifier = hash\n"
"authorityKeyIdentifier = keyid:always,issuer\n"
"[ v3_server ]\n"
"basicConstraints = CA:FALSE\n"
"keyUsage = critical,digitalSignature,keyEncipherment\n"
"extendedKeyUsage = serverAuth,clientAuth\n"
"subjectAltName = @alt_names\n"
"[ alt_names ]\n"
"DNS.1 = test.example.com\n"
"DNS.2 = localhost\n"
"IP.1 = 127.0.0.1\n"
)

# 1. Generate Root CA (Key + Self-Signed Certificate)
EXECUTE_PROCESS(
   COMMAND ${OPENSSL_EXECUTABLE} req -x509 -new -newkey rsa:2048 -nodes -days 365
      -keyout "${ROOT_KEY}" -out "${ROOT_CERT}"
      -subj "/CN=SystemTools Root CA/O=Test PKI/C=NO"
      -config "${OPENSSL_CONFIG}" -extensions v3_ca -batch
   ERROR_QUIET
)

# 2. Generate Intermediate CA (Key + CSR + Certificate signed by Root CA)
EXECUTE_PROCESS(
   COMMAND ${OPENSSL_EXECUTABLE} req -new -newkey rsa:2048 -nodes
      -keyout "${INTER_KEY}" -out "${INTER_CSR}"
      -subj "/CN=SystemTools Intermediate CA/O=Test PKI/C=NO"
      -config "${OPENSSL_CONFIG}" -batch
   ERROR_QUIET
)
EXECUTE_PROCESS(
   COMMAND ${OPENSSL_EXECUTABLE} x509 -req -in "${INTER_CSR}" -days 365
      -CA "${ROOT_CERT}" -CAkey "${ROOT_KEY}" -CAcreateserial -out "${INTER_CERT}"
      -extfile "${OPENSSL_CONFIG}" -extensions v3_inter
   ERROR_QUIET
)

# 3. Generate Server Certificate (Key + CSR + Certificate signed by Intermediate CA)
EXECUTE_PROCESS(
   COMMAND ${OPENSSL_EXECUTABLE} req -new -newkey rsa:2048 -nodes
      -keyout "${SERVER_KEY}" -out "${SERVER_CSR}"
      -subj "/CN=test.example.com/O=Test Server/C=NO"
      -config "${OPENSSL_CONFIG}" -batch
   ERROR_QUIET
)
EXECUTE_PROCESS(
   COMMAND ${OPENSSL_EXECUTABLE} x509 -req -in "${SERVER_CSR}" -days 365
      -CA "${INTER_CERT}" -CAkey "${INTER_KEY}" -CAcreateserial -out "${SERVER_CERT}"
      -extfile "${OPENSSL_CONFIG}" -extensions v3_server
   ERROR_QUIET
)

# Convert server certificate to DER binary format
EXECUTE_PROCESS(
   COMMAND ${OPENSSL_EXECUTABLE} x509 -in "${SERVER_CERT}" -outform DER -out "${SERVER_DER}"
   ERROR_QUIET
)

# 4. Generate Certificate Chain Bundle (Server + Intermediate + Root CA)
FILE(READ "${SERVER_CERT}" TMP_SERVER)
FILE(READ "${INTER_CERT}"  TMP_INTER)
FILE(READ "${ROOT_CERT}"   TMP_ROOT)
FILE(WRITE "${CHAIN_CERT}" "${TMP_SERVER}\n${TMP_INTER}\n${TMP_ROOT}\n")

# 5. Generate Multi-Entry PEM Bundle (Certificates + Keys + CSRs)
FILE(READ "${SERVER_KEY}" TMP_KEY)
FILE(READ "${SERVER_CSR}" TMP_CSR)
FILE(WRITE "${MULTI_BUNDLE}"
   "${TMP_SERVER}\n"
   "${TMP_KEY}\n"
   "${TMP_INTER}\n"
   "${TMP_CSR}\n"
   "${TMP_ROOT}\n"
)

# 6. Generate Certificate Revocation List (CRL) in PEM and DER formats
FILE(WRITE "${PKI_DIR}/index.txt" "")
FILE(WRITE "${PKI_DIR}/crlnumber" "01\n")
EXECUTE_PROCESS(
   COMMAND ${OPENSSL_EXECUTABLE} ca -config "${OPENSSL_CONFIG}" -gencrl
      -keyfile "${ROOT_KEY}" -cert "${ROOT_CERT}" -out "${MOCK_CRL}" -batch
   ERROR_QUIET
)
EXECUTE_PROCESS(
   COMMAND ${OPENSSL_EXECUTABLE} crl -in "${MOCK_CRL}" -outform DER -out "${MOCK_CRL_DER}"
   ERROR_QUIET
)

# Duplicate CRL without extension to test content-based type detection
CONFIGURE_FILE("${MOCK_CRL}" "${MOCK_CRL_NO_EXT}" COPYONLY)

# 7. Generate Untrusted Certificate (for verification rejection testing)
EXECUTE_PROCESS(
   COMMAND ${OPENSSL_EXECUTABLE} req -x509 -new -newkey rsa:2048 -nodes -days 30
      -keyout "${UNTRUSTED_KEY}" -out "${UNTRUSTED_CERT}"
      -subj "/CN=Untrusted Entity/O=Rogue Corp/C=NO"
      -config "${OPENSSL_CONFIG}" -batch
   ERROR_QUIET
)

# 8. Create PEM fixture with an unsupported block header
SET(MOCK_BAD_PEM_FILE "${CMAKE_CURRENT_BINARY_DIR}/mock_bad_type.pem")
FILE(WRITE "${MOCK_BAD_PEM_FILE}"
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEowIBAAKCAQEA0Y3...\n"
"-----END RSA PRIVATE KEY-----\n"
)


#############################################################################
#### Helper Functions                                                    ####
#############################################################################

# Helper for test execution with output regex matching
FUNCTION(ADD_X509_MATCH_TEST TEST_NAME BINARY)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${BASH_EXECUTABLE} "${BINARY}" ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      ENVIRONMENT "PATH=${TEST_ENV_PATH};LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "x509;cli"
   )
ENDFUNCTION()

# Helper for commands asserting non-zero exit code without matching specific output
FUNCTION(ADD_X509_FAIL_TEST TEST_NAME BINARY)
   ADD_TEST(
      NAME ${TEST_NAME}
      COMMAND ${BASH_EXECUTABLE} "${BINARY}" ${ARGN}
   )
   SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "PATH=${TEST_ENV_PATH};LC_ALL=C.UTF-8;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
      FAIL_REGULAR_EXPRESSION "INTERNAL ERROR"
      LABELS "x509;negative;error"
   )
ENDFUNCTION()


#############################################################################
#### 1. CLI Options & I18N for All Tools                                 ####
#############################################################################

# Test check-certificate version flag (short option -v)
ADD_X509_MATCH_TEST(checkcert_cli_version_short "${CHECK_CERTIFICATE_BIN}" -v)

# Test check-certificate version flag (long option --version)
ADD_X509_MATCH_TEST(checkcert_cli_version_long  "${CHECK_CERTIFICATE_BIN}" --version)
SET_TESTS_PROPERTIES(checkcert_cli_version_short checkcert_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "check-certificate (@PROJECT_VERSION_MAJOR@|[0-9]+)"
)

# Test check-certificate help flag (short option -h)
ADD_X509_MATCH_TEST(checkcert_cli_help_short "${CHECK_CERTIFICATE_BIN}" -h)

# Test check-certificate help flag (long option --help)
ADD_X509_MATCH_TEST(checkcert_cli_help_long  "${CHECK_CERTIFICATE_BIN}" --help)
SET_TESTS_PROPERTIES(checkcert_cli_help_short checkcert_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test check-certificate German localized help (-h)
ADD_X509_MATCH_TEST(checkcert_cli_help_i18n_de "${CHECK_CERTIFICATE_BIN}" -h)
SET_TESTS_PROPERTIES(checkcert_cli_help_i18n_de PROPERTIES
   ENVIRONMENT "PATH=${TEST_ENV_PATH};LANGUAGE=de;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Aufruf:"
   LABELS "x509;cli;i18n;locale_de"
)

# Test extract-pem version flag (short option -v)
ADD_X509_MATCH_TEST(extractpem_cli_version_short "${EXTRACT_PEM_BIN}" -v)

# Test extract-pem version flag (long option --version)
ADD_X509_MATCH_TEST(extractpem_cli_version_long  "${EXTRACT_PEM_BIN}" --version)
SET_TESTS_PROPERTIES(extractpem_cli_version_short extractpem_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "extract-pem (@PROJECT_VERSION_MAJOR@|[0-9]+)"
)

# Test extract-pem help flag (short option -h)
ADD_X509_MATCH_TEST(extractpem_cli_help_short "${EXTRACT_PEM_BIN}" -h)

# Test extract-pem help flag (long option --help)
ADD_X509_MATCH_TEST(extractpem_cli_help_long  "${EXTRACT_PEM_BIN}" --help)
SET_TESTS_PROPERTIES(extractpem_cli_help_short extractpem_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test extract-pem Norwegian localized help (-h)
ADD_X509_MATCH_TEST(extractpem_cli_help_i18n_nb "${EXTRACT_PEM_BIN}" -h)
SET_TESTS_PROPERTIES(extractpem_cli_help_i18n_nb PROPERTIES
   ENVIRONMENT "PATH=${TEST_ENV_PATH};LANGUAGE=nb;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Bruk:"
   LABELS "x509;cli;i18n;locale_nb"
)

# Test view-certificate version flag (short option -v)
ADD_X509_MATCH_TEST(viewcert_cli_version_short "${VIEW_CERTIFICATE_BIN}" -v)

# Test view-certificate version flag (long option --version)
ADD_X509_MATCH_TEST(viewcert_cli_version_long  "${VIEW_CERTIFICATE_BIN}" --version)
SET_TESTS_PROPERTIES(viewcert_cli_version_short viewcert_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "view-certificate (@PROJECT_VERSION_MAJOR@|[0-9]+)"
)

# Test view-certificate help flag (short option -h)
ADD_X509_MATCH_TEST(viewcert_cli_help_short "${VIEW_CERTIFICATE_BIN}" -h)

# Test view-certificate help flag (long option --help)
ADD_X509_MATCH_TEST(viewcert_cli_help_long  "${VIEW_CERTIFICATE_BIN}" --help)
SET_TESTS_PROPERTIES(viewcert_cli_help_short viewcert_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test view-crl version flag (short option -v)
ADD_X509_MATCH_TEST(viewcrl_cli_version_short "${VIEW_CRL_BIN}" -v)

# Test view-crl version flag (long option --version)
ADD_X509_MATCH_TEST(viewcrl_cli_version_long  "${VIEW_CRL_BIN}" --version)
SET_TESTS_PROPERTIES(viewcrl_cli_version_short viewcrl_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "view-crl (@PROJECT_VERSION_MAJOR@|[0-9]+)"
)

# Test view-crl help flag (short option -h)
ADD_X509_MATCH_TEST(viewcrl_cli_help_short "${VIEW_CRL_BIN}" -h)

# Test view-crl help flag (long option --help)
ADD_X509_MATCH_TEST(viewcrl_cli_help_long  "${VIEW_CRL_BIN}" --help)
SET_TESTS_PROPERTIES(viewcrl_cli_help_short viewcrl_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test pem2der version flag (short option -v)
ADD_X509_MATCH_TEST(pem2der_cli_version_short "${PEM2DER_BIN}" -v)

# Test pem2der version flag (long option --version)
ADD_X509_MATCH_TEST(pem2der_cli_version_long  "${PEM2DER_BIN}" --version)
SET_TESTS_PROPERTIES(pem2der_cli_version_short pem2der_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "pem2der (@PROJECT_VERSION_MAJOR@|[0-9]+)"
)

# Test pem2der help flag (short option -h)
ADD_X509_MATCH_TEST(pem2der_cli_help_short "${PEM2DER_BIN}" -h)

# Test pem2der help flag (long option --help)
ADD_X509_MATCH_TEST(pem2der_cli_help_long  "${PEM2DER_BIN}" --help)
SET_TESTS_PROPERTIES(pem2der_cli_help_short pem2der_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test der2pem version flag (short option -v)
ADD_X509_MATCH_TEST(der2pem_cli_version_short "${DER2PEM_BIN}" -v)

# Test der2pem version flag (long option --version)
ADD_X509_MATCH_TEST(der2pem_cli_version_long  "${DER2PEM_BIN}" --version)
SET_TESTS_PROPERTIES(der2pem_cli_version_short der2pem_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "der2pem (@PROJECT_VERSION_MAJOR@|[0-9]+)"
)

# Test der2pem help flag (short option -h)
ADD_X509_MATCH_TEST(der2pem_cli_help_short "${DER2PEM_BIN}" -h)

# Test der2pem help flag (long option --help)
ADD_X509_MATCH_TEST(der2pem_cli_help_long  "${DER2PEM_BIN}" --help)
SET_TESTS_PROPERTIES(der2pem_cli_help_short der2pem_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test test-tls-connection version flag (short option -v)
ADD_X509_MATCH_TEST(testtls_cli_version_short "${TEST_TLS_CONNECTION_BIN}" -v)

# Test test-tls-connection version flag (long option --version)
ADD_X509_MATCH_TEST(testtls_cli_version_long  "${TEST_TLS_CONNECTION_BIN}" --version)
SET_TESTS_PROPERTIES(testtls_cli_version_short testtls_cli_version_long PROPERTIES
   PASS_REGULAR_EXPRESSION "test-tls-connection (@PROJECT_VERSION_MAJOR@|[0-9]+)"
)

# Test test-tls-connection help flag (short option -h)
ADD_X509_MATCH_TEST(testtls_cli_help_short "${TEST_TLS_CONNECTION_BIN}" -h)

# Test test-tls-connection help flag (long option --help)
ADD_X509_MATCH_TEST(testtls_cli_help_long  "${TEST_TLS_CONNECTION_BIN}" --help)
SET_TESTS_PROPERTIES(testtls_cli_help_short testtls_cli_help_long PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test test-tls-connection German localized help (-h)
ADD_X509_MATCH_TEST(testtls_cli_help_i18n_de "${TEST_TLS_CONNECTION_BIN}" -h)
SET_TESTS_PROPERTIES(testtls_cli_help_i18n_de PROPERTIES
   ENVIRONMENT "PATH=${TEST_ENV_PATH};LANGUAGE=de;TEXTDOMAINDIR=${TEST_LOCALE_DIR}"
   PASS_REGULAR_EXPRESSION "Aufruf:"
   LABELS "x509;cli;i18n;locale_de"
)


#############################################################################
#### 2. extract-pem Block Extraction & Filtering                         ####
#############################################################################

SET(EXTRACT_WORKDIR "${CMAKE_CURRENT_BINARY_DIR}/extract_run")
FILE(MAKE_DIRECTORY "${EXTRACT_WORKDIR}")

# Test extracting all blocks from a multi-PEM bundle (-o)
ADD_X509_MATCH_TEST(extractpem_all_entries
   "${EXTRACT_PEM_BIN}" -o "${EXTRACT_WORKDIR}/entry-" "${MULTI_BUNDLE}"
)
SET_TESTS_PROPERTIES(extractpem_all_entries PROPERTIES
   PASS_REGULAR_EXPRESSION "Done: 3 certificates, 1 CSRs, 0 CRLs, 1 keys"
   LABELS "extract-pem;extraction"
)

# Test extracting only the first entry (-1)
ADD_X509_MATCH_TEST(extractpem_only_first_short
   "${EXTRACT_PEM_BIN}" -1 -o "${EXTRACT_WORKDIR}/first-" "${MULTI_BUNDLE}"
)
SET_TESTS_PROPERTIES(extractpem_only_first_short PROPERTIES
   PASS_REGULAR_EXPRESSION "first-1\\.crt"
   LABELS "extract-pem;filtering"
)

# Test extracting only the first entry with long option (--only-first-entry)
ADD_X509_MATCH_TEST(extractpem_only_first_long
   "${EXTRACT_PEM_BIN}" --only-first-entry -o "${EXTRACT_WORKDIR}/first_long-" "${MULTI_BUNDLE}"
)
SET_TESTS_PROPERTIES(extractpem_only_first_long PROPERTIES
   PASS_REGULAR_EXPRESSION "first_long-1\\.crt"
   LABELS "extract-pem;filtering"
)

# Test skipping the first entry (-f)
ADD_X509_MATCH_TEST(extractpem_skip_first
   "${EXTRACT_PEM_BIN}" -f -o "${EXTRACT_WORKDIR}/skip1-" "${MULTI_BUNDLE}"
)
SET_TESTS_PROPERTIES(extractpem_skip_first PROPERTIES
   PASS_REGULAR_EXPRESSION "SKIPPING.*skip1-1\\.crt"
   LABELS "extract-pem;filtering"
)

# Test skipping the last entry (-l)
ADD_X509_MATCH_TEST(extractpem_skip_last
   "${EXTRACT_PEM_BIN}" -l -o "${EXTRACT_WORKDIR}/skiplast-" "${MULTI_BUNDLE}"
)
SET_TESTS_PROPERTIES(extractpem_skip_last PROPERTIES
   PASS_REGULAR_EXPRESSION "SKIPPING.*skiplast-3\\.crt"
   LABELS "extract-pem;filtering"
)

# Test quiet mode suppressing extraction progress output (-q)
ADD_X509_MATCH_TEST(extractpem_quiet_mode
   "${EXTRACT_PEM_BIN}" -q -o "${EXTRACT_WORKDIR}/quiet-" "${MULTI_BUNDLE}"
)
SET_TESTS_PROPERTIES(extractpem_quiet_mode PROPERTIES
   FAIL_REGULAR_EXPRESSION "Extracting|Done:|=>"
   LABELS "extract-pem;quiet"
)

# Test error when PEM file contains an unhandled block type
ADD_X509_MATCH_TEST(extractpem_fail_unsupported_block_type
   "${EXTRACT_PEM_BIN}" "${MOCK_BAD_PEM_FILE}"
)
SET_TESTS_PROPERTIES(extractpem_fail_unsupported_block_type PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Unexpected type RSA PRIVATE KEY in"
   LABELS "extract-pem;negative"
)

# Test mutual exclusivity between -1 and -f
ADD_X509_MATCH_TEST(extractpem_fail_mutually_exclusive_1_f
   "${EXTRACT_PEM_BIN}" -1 -f "${MULTI_BUNDLE}"
)
SET_TESTS_PROPERTIES(extractpem_fail_mutually_exclusive_1_f PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: --only-first-entry and --skip-first-entry/--skip-last-entry are mutually exclusive!"
)

# Test mutual exclusivity between -1 and -l
ADD_X509_MATCH_TEST(extractpem_fail_mutually_exclusive_1_l
   "${EXTRACT_PEM_BIN}" -1 -l "${MULTI_BUNDLE}"
)
SET_TESTS_PROPERTIES(extractpem_fail_mutually_exclusive_1_l PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: --only-first-entry and --skip-first-entry/--skip-last-entry are mutually exclusive!"
)


#############################################################################
#### 3. pem2der and der2pem Format Conversions                           ####
#############################################################################

SET(CONV_DIR "${CMAKE_CURRENT_BINARY_DIR}/conv_run")
FILE(MAKE_DIRECTORY "${CONV_DIR}")

SET(CONV_CERT_DER "${CONV_DIR}/converted_cert.der")
SET(CONV_CERT_PEM "${CONV_DIR}/converted_cert.pem")
SET(CONV_CRL_DER  "${CONV_DIR}/converted_crl.der.crl")
SET(CONV_CRL_PEM  "${CONV_DIR}/converted_crl.pem")
SET(TEST_OUT_CRL_DER "${CONV_DIR}/detected_crl.der")

# Test converting PEM certificate to DER format using pem2der
ADD_TEST(
   NAME pem2der_convert_certificate
   COMMAND ${BASH_EXECUTABLE} "${PEM2DER_BIN}" "${SERVER_CERT}" "${CONV_CERT_DER}"
)
SET_TESTS_PROPERTIES(pem2der_convert_certificate PROPERTIES
   ENVIRONMENT "PATH=${TEST_ENV_PATH};LC_ALL=C.UTF-8"
   FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
   LABELS "pem2der;conversion"
)

# Test converting DER certificate to PEM format using der2pem
ADD_TEST(
   NAME der2pem_convert_certificate
   COMMAND ${BASH_EXECUTABLE} "${DER2PEM_BIN}" "${CONV_CERT_DER}" "${CONV_CERT_PEM}"
)
SET_TESTS_PROPERTIES(der2pem_convert_certificate PROPERTIES
   DEPENDS pem2der_convert_certificate
   ENVIRONMENT "PATH=${TEST_ENV_PATH};LC_ALL=C.UTF-8"
   FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
   LABELS "der2pem;conversion"
)

# Test certificate PEM -> DER -> PEM round-trip comparison
ADD_TEST(
   NAME x509_roundtrip_certificate_diff
   COMMAND ${CMAKE_COMMAND} -E compare_files "${SERVER_CERT}" "${CONV_CERT_PEM}"
)
SET_TESTS_PROPERTIES(x509_roundtrip_certificate_diff PROPERTIES
   DEPENDS der2pem_convert_certificate
   LABELS "x509;roundtrip"
)

# Test converting PEM CRL to DER format using pem2der (with .crl extension)
ADD_TEST(
   NAME pem2der_convert_crl
   COMMAND ${BASH_EXECUTABLE} "${PEM2DER_BIN}" "${MOCK_CRL}" "${CONV_CRL_DER}"
)
SET_TESTS_PROPERTIES(pem2der_convert_crl PROPERTIES
   ENVIRONMENT "PATH=${TEST_ENV_PATH};LC_ALL=C.UTF-8"
   FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
   LABELS "pem2der;crl;conversion"
)

# Test converting DER CRL to PEM format using der2pem
ADD_TEST(
   NAME der2pem_convert_crl
   COMMAND ${BASH_EXECUTABLE} "${DER2PEM_BIN}" "${CONV_CRL_DER}" "${CONV_CRL_PEM}"
)
SET_TESTS_PROPERTIES(der2pem_convert_crl PROPERTIES
   DEPENDS pem2der_convert_crl
   ENVIRONMENT "PATH=${TEST_ENV_PATH};LC_ALL=C.UTF-8"
   FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
   LABELS "der2pem;crl;conversion"
)

# Test CRL PEM -> DER -> PEM round-trip comparison
ADD_TEST(
   NAME x509_roundtrip_crl_diff
   COMMAND ${CMAKE_COMMAND} -E compare_files "${MOCK_CRL}" "${CONV_CRL_PEM}"
)
SET_TESTS_PROPERTIES(x509_roundtrip_crl_diff PROPERTIES
   DEPENDS der2pem_convert_crl
   LABELS "x509;roundtrip;crl"
)

# Test pem2der correctly detecting X509 CRL content without a .crl filename extension
ADD_TEST(
   NAME pem2der_detect_crl_by_content
   COMMAND ${BASH_EXECUTABLE} "${PEM2DER_BIN}" "${MOCK_CRL_NO_EXT}" "${TEST_OUT_CRL_DER}"
)
SET_TESTS_PROPERTIES(pem2der_detect_crl_by_content PROPERTIES
   ENVIRONMENT "PATH=${TEST_ENV_PATH};LC_ALL=C.UTF-8"
   FAIL_REGULAR_EXPRESSION "INTERNAL ERROR;ERROR:"
   LABELS "pem2der;crl"
)


#############################################################################
#### 4. view-certificate Format & Details Validation                     ####
#############################################################################

# Test viewing a standard PEM certificate
ADD_X509_MATCH_TEST(viewcert_pem_server "${VIEW_CERTIFICATE_BIN}" "${SERVER_CERT}")
SET_TESTS_PROPERTIES(viewcert_pem_server PROPERTIES
   PASS_REGULAR_EXPRESSION "Subject:.*test\\.example\\.com.*Subject Alternative Name:.*DNS:localhost"
   LABELS "view-certificate;pem"
)

# Test viewing a binary DER certificate
ADD_X509_MATCH_TEST(viewcert_der_server "${VIEW_CERTIFICATE_BIN}" "${SERVER_DER}")
SET_TESTS_PROPERTIES(viewcert_der_server PROPERTIES
   PASS_REGULAR_EXPRESSION "Subject:.*test\\.example\\.com.*Public Key Algorithm:.*rsaEncryption"
   LABELS "view-certificate;der"
)

# Test viewing a bundled certificate chain showing hierarchy
ADD_X509_MATCH_TEST(viewcert_chain_hierarchy "${VIEW_CERTIFICATE_BIN}" "${CHAIN_CERT}")
SET_TESTS_PROPERTIES(viewcert_chain_hierarchy PROPERTIES
   PASS_REGULAR_EXPRESSION "#1\\..*test\\.example\\.com.*#2\\..*SystemTools Intermediate CA.*#3\\..*SystemTools Root CA"
   LABELS "view-certificate;chain"
)

# Test viewing a certificate streamed via standard input placeholder (-)
ADD_TEST(
   NAME viewcert_stdin_pipeline
   COMMAND ${BASH_EXECUTABLE} -c "\"${VIEW_CERTIFICATE_BIN}\" - < \"${SERVER_CERT}\""
)
SET_TESTS_PROPERTIES(viewcert_stdin_pipeline PROPERTIES
   ENVIRONMENT "PATH=${TEST_ENV_PATH};LC_ALL=C.UTF-8"
   PASS_REGULAR_EXPRESSION "Certificate in (/dev/stdin|.*view-certificate.*):.*Subject:.*test\\.example\\.com"
   LABELS "view-certificate;stdin"
)


#############################################################################
#### 5. view-crl Format & Revocation List Display                        ####
#############################################################################

# Test viewing a PEM-encoded CRL file
ADD_X509_MATCH_TEST(viewcrl_pem_display "${VIEW_CRL_BIN}" "${MOCK_CRL}")
SET_TESTS_PROPERTIES(viewcrl_pem_display PROPERTIES
   PASS_REGULAR_EXPRESSION "CRL in.*sample\\.crl:.*Issuer:.*SystemTools Root CA"
   LABELS "view-crl;pem"
)

# Test viewing a binary DER-encoded CRL file
ADD_X509_MATCH_TEST(viewcrl_der_display "${VIEW_CRL_BIN}" "${MOCK_CRL_DER}")
SET_TESTS_PROPERTIES(viewcrl_der_display PROPERTIES
   PASS_REGULAR_EXPRESSION "Issuer:.*SystemTools Root CA"
   LABELS "view-crl;der"
)

# Test viewing a CRL streamed via standard input placeholder (-)
ADD_TEST(
   NAME viewcrl_stdin_pipeline
   COMMAND ${BASH_EXECUTABLE} -c "\"${VIEW_CRL_BIN}\" - < \"${MOCK_CRL}\""
)
SET_TESTS_PROPERTIES(viewcrl_stdin_pipeline PROPERTIES
   ENVIRONMENT "PATH=${TEST_ENV_PATH};LC_ALL=C.UTF-8"
   PASS_REGULAR_EXPRESSION "Issuer:.*SystemTools Root CA"
   LABELS "view-crl;stdin"
)


#############################################################################
#### 6. check-certificate Hierarchy Verification                         ####
#############################################################################

# Test checking self-signed Root CA certificate (triggers self-signature warning)
ADD_X509_MATCH_TEST(checkcert_self_signed_root
   "${CHECK_CERTIFICATE_BIN}" -n "${ROOT_CERT}" "${ROOT_CERT}"
)
SET_TESTS_PROPERTIES(checkcert_self_signed_root PROPERTIES
   PASS_REGULAR_EXPRESSION "WARNING: Only checking self-signature!.*OpenSSL:.*OKAY"
   LABELS "check-certificate;verify"
)

# Test verifying Intermediate CA with Root CA
ADD_X509_MATCH_TEST(checkcert_verify_intermediate
   "${CHECK_CERTIFICATE_BIN}" -n "${ROOT_CERT}" "${INTER_CERT}"
)
SET_TESTS_PROPERTIES(checkcert_verify_intermediate PROPERTIES
   PASS_REGULAR_EXPRESSION "OpenSSL:.*OKAY"
   LABELS "check-certificate;verify"
)

# Test verifying bundled certificate chain with Root CA
ADD_X509_MATCH_TEST(checkcert_verify_chain_bundled
   "${CHECK_CERTIFICATE_BIN}" -n "${ROOT_CERT}" "${CHAIN_CERT}"
)
SET_TESTS_PROPERTIES(checkcert_verify_chain_bundled PROPERTIES
   PASS_REGULAR_EXPRESSION "OpenSSL:.*OKAY"
   LABELS "check-certificate;verify"
)

# Test verifying certificate with explicit CRL file (-C)
ADD_X509_MATCH_TEST(checkcert_verify_with_crl_short
   "${CHECK_CERTIFICATE_BIN}" -n -C "${MOCK_CRL}" "${ROOT_CERT}" "${INTER_CERT}"
)
SET_TESTS_PROPERTIES(checkcert_verify_with_crl_short PROPERTIES
   PASS_REGULAR_EXPRESSION "OpenSSL:.*OKAY"
   LABELS "check-certificate;crl"
)

# Test verifying certificate with long CRL option (--crl)
ADD_X509_MATCH_TEST(checkcert_verify_with_crl_long
   "${CHECK_CERTIFICATE_BIN}" -n --crl "${MOCK_CRL}" "${ROOT_CERT}" "${INTER_CERT}"
)
SET_TESTS_PROPERTIES(checkcert_verify_with_crl_long PROPERTIES
   PASS_REGULAR_EXPRESSION "OpenSSL:.*OKAY"
   LABELS "check-certificate;crl"
)

# Test verification failure when certificate is not signed by specified CA
ADD_X509_MATCH_TEST(checkcert_fail_untrusted_certificate
   "${CHECK_CERTIFICATE_BIN}" -n "${ROOT_CERT}" "${UNTRUSTED_CERT}"
)
SET_TESTS_PROPERTIES(checkcert_fail_untrusted_certificate PROPERTIES
   PASS_REGULAR_EXPRESSION "CERTIFICATE VERIFICATIONS FAILED!"
   LABELS "check-certificate;security"
)


#############################################################################
#### 7. test-tls-connection Tests                                        ####
#############################################################################

# ====== 7.1 Offline Parameter Handling & Connection Failure ================

# Test failure connecting to a closed/unreachable local port (fails gracefully without hang)
ADD_X509_FAIL_TEST(testtls_connect_unreachable_port
   "${TEST_TLS_CONNECTION_BIN}" -n -c "127.0.0.1:1" "${ROOT_CERT}"
)
SET_TESTS_PROPERTIES(testtls_connect_unreachable_port PROPERTIES
   LABELS "test-tls-connection;network;negative"
)

# Test saving certificate option with invalid endpoint (-s)
ADD_X509_FAIL_TEST(testtls_save_cert_option_unreachable
   "${TEST_TLS_CONNECTION_BIN}" -s "${CONV_DIR}/saved.crt" -n -c "127.0.0.1:1" "${ROOT_CERT}"
)
SET_TESTS_PROPERTIES(testtls_save_cert_option_unreachable PROPERTIES
   LABELS "test-tls-connection;network;negative"
)


# ====== 7.2 Live Network Tests (www.ietf.org & www.heise.de) ==============

# Search for system CA bundle / root certificate to verify public servers
SET(SYSTEM_CA_CERT "")
FOREACH(CA_CANDIDATE
   "/etc/ssl/certs/ca-certificates.crt"
   "/etc/ssl/cert.pem"
   "/etc/pki/tls/certs/ca-bundle.crt"
   "/etc/ssl/ca-bundle.pem"
   "/usr/share/ca-certificates/mozilla/ISRG_Root_X1.crt"
   "/etc/ssl/certs/ISRG_Root_X1.pem"
   "/usr/local/share/certs/ca-root-nss.crt"
   "/opt/homebrew/etc/ca-certificates/cert.pem"
   )
   IF(EXISTS "${CA_CANDIDATE}")
      SET(SYSTEM_CA_CERT "${CA_CANDIDATE}")
      BREAK()
   ENDIF()
ENDFOREACH()

# Fallback to test PKI Root CA if no system store is installed
IF(NOT SYSTEM_CA_CERT)
   SET(SYSTEM_CA_CERT "${ROOT_CERT}")
ENDIF()

# Regex patterns to detect missing network connectivity across platforms
SET(NETWORK_SKIP_REGEX
   "Temporary failure in name resolution|Name or service not known|nodename nor servname provided|No address associated with hostname|Could not resolve|Connection refused|Network is unreachable|No route to host|Operation timed out|Connection timed out|connect:errno=|Minimum number of 1 action"
)

SET(ONLINE_CERTS_DIR "${CMAKE_CURRENT_BINARY_DIR}/online_certs")
FILE(MAKE_DIRECTORY "${ONLINE_CERTS_DIR}")

# 1. Test downloading and verifying certificate from www.ietf.org:443
ADD_X509_MATCH_TEST(testtls_online_ietf_verify
   "${TEST_TLS_CONNECTION_BIN}" -n "www.ietf.org:443" "${SYSTEM_CA_CERT}"
)
SET_TESTS_PROPERTIES(testtls_online_ietf_verify PROPERTIES
   PASS_REGULAR_EXPRESSION "Connecting to www\\.ietf\\.org:443"
   SKIP_REGULAR_EXPRESSION "${NETWORK_SKIP_REGEX}"
   TIMEOUT 30
   LABELS "test-tls-connection;network;online"
)

# 2. Test downloading and verifying certificate from www.heise.de:443
ADD_X509_MATCH_TEST(testtls_online_heise_verify
   "${TEST_TLS_CONNECTION_BIN}" -n "www.heise.de:443" "${SYSTEM_CA_CERT}"
)
SET_TESTS_PROPERTIES(testtls_online_heise_verify PROPERTIES
   PASS_REGULAR_EXPRESSION "Connecting to www\\.heise\\.de:443"
   SKIP_REGULAR_EXPRESSION "${NETWORK_SKIP_REGEX}"
   TIMEOUT 30
   LABELS "test-tls-connection;network;online"
)

# 3. Test default port fallback (automatic :443 append) on www.ietf.org
ADD_X509_MATCH_TEST(testtls_online_ietf_default_port
   "${TEST_TLS_CONNECTION_BIN}" -n -c "www.ietf.org" "${SYSTEM_CA_CERT}"
)
SET_TESTS_PROPERTIES(testtls_online_ietf_default_port PROPERTIES
   PASS_REGULAR_EXPRESSION "Connecting to www\\.ietf\\.org:443"
   SKIP_REGULAR_EXPRESSION "${NETWORK_SKIP_REGEX}"
   TIMEOUT 30
   LABELS "test-tls-connection;network;online"
)

# 4. Test default port fallback (automatic :443 append) on www.heise.de
ADD_X509_MATCH_TEST(testtls_online_heise_default_port
   "${TEST_TLS_CONNECTION_BIN}" -n -c "www.heise.de" "${SYSTEM_CA_CERT}"
)
SET_TESTS_PROPERTIES(testtls_online_heise_default_port PROPERTIES
   PASS_REGULAR_EXPRESSION "Connecting to www\\.heise\\.de:443"
   SKIP_REGULAR_EXPRESSION "${NETWORK_SKIP_REGEX}"
   TIMEOUT 30
   LABELS "test-tls-connection;network;online"
)

# 5. Test downloading and saving certificate to file (-s) from www.ietf.org
SET(IETF_SAVED_CERT "${ONLINE_CERTS_DIR}/www_ietf_org.crt")
ADD_X509_MATCH_TEST(testtls_online_ietf_save_cert
   "${TEST_TLS_CONNECTION_BIN}" -s "${IETF_SAVED_CERT}" -n -c "www.ietf.org:443" "${SYSTEM_CA_CERT}"
)
SET_TESTS_PROPERTIES(testtls_online_ietf_save_cert PROPERTIES
   PASS_REGULAR_EXPRESSION "Connecting to www\\.ietf\\.org:443"
   SKIP_REGULAR_EXPRESSION "${NETWORK_SKIP_REGEX}"
   TIMEOUT 30
   LABELS "test-tls-connection;network;online"
)

# 6. Test downloading and saving certificate using long option (--save-certificate) from www.heise.de
SET(HEISE_SAVED_CERT "${ONLINE_CERTS_DIR}/www_heise_de.crt")
ADD_X509_MATCH_TEST(testtls_online_heise_save_cert
   "${TEST_TLS_CONNECTION_BIN}" --save-certificate "${HEISE_SAVED_CERT}" -n -c "www.heise.de:443" "${SYSTEM_CA_CERT}"
)
SET_TESTS_PROPERTIES(testtls_online_heise_save_cert PROPERTIES
   PASS_REGULAR_EXPRESSION "Connecting to www\\.heise\\.de:443"
   SKIP_REGULAR_EXPRESSION "${NETWORK_SKIP_REGEX}"
   TIMEOUT 30
   LABELS "test-tls-connection;network;online"
)

# 7. Test view-certificate integration pipeline with live downloaded certificate from www.ietf.org
ADD_X509_MATCH_TEST(testtls_online_ietf_view
   "${TEST_TLS_CONNECTION_BIN}" -c "www.ietf.org:443" "${SYSTEM_CA_CERT}"
)
SET_TESTS_PROPERTIES(testtls_online_ietf_view PROPERTIES
   PASS_REGULAR_EXPRESSION "Connecting to www\\.ietf\\.org:443.*Subject:.*ietf\\.org"
   SKIP_REGULAR_EXPRESSION "${NETWORK_SKIP_REGEX}"
   TIMEOUT 30
   LABELS "test-tls-connection;network;online"
)

# 8. Test view-certificate integration pipeline with live downloaded certificate from www.heise.de
ADD_X509_MATCH_TEST(testtls_online_heise_view
   "${TEST_TLS_CONNECTION_BIN}" -c "www.heise.de:443" "${SYSTEM_CA_CERT}"
)
SET_TESTS_PROPERTIES(testtls_online_heise_view PROPERTIES
   PASS_REGULAR_EXPRESSION "Connecting to www\\.heise\\.de:443.*Subject:.*heise\\.de"
   SKIP_REGULAR_EXPRESSION "${NETWORK_SKIP_REGEX}"
   TIMEOUT 30
   LABELS "test-tls-connection;network;online"
)


#############################################################################
#### 8. Error Handling & Negative Boundaries Across All Tools            ####
#############################################################################

# ------ Missing Positional Arguments ---------------------------------------
# Test check-certificate with no arguments
ADD_X509_MATCH_TEST(checkcert_fail_no_args "${CHECK_CERTIFICATE_BIN}")
SET_TESTS_PROPERTIES(checkcert_fail_no_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test extract-pem with no arguments
ADD_X509_MATCH_TEST(extractpem_fail_no_args "${EXTRACT_PEM_BIN}")
SET_TESTS_PROPERTIES(extractpem_fail_no_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test view-certificate with no arguments
ADD_X509_MATCH_TEST(viewcert_fail_no_args "${VIEW_CERTIFICATE_BIN}")
SET_TESTS_PROPERTIES(viewcert_fail_no_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test view-crl with no arguments
ADD_X509_MATCH_TEST(viewcrl_fail_no_args "${VIEW_CRL_BIN}")
SET_TESTS_PROPERTIES(viewcrl_fail_no_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test pem2der with no arguments
ADD_X509_MATCH_TEST(pem2der_fail_no_args "${PEM2DER_BIN}")
SET_TESTS_PROPERTIES(pem2der_fail_no_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test pem2der with only 1 argument
ADD_X509_MATCH_TEST(pem2der_fail_one_arg "${PEM2DER_BIN}" "${SERVER_CERT}")
SET_TESTS_PROPERTIES(pem2der_fail_one_arg PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test pem2der with 3 arguments (too many)
ADD_X509_MATCH_TEST(pem2der_fail_three_args "${PEM2DER_BIN}" "${SERVER_CERT}" out.der extra)
SET_TESTS_PROPERTIES(pem2der_fail_three_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test der2pem with no arguments
ADD_X509_MATCH_TEST(der2pem_fail_no_args "${DER2PEM_BIN}")
SET_TESTS_PROPERTIES(der2pem_fail_no_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test der2pem with only 1 argument
ADD_X509_MATCH_TEST(der2pem_fail_one_arg "${DER2PEM_BIN}" "${SERVER_DER}")
SET_TESTS_PROPERTIES(der2pem_fail_one_arg PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test der2pem with 3 arguments (too many)
ADD_X509_MATCH_TEST(der2pem_fail_three_args "${DER2PEM_BIN}" "${SERVER_DER}" out.pem extra)
SET_TESTS_PROPERTIES(der2pem_fail_three_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test test-tls-connection with no arguments
ADD_X509_MATCH_TEST(testtls_fail_no_args "${TEST_TLS_CONNECTION_BIN}")
SET_TESTS_PROPERTIES(testtls_fail_no_args PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# Test test-tls-connection with only 1 argument
ADD_X509_MATCH_TEST(testtls_fail_one_arg "${TEST_TLS_CONNECTION_BIN}" "localhost:443")
SET_TESTS_PROPERTIES(testtls_fail_one_arg PROPERTIES
   PASS_REGULAR_EXPRESSION "Usage:"
)

# ------ Non-Existent Input Files -------------------------------------------
# Test check-certificate with non-existent CA file
ADD_X509_MATCH_TEST(checkcert_fail_nonexistent_ca
   "${CHECK_CERTIFICATE_BIN}" "/nonexistent/ca.crt" "${SERVER_CERT}"
)
SET_TESTS_PROPERTIES(checkcert_fail_nonexistent_ca PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Unable to find CA certificate file /nonexistent/ca\\.crt!"
)

# Test check-certificate with non-existent certificate file
ADD_X509_MATCH_TEST(checkcert_fail_nonexistent_cert
   "${CHECK_CERTIFICATE_BIN}" "${ROOT_CERT}" "/nonexistent/cert.crt"
)
SET_TESTS_PROPERTIES(checkcert_fail_nonexistent_cert PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Unable to find certificate file /nonexistent/cert\\.crt!"
)

# Test check-certificate with non-existent CRL file
ADD_X509_MATCH_TEST(checkcert_fail_nonexistent_crl
   "${CHECK_CERTIFICATE_BIN}" -C "/nonexistent/crl.crl" "${ROOT_CERT}" "${SERVER_CERT}"
)
SET_TESTS_PROPERTIES(checkcert_fail_nonexistent_crl PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Unable to find CRL file /nonexistent/crl\\.crl!"
)

# Test extract-pem with non-existent input file
ADD_X509_MATCH_TEST(extractpem_fail_nonexistent_file
   "${EXTRACT_PEM_BIN}" "/nonexistent/file.pem"
)
SET_TESTS_PROPERTIES(extractpem_fail_nonexistent_file PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Unable to find PEM file /nonexistent/file\\.pem!"
)

# Test view-certificate with non-existent input file
ADD_X509_MATCH_TEST(viewcert_fail_nonexistent_file
   "${VIEW_CERTIFICATE_BIN}" "/nonexistent/cert.pem"
)
SET_TESTS_PROPERTIES(viewcert_fail_nonexistent_file PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Unable to find certificate file /nonexistent/cert\\.pem!"
)

# Test view-crl with non-existent input file
ADD_X509_MATCH_TEST(viewcrl_fail_nonexistent_file
   "${VIEW_CRL_BIN}" "/nonexistent/crl.pem"
)
SET_TESTS_PROPERTIES(viewcrl_fail_nonexistent_file PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Unable to find CRL file /nonexistent/crl\\.pem!"
)

# Test pem2der with non-existent input file
ADD_X509_MATCH_TEST(pem2der_fail_nonexistent_file
   "${PEM2DER_BIN}" "/nonexistent/file.pem" out.der
)
SET_TESTS_PROPERTIES(pem2der_fail_nonexistent_file PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: PEM file /nonexistent/file\\.pem does not exist!"
)

# Test der2pem with non-existent input file
ADD_X509_MATCH_TEST(der2pem_fail_nonexistent_file
   "${DER2PEM_BIN}" "/nonexistent/file.der" out.pem
)
SET_TESTS_PROPERTIES(der2pem_fail_nonexistent_file PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: DER file /nonexistent/file\\.der does not exist!"
)

# Test test-tls-connection with non-existent CA certificate file
ADD_X509_MATCH_TEST(testtls_fail_nonexistent_ca
   "${TEST_TLS_CONNECTION_BIN}" "localhost:443" "/nonexistent/ca.crt"
)
SET_TESTS_PROPERTIES(testtls_fail_nonexistent_ca PROPERTIES
   PASS_REGULAR_EXPRESSION "ERROR: Unable to find CA certificate file /nonexistent/ca\\.crt!"
)

# ------ Missing Option Arguments -------------------------------------------
# Test check-certificate with missing -C argument
ADD_X509_FAIL_TEST(checkcert_fail_missing_opt_C "${CHECK_CERTIFICATE_BIN}" -C)

# Test extract-pem with missing -o argument
ADD_X509_FAIL_TEST(extractpem_fail_missing_opt_o "${EXTRACT_PEM_BIN}" -o)

# Test test-tls-connection with missing -s argument
ADD_X509_FAIL_TEST(testtls_fail_missing_opt_s "${TEST_TLS_CONNECTION_BIN}" -s)

# ------ Unrecognized Options -----------------------------------------------
# Test check-certificate with unknown option -z
ADD_X509_FAIL_TEST(checkcert_fail_unknown_opt    "${CHECK_CERTIFICATE_BIN}" -z)

# Test extract-pem with unknown option -z
ADD_X509_FAIL_TEST(extractpem_fail_unknown_opt   "${EXTRACT_PEM_BIN}" -z "${MULTI_BUNDLE}")

# Test view-certificate with unknown option -z
ADD_X509_FAIL_TEST(viewcert_fail_unknown_opt     "${VIEW_CERTIFICATE_BIN}" -z "${SERVER_CERT}")

# Test view-crl with unknown option -z
ADD_X509_FAIL_TEST(viewcrl_fail_unknown_opt      "${VIEW_CRL_BIN}" -z "${MOCK_CRL}")

# Test pem2der with unknown option -z
ADD_X509_FAIL_TEST(pem2der_fail_unknown_opt "${PEM2DER_BIN}" -z "${SERVER_CERT}" out.der)

# Test der2pem with unknown option -z
ADD_X509_FAIL_TEST(der2pem_fail_unknown_opt "${DER2PEM_BIN}" -z "${SERVER_DER}" out.pem)

# Test test-tls-connection with unknown option -z
ADD_X509_FAIL_TEST(testtls_fail_unknown_opt "${TEST_TLS_CONNECTION_BIN}" -z "localhost:443" "${ROOT_CERT}")
