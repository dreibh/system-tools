// ==========================================================================
//         ____            _                     _____           _
//        / ___| _   _ ___| |_ ___ _ __ ___     |_   _|__   ___ | |___
//        \___ \| | | / __| __/ _ \ '_ ` _ \ _____| |/ _ \ / _ \| / __|
//         ___) | |_| \__ \ ||  __/ | | | | |_____| | (_) | (_) | \__ \
//        |____/ \__, |___/\__\___|_| |_| |_|     |_|\___/ \___/|_|___/
//               |___/
//                             --- System-Tools ---
//                  https://www.nntb.no/~dreibh/system-tools/
// ==========================================================================
//
// X.509 Certificate Viewer
// Copyright (C) 2025-2026 by Thomas Dreibholz
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Contact: dreibh@simula.no

#include "x509-helpers.h"
#include "package-version.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <getopt.h>
#include <iostream>
#include <propertyformatter.h>
#include <string>
#include <vector>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/opensslv.h>
#include <openssl/pem.h>
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L) && !defined(LIBRESSL_VERSION_NUMBER)
#include <openssl/core_names.h>
#endif


static X509ColorSettings colors;


// ###### Format Public Key Algorithm (and key size / curve if available) ###
static std::string formatPublicKeyAlgorithm(const X509* cert)
{
   X509_PUBKEY* pubkey = X509_get_X509_PUBKEY(cert);
   if(pubkey == nullptr) {
      return "?";
   }

   ASN1_OBJECT* algObj = nullptr;
   if((X509_PUBKEY_get0_param(&algObj, nullptr, nullptr, nullptr, pubkey) != 1) ||
      (algObj == nullptr)) {
      return "?";
   }

   const int   nid      = OBJ_obj2nid(algObj);
   const char* longName = (nid != NID_undef) ? OBJ_nid2ln(nid) : nullptr;
   std::string result;
   if(longName != nullptr) {
      result = longName;
   }
   else {
      char oidBuf[128];
      OBJ_obj2txt(oidBuf, sizeof(oidBuf), algObj, 1);
      result = oidBuf;
   }

   EVP_PKEY* evpKey = X509_get0_pubkey(cert);
   if(evpKey != nullptr) {
#if defined(LIBRESSL_VERSION_NUMBER) && (LIBRESSL_VERSION_NUMBER < 0x3050000fL)
      const int bits = EVP_PKEY_bits(evpKey);
#else
      const int bits = EVP_PKEY_get_bits(evpKey);
#endif
      std::string groupName;
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L) && !defined(LIBRESSL_VERSION_NUMBER)
      char   groupBuf[128] { };
      size_t groupLen = 0;
      if((EVP_PKEY_get_utf8_string_param(evpKey, OSSL_PKEY_PARAM_GROUP_NAME,
                                         groupBuf, sizeof(groupBuf), &groupLen) == 1) &&
         (groupLen > 0)) {
         groupName.assign(groupBuf, groupLen);
      }
#endif
      if((bits > 0) && !groupName.empty()) {
         result += " (" + std::to_string(bits) + " bit, " + groupName + ")";
      }
      else if(bits > 0) {
         result += " (" + std::to_string(bits) + " bit)";
      }
      else if(!groupName.empty()) {
         result += " (" + groupName + ")";
      }
   }
   return result;
}


// ###### Format an X.509v3 certificate extension by NID ####################
static std::string formatCertExtensionByNid(const X509* cert, const int nid)
{
   const int index = X509_get_ext_by_NID(cert, nid, -1);
   if(index < 0) {
      return "(none)";
   }
   return formatX509Extension(X509_get_ext(cert, index), colors, true);
}


