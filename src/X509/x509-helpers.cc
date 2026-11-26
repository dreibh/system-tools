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
// X.509 Common Helpers
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

#include <chrono>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>

#include <openssl/objects.h>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif


// ###### Initialise console UTF-8, locale, and gettext domain ##############
void initialiseLocaleAndI18n(const char* domain)
{
#if defined(_WIN32)
   SetConsoleCP(CP_UTF8);
   SetConsoleOutputCP(CP_UTF8);
   _setmode(_fileno(stdin), _O_BINARY);
#endif

   std::ios_base::sync_with_stdio(false);

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

#if defined(ENABLE_NLS)
#if !defined(_WIN32)
   const char* textdomaindir     = std::getenv("TEXTDOMAINDIR");
#else
   char*       textdomaindir     = nullptr;
   size_t      textdomaindirsize = 0;
   _dupenv_s(&textdomaindir, &textdomaindirsize, "TEXTDOMAINDIR");
#endif
   bindtextdomain(domain,
                  (((textdomaindir) && (textdomaindir[0] != 0x00)) ?
                      textdomaindir : PROJECT_LOCALEDIR));
#if defined(_WIN32)
   if(textdomaindir) {
      free(textdomaindir);
   }
#endif
   bind_textdomain_codeset(domain, "UTF-8");
   textdomain(domain);
#else
   (void)domain;
#endif
}


// ###### Trim trailing whitespace/newlines #################################
std::string trimTrailingWhitespace(std::string_view sv)
{
   const auto last = sv.find_last_not_of(" \t\r\n");
   if(last == std::string_view::npos) {
      return "";
   }
   return std::string(sv.substr(0, last + 1));
}


// ###### Wrap each line of a (possibly multiline) value in ANSI colors #####
std::string colorizeLines(const std::string& text,
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
std::string bioToString(BIO* bio)
{
   char*      data   = nullptr;
   const long length = BIO_get_mem_data(bio, &data);
   if((data == nullptr) || (length <= 0)) {
      return "";
   }
   return trimTrailingWhitespace(std::string_view(data, static_cast<size_t>(length)));
}


// ###### Underline Common Name (CN = ...) in a Distinguished Name ##########
std::string highlightCommonName(const std::string&       dn,
                                const X509ColorSettings& colors)
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
std::string formatX509Name(const X509_NAME*         name,
                           const X509ColorSettings& colors)
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
   return dn.empty() ? "?" : highlightCommonName(dn, colors);
}


// ###### Format ASN1_TIME in current locale and local time zone ############
std::string formatAsn1Time(const ASN1_TIME* asn1Time)
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
   utcTm.tm_wday  = static_cast<int>(weekday{ dayPoint }.c_encoding());
   utcTm.tm_isdst = 0;
   if(std::strftime(buffer, sizeof(buffer), "%c UTC", &utcTm) != 0) {
      return buffer;
   }
   return "?";
}


// ###### Format X509_ALGOR signature algorithm #############################
std::string formatSignatureAlgorithm(const X509_ALGOR* sigAlg)
{
   if(sigAlg == nullptr) {
      return "?";
   }

   const ASN1_OBJECT* algObj = nullptr;
   X509_ALGOR_get0(&algObj, nullptr, nullptr, sigAlg);
   if(algObj == nullptr) {
      return "?";
   }

   const int nid = OBJ_obj2nid(algObj);
   if(nid != NID_undef) {
      const char* longName = OBJ_nid2ln(nid);
      if(longName != nullptr) {
         return longName;
      }
   }

   char oidBuf[128];
   OBJ_obj2txt(oidBuf, sizeof(oidBuf), algObj, 1);
   return oidBuf;
}


// ###### Format an X509_EXTENSION value ####################################
std::string formatX509Extension(const X509_EXTENSION*    ext,
                                const X509ColorSettings& colors,
                                const bool               includeCriticalPrefix)
{
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

   if(includeCriticalPrefix && (X509_EXTENSION_get_critical(ext) != 0)) {
      return colors.UnderlineOn + "critical:" + colors.UnderlineOff + " " + value;
   }
   return value;
}


// ###### Format ASN1_INTEGER as uppercase hexadecimal string ###############
std::string formatAsn1IntegerHex(const ASN1_INTEGER* asn1Int,
                                 BIO*                scratchBio)
{
   if(asn1Int == nullptr) {
      return "?";
   }

   BioPtr ownedBio(nullptr, &BIO_free);
   BIO*   activeBio = scratchBio;
   if(activeBio != nullptr) {
      (void)BIO_reset(activeBio);
   }
   else {
      ownedBio.reset(BIO_new(BIO_s_mem()));
      activeBio = ownedBio.get();
   }
   if(activeBio == nullptr) {
      return "?";
   }

   if(i2a_ASN1_INTEGER(activeBio, asn1Int) <= 0) {
      return "?";
   }

   char*      data   = nullptr;
   const long length = BIO_get_mem_data(activeBio, &data);
   if((data == nullptr) || (length <= 0)) {
      return "?";
   }

   // Strip line-continuation wraps ("\\\n") emitted by i2a_ASN1_INTEGER on >35-byte values:
   std::string hex;
   hex.reserve(static_cast<size_t>(length));
   for(long i = 0; i < length; ++i) {
      const char ch = data[i];
      if((ch != '\\') && (ch != '\n') && (ch != '\r')) {
         hex += ch;
      }
   }
   return hex.empty() ? "?" : hex;
}


// ###### Read stream into bounded memory buffer ############################
static bool readStreamBounded(std::istream&         inputStream,
                              std::vector<uint8_t>& buffer,
                              const size_t          maxBytes)
{
   buffer.clear();
   char chunk[16384];
   while(inputStream.read(chunk, sizeof(chunk)) || (inputStream.gcount() > 0)) {
      const size_t bytesRead = static_cast<size_t>(inputStream.gcount());
      if(buffer.size() + bytesRead > maxBytes) {
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
bool readInputBytes(const std::string&    filePath,
                    std::vector<uint8_t>& buffer,
                    const size_t          maxBytes)
{
   if(filePath == "-") {
      return readStreamBounded(std::cin, buffer, maxBytes);
   }

   std::ifstream fileStream(std::filesystem::path(filePath), std::ios::binary);
   if(!fileStream.is_open()) {
      return false;
   }
   return readStreamBounded(fileStream, buffer, maxBytes);
}
