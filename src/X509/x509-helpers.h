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

#ifndef X509_HELPERS_H
#define X509_HELPERS_H

#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#if defined(ENABLE_NLS)
#include <libintl.h>
#else
#define gettext(string)                          (string)
#define ngettext(singular, plural, count)        (((count) == 1) ? (singular) : (plural))
#define bindtextdomain(domain, directory)        (directory)
#define textdomain(domain)                       (domain)
#define bind_textdomain_codeset(domain, codeset) (codeset)
#endif


// ====== RAII Deleters for OpenSSL Objects =================================
using BioPtr     = std::unique_ptr<BIO, decltype(&BIO_free)>;
using X509Ptr    = std::unique_ptr<X509, decltype(&X509_free)>;
using X509CrlPtr = std::unique_ptr<X509_CRL, decltype(&X509_CRL_free)>;

// Maximum input size (256 MiB) to support large CRLs while preventing
// unbounded allocations and signed int overflow in BIO_new_mem_buf():
inline constexpr size_t maxX509InputBytes = 256ULL * 1024ULL * 1024ULL;


// ====== Struct for ANSI Color Settings ====================================
struct X509ColorSettings {
   std::string Header       { "\x1b[34m" };
   std::string Label        { "\x1b[33m" };
   std::string Subject      { "\x1b[35m" };
   std::string Value        { "\x1b[36m" };
   std::string Issuer       { "\x1b[37m" };
   std::string Revoked      { "\x1b[31m" };
   std::string UnderlineOn  { "\x1b[4m"  };
   std::string UnderlineOff { "\x1b[24m" };
   std::string Reset        { "\x1b[0m"  };

   void disable() noexcept {
      Header.clear();
      Label.clear();
      Subject.clear();
      Value.clear();
      Issuer.clear();
      Revoked.clear();
      UnderlineOn.clear();
      UnderlineOff.clear();
      Reset.clear();
   }
};


// ###### Format an already-translated pattern with C++20 std::vformat ######
template<typename... Args>
inline std::string formatTranslatedText(std::string_view translatedPattern,
                                        const Args&...   arguments)
{
   try {
      return std::vformat(translatedPattern,
                          std::make_format_args(arguments...));
   }
   catch(const std::format_error&) {
      return std::string(translatedPattern);
   }
}


// ###### C++20 gettext runtime format helper ###############################
template<typename... Args>
inline std::string formatText(const char*    messageIdentifier,
                              const Args&... arguments)
{
   return formatTranslatedText(gettext(messageIdentifier), arguments...);
}


// ====== Shared X.509 Helper Functions =====================================
void initialiseLocaleAndI18n(const char* domain);

std::string trimTrailingWhitespace(std::string_view sv);
std::string coloriseLines(const std::string& text,
                          const std::string& color,
                          const std::string& reset);

std::string bioToString(BIO* bio);
std::string highlightCommonName(const std::string&       dn,
                                const X509ColorSettings& colors);
std::string formatX509Name(const X509_NAME*         name,
                           const X509ColorSettings& colors);
std::string formatAsn1Time(const ASN1_TIME* asn1Time);
std::string formatSignatureAlgorithm(const X509_ALGOR* sigAlg);
std::string formatX509Extension(const X509_EXTENSION*    ext,
                                const X509ColorSettings& colors,
                                bool                     includeCriticalPrefix = true);
std::string formatAsn1IntegerHex(const ASN1_INTEGER* asn1Int,
                                 BIO*                scratchBio = nullptr);

bool readInputBytes(const std::string&    filePath,
                    std::vector<uint8_t>& buffer,
                    size_t                maxBytes = maxX509InputBytes);

#endif // X509_HELPERS_H