// ###### Parse PEM chain or DER certificate from memory buffer #############
static std::vector<X509Ptr> loadCertificates(const std::vector<uint8_t>& buffer)
{
   std::vector<X509Ptr> certificates;
   if(buffer.empty() || (buffer.size() > maxX509InputBytes)) {
      return certificates;
   }

   // 1. Try reading one or more PEM certificates (including TRUSTED CERTIFICATE):
   {
      BioPtr memBio(BIO_new_mem_buf(buffer.data(), static_cast<int>(buffer.size())), &BIO_free);
      if(memBio) {
         ERR_clear_error();
         while(true) {
            X509* rawCert = PEM_read_bio_X509_AUX(memBio.get(), nullptr, nullptr, nullptr);
            if(rawCert == nullptr) {
               const unsigned long err = ERR_peek_last_error();
               if((err == 0) || (ERR_GET_REASON(err) == PEM_R_NO_START_LINE)) {
                  ERR_clear_error();
               }
               else {
                  ERR_clear_error();
                  certificates.clear();
               }
               break;
            }
            certificates.emplace_back(rawCert, &X509_free);
         }
      }
   }

   if(!certificates.empty()) {
      return certificates;
   }

   // 2. Fallback: try reading binary DER certificate(s):
   {
      BioPtr memBio(BIO_new_mem_buf(buffer.data(), static_cast<int>(buffer.size())), &BIO_free);
      if(memBio) {
         ERR_clear_error();
         while(BIO_ctrl_pending(memBio.get()) > 0) {
            X509* rawCert = d2i_X509_bio(memBio.get(), nullptr);
            if(rawCert == nullptr) {
               ERR_clear_error();
               certificates.clear();
               break;
            }
            certificates.emplace_back(rawCert, &X509_free);
         }
      }
   }

   return certificates;
}


