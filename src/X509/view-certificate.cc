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

#include <chrono>
#include <clocale>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <locale>
#include <memory>
#include <propertyformatter.h>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/opensslv.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L) && !defined(LIBRESSL_VERSION_NUMBER)
#include <openssl/core_names.h>
#endif

#if defined(ENABLE_NLS)
#include <libintl.h>
#else
#define gettext(string)                          (string)
#define ngettext(singular, plural, count)        (((count) == 1) ? (singular) : (plural))
#define bindtextdomain(domain, directory)        (directory)
#define textdomain(domain)                       (domain)
#define bind_textdomain_codeset(domain, codeset) (codeset)
#endif

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

#include "package-version.h"


// ====== RAII Deleters for OpenSSL Objects =================================
using BioPtr  = std::unique_ptr<BIO, decltype(&BIO_free)>;
using X509Ptr = std::unique_ptr<X509, decltype(&X509_free)>;

// Maximum input size (64 MiB) to guard against unbounded input and BIO int overflow:
static constexpr size_t maxCertificateBytes = 64ULL * 1024ULL * 1024ULL;


// ====== Struct for ANSI Color Settings ====================================
struct ColorSettings {
   std::string Header       { "\x1b[34m" };
   std::string Label        { "\x1b[33m" };
   std::string Subject      { "\x1b[35m" };
   std::string Value        { "\x1b[36m" };
   std::string Issuer       { "\x1b[37m" };
   std::string UnderlineOn  { "\x1b[4m"  };
   std::string UnderlineOff { "\x1b[24m" };
   std::string Reset        { "\x1b[0m"  };

   void disable() noexcept {
      Header.clear();
      Label.clear();
      Subject.clear();
      Value.clear();
      Issuer.clear();
      UnderlineOn.clear();
      UnderlineOff.clear();
      Reset.clear();
   }
};

static ColorSettings colors;


// ###### C++20 gettext runtime format helper ################################
template<typename... Args>
static std::string formatText(const char* messageIdentifier,
                              const Args&... arguments)
{
   try {
      return std::vformat(gettext(messageIdentifier),
                          std::make_format_args(arguments...));
   }
   catch(const std::format_error&) {
      return gettext(messageIdentifier);
   }
}


// ###### Trim trailing whitespace/newlines #################################
static std::string trimTrailingWhitespace(std::string_view sv)
{
   const auto last = sv.find_last_not_of(" \t\r\n");
   if(last == std::string_view::npos) {
      return "";
   }
   return std::string(sv.substr(0, last + 1));
}


// ###### Wrap each line of a (possibly multiline) value in ANSI colors #####
static std::string coloriseLines(const std::string& text,
                                 const std::string& color,
                                 const std::string& reset)
{
   if(color.empty() && reset.empty()) {
      return text;
   }

   std::istringstream stream(text);
   std::string        line;
   std::string        result;
   bool               first = true;

   while(std::getline(stream, line)) {
      if(!first) {
         result += '\n';
      }
      first = false;
      if(!line.empty()) {
         result += color;
         result += line;
         result += reset;
      }
   }
   return result;
}


// ###### Convert BIO memory buffer to std::string ##########################
static std::string bioToString(BIO* bio)
{
   char*      data   = nullptr;
   const long length = BIO_get_mem_data(bio, &data);
   if((data == nullptr) || (length <= 0)) {
      return "";
   }
   return trimTrailingWhitespace(std::string_view(data, static_cast<size_t>(length)));
}


// ###### Underline Common Name (CN = ...) in a Distinguished Name ##########
static std::string highlightCommonName(const std::string& dn)
{
   if(colors.UnderlineOn.empty()) {
      return dn;
   }

   std::string result;
   result.reserve(dn.size() + 16);

   size_t cursor = 0;
   while(cursor < dn.size()) {
      const size_t cnPos = dn.find("CN = ", cursor);
      if(cnPos == std::string::npos) {
         result.append(dn, cursor, std::string::npos);
         break;
      }

      const bool atBoundary =
         (cnPos == 0) ||
         (dn[cnPos - 1] == ' ') ||
         (dn[cnPos - 1] == ',') ||
         (dn[cnPos - 1] == '+') ||
         (dn[cnPos - 1] == '/');

      if(!atBoundary) {
         result.append(dn, cursor, (cnPos + 5) - cursor);
         cursor = cnPos + 5;
         continue;
      }

      result.append(dn, cursor, cnPos - cursor);

      size_t endPos   = cnPos + 5;
      bool   inQuotes = false;
      while(endPos < dn.size()) {
         if((dn[endPos] == '\\') && (endPos + 1 < dn.size())) {
            endPos += 2;
            continue;
         }
         if(dn[endPos] == '"') {
            inQuotes = !inQuotes;
            endPos++;
            continue;
         }
         if(!inQuotes) {
            if(dn[endPos] == ',') {
               break;
            }
            if((dn[endPos] == ' ') && (endPos + 2 < dn.size()) &&
               (dn[endPos + 1] == '+') && (dn[endPos + 2] == ' ')) {
               break;
            }
         }
         endPos++;
      }

      result += colors.UnderlineOn;
      result.append(dn, cnPos, endPos - cnPos);
      result += colors.UnderlineOff;
      cursor = endPos;
   }

   return result;
}