// ###### Display certificate details and hierarchy #########################
static bool displayCertificateFile(const std::string& certificateFile,
                                   const bool         isRtl)
{
   const std::string displayPath = (certificateFile == "-") ? "/dev/stdin" : certificateFile;

   if(certificateFile != "-") {
      std::error_code errorCode;
      if(!std::filesystem::exists(certificateFile, errorCode)) {
         // TRANSLATORS: Error message when certificate file cannot be found:
         // {0} = file path
         std::cerr << formatText("ERROR: Unable to find certificate file {0}!", certificateFile) << '\n';
         return false;
      }
   }

   std::vector<uint8_t> rawBytes;
   if(!readInputBytes(certificateFile, rawBytes)) {
      // TRANSLATORS: Error message when certificate file cannot be read:
      // {0} = file path
      std::cerr << formatText("ERROR: Unable to read certificate file {0}!", displayPath) << '\n';
      return false;
   }

   const std::vector<X509Ptr> certificates = loadCertificates(rawBytes);
   if(certificates.empty()) {
      // TRANSLATORS: Error message when input is not a valid PEM or DER X.509 certificate:
      // {0} = file path
      std::cerr << formatText("ERROR: Unable to parse X.509 certificate in {0}!", displayPath) << '\n';
      return false;
   }

   const X509* leafCert = certificates.front().get();

   const std::string subject            = formatX509Name(X509_get_subject_name(leafCert), colors);
   const std::string san                = formatCertExtensionByNid(leafCert, NID_subject_alt_name);
   const std::string validityFrom       = formatAsn1Time(X509_get0_notBefore(leafCert));
   const std::string validityTo         = formatAsn1Time(X509_get0_notAfter(leafCert));
   const std::string publicKeyAlgorithm = formatPublicKeyAlgorithm(leafCert);
   const std::string signatureAlgorithm = formatSignatureAlgorithm(X509_get0_tbs_sigalg(leafCert));
   const std::string basicConstraints   = formatCertExtensionByNid(leafCert, NID_basic_constraints);
   const std::string keyUsage           = formatCertExtensionByNid(leafCert, NID_key_usage);
   const std::string extKeyUsage        = formatCertExtensionByNid(leafCert, NID_ext_key_usage);
   const std::string subjectKeyId       = formatCertExtensionByNid(leafCert, NID_subject_key_identifier);
   const std::string authorityKeyId     = formatCertExtensionByNid(leafCert, NID_authority_key_identifier);

   // ====== Certificate details ============================================
   std::cout << colors.Header
             // TRANSLATORS: Header line for certificate properties:
             // {0} = certificate file path
             << formatText("Certificate in {0}:", displayPath)
             << colors.Reset << '\n';

   PropertyFormatter detailsFormatter("   ", { "", "", "" }, "", isRtl);

   // TRANSLATORS: Table row label for certificate Subject DN.
   detailsFormatter.add(colors.Label + gettext("Subject:"),
                        colorizeLines(subject, colors.Subject, colors.Reset));
   // TRANSLATORS: Table row label for Subject Alternative Name extension.
   detailsFormatter.add(colors.Label + gettext("Subject Alternative Name:"),
                        colorizeLines(san, colors.Subject, colors.Reset));
   // TRANSLATORS: Table row label for certificate validity period.
   detailsFormatter.add(colors.Label + gettext("Validity:"),
                        colors.Value + validityFrom + colors.Label + " — " + colors.Value + validityTo + colors.Reset);
   // TRANSLATORS: Table row label for public key algorithm.
   detailsFormatter.add(colors.Label + gettext("Public Key Algorithm:"),
                        colorizeLines(publicKeyAlgorithm, colors.Value, colors.Reset));
   // TRANSLATORS: Table row label for signature algorithm.
   detailsFormatter.add(colors.Label + gettext("Signature Algorithm:"),
                        colorizeLines(signatureAlgorithm, colors.Value, colors.Reset));
   // TRANSLATORS: Table row label for Basic Constraints extension.
   detailsFormatter.add(colors.Label + gettext("Basic Constraints:"),
                        colorizeLines(basicConstraints, colors.Label, colors.Reset));
   // TRANSLATORS: Table row label for Key Usage extension.
   detailsFormatter.add(colors.Label + gettext("Key Usage:"),
                        colorizeLines(keyUsage, colors.Label, colors.Reset));
   // TRANSLATORS: Table row label for Extended Key Usage extension.
   detailsFormatter.add(colors.Label + gettext("Extended Key Usage:"),
                        colorizeLines(extKeyUsage, colors.Label, colors.Reset));
   // TRANSLATORS: Table row label for Subject Key Identifier extension.
   detailsFormatter.add(colors.Label + gettext("Subject Key Identifier:"),
                        colorizeLines(subjectKeyId, colors.Label, colors.Reset));
   // TRANSLATORS: Table row label for Authority Key Identifier extension.
   detailsFormatter.add(colors.Label + gettext("Authority Key Identifier:"),
                        colorizeLines(authorityKeyId, colors.Label, colors.Reset));

   std::cout << detailsFormatter;

   // ====== Certificate hierarchy ==========================================
   std::cout << colors.Header
             // TRANSLATORS: Header line for certificate chain hierarchy:
             // {0} = certificate file path
             << formatText("Hierarchy in {0}:", displayPath)
             << colors.Reset << '\n';

   PropertyFormatter hierarchyFormatter(" ", { " " }, "", isRtl);
   for(size_t i = 0; i < certificates.size(); ++i) {
      const std::string  entrySubject = formatX509Name(X509_get_subject_name(certificates[i].get()), colors);
      const std::string& entryColor   = (i == 0) ? colors.Subject : colors.Issuer;

      // TRANSLATORS: Certificate chain entry index:
      // {0} = 1-based position in certificate hierarchy (#1.)
      hierarchyFormatter.add(colors.Label + formatText("#{0}.", i + 1),
                             colorizeLines(entrySubject, entryColor, colors.Reset));
   }

   std::cout << hierarchyFormatter;
   return true;
}


// ###### Version ############################################################
[[noreturn]] static void version()
{
   std::cout << "view-certificate " << PROJECT_VERSION << '\n';
   std::exit(0);
}


// ------ BEGIN-OF-USAGE-BLOCK ------
// ###### Usage #############################################################
#define HEADER(s)    "\x1b[4m" << (s) << "\x1b[0m"
#define CMD(s)       "\x1b[1m" << (s) << "\x1b[0m"
#define OPT(s)       "\x1b[1m" s "\x1b[0m"
#define ARG(s)       "\x1b[4m" s "\x1b[0m"
#define SYN(s)       "\x1b[1m" s "\x1b[0m"
#define PIPE         SYN(" | ")
#define ENTRY(s)     "\t" SYN("[") s SYN("]\n")

[[noreturn]]
static void usage(const char* program, const int exitCode)
{
   // TRANSLATORS: "LTR" stands for Left-to-Right layout direction.
   // Translators of Right-to-Left languages (such as Arabic, Hebrew, Persian)
   // must translate this string to "RTL".
   const bool rtl = (std::string(gettext("LTR")) == "RTL");
   const std::string manpage(std::string("\x1b[1m") +
                             std::filesystem::path(program).stem().string() +
                             std::string("\x1b[0m"));
   const std::string homepage((rtl ? std::string("\xe2\x80\x8e") : std::string("")) +
                              std::string("[\x1b[1m\x1b]8;;") +
                              std::string(PROJECT_HOMEPAGE_URL) +
                              "\a" + std::string(PROJECT_HOMEPAGE_URL) + "\a" +
                              std::string("\x1b]8;;\a\x1b[0m]") +
                              (rtl ? std::string("\xe2\x80\x8f") : std::string("")));
   const std::string repository((rtl ? std::string("\xe2\x80\x8e") : std::string("")) +
                                std::string("[\x1b[1m\x1b]8;;") +
                                std::string(PROJECT_SOURCE_URL) +
                                "\a" + std::string(PROJECT_SOURCE_URL) + "\a" +
                                std::string("\x1b]8;;\a\x1b[0m]") +
                                (rtl ? std::string("\xe2\x80\x8f") : std::string("")));

   // TRANSLATORS: Command-line syntax message prefix:
   // "Usage: <program> [options]".
   std::cerr << HEADER(gettext("Usage:")) << "\n"
             << CMD(program) << " " << ARG("certificate_file") << " ...\n"
                   ENTRY(OPT("-c") PIPE OPT("--no-color") PIPE OPT("--no-colour"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-h") PIPE OPT("--help"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-v") PIPE OPT("--version"))
             << "\n"
             // TRANSLATORS: Informational message at the end of usage:
             // {0} = manpage name, {1} = documentation URL, {2} = repository URL
             << formatText("For further information, see the man page for {0}, read the documentation at {1}, examine the source code at {2}, or consult your favourite LLM.",
                           manpage, homepage, repository) << "\n";
   std::exit(exitCode);
}
// ------ END-OF-USAGE-BLOCK ------


// ###### Main program #######################################################
int main(int argc, char** argv)
{
   initialiseLocaleAndI18n("view-certificate");

   static const struct option longOptions[] = {
      { "no-color",  no_argument, nullptr, 'c' },
      { "no-colour", no_argument, nullptr, 'c' },
      { "help",      no_argument, nullptr, 'h' },
      { "version",   no_argument, nullptr, 'v' },
      { nullptr,     0,           nullptr, 0   }
   };

   int option    { 0 };
   int longIndex { 0 };
   while((option = getopt_long(argc, argv, "chv", longOptions, &longIndex)) != -1) {
      switch(option) {
         case 'c':
            colors.disable();
            break;
         case 'h':
            usage(argv[0], 0);
            break;
         case 'v':
            version();
            break;
         default:
            usage(argv[0], 1);
            break;
      }
   }

   if(optind >= argc) {
      usage(argv[0], 1);
   }

   // TRANSLATORS: "LTR" stands for Left-to-Right layout direction.
   // Translators of Right-to-Left languages (such as Arabic, Hebrew, Persian)
   // must translate this string to "RTL".
   const bool isRtl = (std::strcmp(gettext("LTR"), "RTL") == 0);

   for(int i = optind; i < argc; ++i) {
      if(!displayCertificateFile(argv[i], isRtl)) {
         return 1;
      }
   }

   return 0;
}