// ###### Format X509_NAME as human-readable UTF-8 string ###################
static std::string formatX509Name(const X509_NAME* name)
{
   if(name == nullptr) {
      return "?";
   }

   BioPtr memBio(BIO_new(BIO_s_mem()), &BIO_free);
   if(!memBio) {
      return "?";
   }

   const unsigned long flags = XN_FLAG_ONELINE & ~static_cast<unsigned long>(ASN1_STRFLGS_ESC_MSB);
   if(X509_NAME_print_ex(memBio.get(), name, 0, flags) < 0) {
      return "?";
   }

   const std::string dn = bioToString(memBio.get());
   return dn.empty() ? "?" : highlightCommonName(dn);
}


// ###### Format ASN1_TIME in current locale and local time zone ############
static std::string formatAsn1Time(const ASN1_TIME* asn1Time)
{
   if(asn1Time == nullptr) {
      return "?";
   }

   struct tm utcTm { };
   if(ASN1_TIME_to_tm(asn1Time, &utcTm) != 1) {
      return "?";
   }

   using namespace std::chrono;
   const year_month_day ymd {
      year{ utcTm.tm_year + 1900 },
      month{ static_cast<unsigned int>(utcTm.tm_mon + 1) },
      day{ static_cast<unsigned int>(utcTm.tm_mday) }
   };
   if(!ymd.ok()) {
      return "?";
   }

   const sys_days dayPoint = sys_days{ ymd };
   const int64_t  unixSecs =
      duration_cast<seconds>(dayPoint.time_since_epoch()).count() +
      (static_cast<int64_t>(utcTm.tm_hour) * 3600LL) +
      (static_cast<int64_t>(utcTm.tm_min)  * 60LL) +
      static_cast<int64_t>(utcTm.tm_sec);

   const std::time_t timestamp     = static_cast<std::time_t>(unixSecs);
   bool              haveLocalTime = false;
   struct tm         localTm { };

   if(static_cast<int64_t>(timestamp) == unixSecs) {
#if defined(_WIN32)
      haveLocalTime = (localtime_s(&localTm, &timestamp) == 0);
#else
      haveLocalTime = (localtime_r(&timestamp, &localTm) != nullptr);
#endif
   }

   char buffer[128];
   if(haveLocalTime) {
      if(std::strftime(buffer, sizeof(buffer), "%c %Z", &localTm) != 0) {
         return buffer;
      }
   }

   // Fallback to UTC formatting if date exceeds platform localtime range (e.g. 9999-12-31Z):
   utcTm.tm_wday = static_cast<int>(weekday{ dayPoint }.c_encoding());
   utcTm.tm_isdst = 0;
   if(std::strftime(buffer, sizeof(buffer), "%c UTC", &utcTm) != 0) {
      return buffer;
   }
   return "?";
}


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
      if(EVP_PKEY_get_utf8_string_param(evpKey, OSSL_PKEY_PARAM_GROUP_NAME,
                                        groupBuf, sizeof(groupBuf), &groupLen) == 1 &&
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


// ###### Format Signature Algorithm ########################################
static std::string formatSignatureAlgorithm(const X509* cert)
{
   const X509_ALGOR* sigAlg = X509_get0_tbs_sigalg(cert);
   if((sigAlg == nullptr) || (sigAlg->algorithm == nullptr)) {
      return "?";
   }

   const int nid = OBJ_obj2nid(sigAlg->algorithm);
   if(nid != NID_undef) {
      const char* longName = OBJ_nid2ln(nid);
      if(longName != nullptr) {
         return longName;
      }
   }

   char oidBuf[128];
   OBJ_obj2txt(oidBuf, sizeof(oidBuf), sigAlg->algorithm, 1);
   return oidBuf;
}


// ###### Format an X.509v3 extension by NID ################################
static std::string formatExtensionByNID(const X509* cert, const int nid)
{
   const int index = X509_get_ext_by_NID(cert, nid, -1);
   if(index < 0) {
      return "(none)";
   }

   const X509_EXTENSION* ext = X509_get_ext(cert, index);
   if(ext == nullptr) {
      return "(none)";
   }

   BioPtr memBio(BIO_new(BIO_s_mem()), &BIO_free);
   if(!memBio) {
      return "?";
   }

   if(X509V3_EXT_print(memBio.get(), const_cast<X509_EXTENSION*>(ext), X509V3_EXT_DEFAULT, 0) != 1) {
      const ASN1_OCTET_STRING* rawData = X509_EXTENSION_get_data(const_cast<X509_EXTENSION*>(ext));
      if(rawData != nullptr) {
         ASN1_STRING_print(memBio.get(), rawData);
      }
   }

   std::string value = bioToString(memBio.get());
   if(value.empty()) {
      value = "(none)";
   }

   if(X509_EXTENSION_get_critical(ext) != 0) {
      return colors.UnderlineOn + "critical:" + colors.UnderlineOff + " " + value;
   }
   return value;
}


// ###### Read stream into bounded memory buffer ############################
static bool readStreamBounded(std::istream&         inputStream,
                              std::vector<uint8_t>& buffer)
{
   buffer.clear();
   char chunk[16384];
   while(inputStream.read(chunk, sizeof(chunk)) || (inputStream.gcount() > 0)) {
      const size_t bytesRead = static_cast<size_t>(inputStream.gcount());
      if(buffer.size() + bytesRead > maxCertificateBytes) {
         return false;
      }
      buffer.insert(buffer.end(),
                    reinterpret_cast<const uint8_t*>(chunk),
                    reinterpret_cast<const uint8_t*>(chunk) + bytesRead);
      if(inputStream.eof()) {
         break;
      }
   }
   return !inputStream.bad();
}


// ###### Read file or stdin into memory buffer #############################
static bool readInputBytes(const std::string&    filePath,
                           std::vector<uint8_t>& buffer)
{
   if(filePath == "-") {
      return readStreamBounded(std::cin, buffer);
   }

   std::ifstream fileStream(std::filesystem::path(filePath), std::ios::binary);
   if(!fileStream.is_open()) {
      return false;
   }
   return readStreamBounded(fileStream, buffer);
}


// ###### Parse PEM chain or DER certificate from memory buffer #############
static std::vector<X509Ptr> loadCertificates(const std::vector<uint8_t>& buffer)
{
   std::vector<X509Ptr> certificates;
   if(buffer.empty() || (buffer.size() > maxCertificateBytes)) {
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
                  // Reject corrupted trailing PEM block in a multi-certificate bundle:
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

   const std::string subject            = formatX509Name(X509_get_subject_name(leafCert));
   const std::string san                = formatExtensionByNID(leafCert, NID_subject_alt_name);
   const std::string validityFrom       = formatAsn1Time(X509_get0_notBefore(leafCert));
   const std::string validityTo         = formatAsn1Time(X509_get0_notAfter(leafCert));
   const std::string publicKeyAlgorithm = formatPublicKeyAlgorithm(leafCert);
   const std::string signatureAlgorithm = formatSignatureAlgorithm(leafCert);
   const std::string basicConstraints   = formatExtensionByNID(leafCert, NID_basic_constraints);
   const std::string keyUsage           = formatExtensionByNID(leafCert, NID_key_usage);
   const std::string extKeyUsage        = formatExtensionByNID(leafCert, NID_ext_key_usage);
   const std::string subjectKeyId       = formatExtensionByNID(leafCert, NID_subject_key_identifier);
   const std::string authorityKeyId     = formatExtensionByNID(leafCert, NID_authority_key_identifier);

   // ====== Certificate details ============================================
   // TRANSLATORS: Header line for certificate properties:
   // {0} = certificate file path
   std::cout << colors.Header
             << formatText("Certificate in {0}:", displayPath)
             << colors.Reset << '\n';

   PropertyFormatter detailsFormatter("   ", { "", "", "" }, "", isRtl);

   // TRANSLATORS: Table row label for certificate Subject DN.
   detailsFormatter.add(colors.Label + gettext("Subject:"),
                        coloriseLines(subject, colors.Subject, colors.Reset));
   // TRANSLATORS: Table row label for Subject Alternative Name extension.
   detailsFormatter.add(colors.Label + gettext("Subject Alternative Name:"),
                        coloriseLines(san, colors.Subject, colors.Reset));
   // TRANSLATORS: Table row label for certificate validity period.
   detailsFormatter.add(colors.Label + gettext("Validity:"),
                        colors.Value + validityFrom + colors.Label + " — " + colors.Value + validityTo + colors.Reset);
   // TRANSLATORS: Table row label for public key algorithm.
   detailsFormatter.add(colors.Label + gettext("Public Key Algorithm:"),
                        coloriseLines(publicKeyAlgorithm, colors.Value, colors.Reset));
   // TRANSLATORS: Table row label for signature algorithm.
   detailsFormatter.add(colors.Label + gettext("Signature Algorithm:"),
                        coloriseLines(signatureAlgorithm, colors.Value, colors.Reset));
   // TRANSLATORS: Table row label for Basic Constraints extension.
   detailsFormatter.add(colors.Label + gettext("Basic Constraints:"),
                        coloriseLines(basicConstraints, colors.Label, colors.Reset));
   // TRANSLATORS: Table row label for Key Usage extension.
   detailsFormatter.add(colors.Label + gettext("Key Usage:"),
                        coloriseLines(keyUsage, colors.Label, colors.Reset));
   // TRANSLATORS: Table row label for Extended Key Usage extension.
   detailsFormatter.add(colors.Label + gettext("Extended Key Usage:"),
                        coloriseLines(extKeyUsage, colors.Label, colors.Reset));
   // TRANSLATORS: Table row label for Subject Key Identifier extension.
   detailsFormatter.add(colors.Label + gettext("Subject Key Identifier:"),
                        coloriseLines(subjectKeyId, colors.Label, colors.Reset));
   // TRANSLATORS: Table row label for Authority Key Identifier extension.
   detailsFormatter.add(colors.Label + gettext("Authority Key Identifier:"),
                        coloriseLines(authorityKeyId, colors.Label, colors.Reset));

   std::cout << detailsFormatter;

   // ====== Certificate hierarchy ==========================================
   // TRANSLATORS: Header line for certificate chain hierarchy:
   // {0} = certificate file path
   std::cout << colors.Header
             << formatText("Hierarchy in {0}:", displayPath)
             << colors.Reset << '\n';

   PropertyFormatter hierarchyFormatter(" ", { " " }, "", isRtl);
   for(size_t i = 0; i < certificates.size(); ++i) {
      const std::string  entrySubject = formatX509Name(X509_get_subject_name(certificates[i].get()));
      const std::string& entryColor   = (i == 0) ? colors.Subject : colors.Issuer;

      // TRANSLATORS: Certificate chain entry index:
      // {0} = 1-based position in certificate hierarchy (#1.)
      hierarchyFormatter.add(colors.Label + formatText("#{0}.", i + 1),
                             coloriseLines(entrySubject, entryColor, colors.Reset));
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

[[ noreturn ]]
static void usage(const char* program, const int exitCode)
{
   // TRANSLATORS: Text direction: LTR (Left-to-Right) or RTL (Right-to-Left)
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
             << CMD(program) << "\n"
                   ENTRY(OPT("-c") PIPE OPT("--no-color") PIPE OPT("--no-colour"))
                   "\t" ARG("certificate_file") " " "..." "\n"
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
   // ====== Configure UTF-8 Console and Binary Stdin on Windows ============
#if defined(_WIN32)
   SetConsoleCP(CP_UTF8);
   SetConsoleOutputCP(CP_UTF8);
   _setmode(_fileno(stdin), _O_BINARY);
#endif

   // ====== Initialise locale support ======================================
   if(std::setlocale(LC_ALL, "") == nullptr) {
      std::setlocale(LC_ALL, "C.UTF-8");
   }
   try {
      std::locale userLocale("");
      std::cout.imbue(userLocale);
      std::cerr.imbue(userLocale);
   }
   catch(const std::runtime_error&) {
      try {
         std::locale fallbackLocale("C.UTF-8");
         std::cout.imbue(fallbackLocale);
         std::cerr.imbue(fallbackLocale);
      }
      catch(const std::runtime_error&) {
         // Defaults to std::locale::classic() if C.UTF-8 is unavailable
      }
   }

   // ====== Initialise i18n support ========================================
#if defined(ENABLE_NLS)
#if !defined(_WIN32)
   const char* textdomaindir     = std::getenv("TEXTDOMAINDIR");
#else
   char*       textdomaindir     = nullptr;
   size_t      textdomaindirsize = 0;
   _dupenv_s(&textdomaindir, &textdomaindirsize, "TEXTDOMAINDIR");
#endif
   bindtextdomain("view-certificate",
                  (((textdomaindir) && (textdomaindir[0] != 0x00)) ?
                      textdomaindir : PROJECT_LOCALEDIR));
#if defined(_WIN32)
   if(textdomaindir) {
      free(textdomaindir);
   }
#endif
   bind_textdomain_codeset("view-certificate", "UTF-8");
   textdomain("view-certificate");
#endif

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
