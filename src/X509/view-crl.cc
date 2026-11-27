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
// X.509 CRL Viewer
// Copyright (C) 2026 by Thomas Dreibholz
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

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <getopt.h>
#include <iostream>
#include <propertyformatter.h>
#include <string>
#include <string_view>
#include <vector>

#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/pem.h>


static X509ColorSettings colors;


// ###### Format an X.509v2 CRL extension by NID ############################
static std::string formatCrlExtensionByNid(const X509_CRL* crl, const int nid)
{
   const int index = X509_CRL_get_ext_by_NID(crl, nid, -1);
   if(index < 0) {
      return "(none)";
   }
   return formatX509Extension(X509_CRL_get_ext(crl, index), colors, true);
}


// ###### Parse one or more PEM or DER CRLs from memory buffer ##############
static std::vector<X509CrlPtr> loadCrls(const std::vector<uint8_t>& buffer)
{
   std::vector<X509CrlPtr> crls;
   if(buffer.empty() || (buffer.size() > maxX509InputBytes)) {
      return crls;
   }

   // 1. Try reading one or more PEM CRLs:
   {
      BioPtr memBio(BIO_new_mem_buf(buffer.data(), static_cast<int>(buffer.size())), &BIO_free);
      if(memBio) {
         ERR_clear_error();
         while(true) {
            X509_CRL* rawCrl = PEM_read_bio_X509_CRL(memBio.get(), nullptr, nullptr, nullptr);
            if(rawCrl == nullptr) {
               const unsigned long err = ERR_peek_last_error();
               if((err == 0) || (ERR_GET_REASON(err) == PEM_R_NO_START_LINE)) {
                  ERR_clear_error();
               }
               else {
                  ERR_clear_error();
                  crls.clear();
               }
               break;
            }
            crls.emplace_back(rawCrl, &X509_CRL_free);
         }
      }
   }

   if(!crls.empty()) {
      return crls;
   }

   // 2. Fallback: try reading binary DER CRL(s):
   {
      BioPtr memBio(BIO_new_mem_buf(buffer.data(), static_cast<int>(buffer.size())), &BIO_free);
      if(memBio) {
         ERR_clear_error();
         while(BIO_ctrl_pending(memBio.get()) > 0) {
            X509_CRL* rawCrl = d2i_X509_CRL_bio(memBio.get(), nullptr);
            if(rawCrl == nullptr) {
               ERR_clear_error();
               crls.clear();
               break;
            }
            crls.emplace_back(rawCrl, &X509_CRL_free);
         }
      }
   }

   return crls;
}


// ###### Display a single parsed X509_CRL ##################################
static void displaySingleCrl(const X509_CRL*    crl,
                             const std::string& displayPath,
                             const bool         isRtl)
{
   const std::string issuer = formatX509Name(X509_CRL_get_issuer(crl), colors);

   const ASN1_TIME*  lastUpdateAsn1 = X509_CRL_get0_lastUpdate(crl);
   const ASN1_TIME*  nextUpdateAsn1 = X509_CRL_get0_nextUpdate(crl);
   const std::string validityFrom   = formatAsn1Time(lastUpdateAsn1);
   const std::string validityTo     = (nextUpdateAsn1 != nullptr) ? formatAsn1Time(nextUpdateAsn1) : "(none)";

   const X509_ALGOR* sigAlg = nullptr;
   X509_CRL_get0_signature(crl, nullptr, &sigAlg);
   const std::string signatureAlgorithm = formatSignatureAlgorithm(sigAlg);
   const std::string crlNumber          = formatCrlExtensionByNid(crl, NID_crl_number);
   const std::string authorityKeyId     = formatCrlExtensionByNid(crl, NID_authority_key_identifier);

   // ====== CRL details ====================================================
   std::cout << colors.Header
             // TRANSLATORS: Header line for CRL properties:
             // {0} = CRL file path
             << formatText("CRL in {0}:", displayPath)
             << colors.Reset << '\n';

   PropertyFormatter detailsFormatter("   ", { "", "", "" }, "", isRtl);

   // TRANSLATORS: Table row label for CRL Issuer DN.
   detailsFormatter.add(colors.Label + gettext("Issuer:"),
                        coloriseLines(issuer, colors.Subject, colors.Reset));
   // TRANSLATORS: Table row label for CRL validity period (Last Update to Next Update).
   detailsFormatter.add(colors.Label + gettext("Validity:"),
                        colors.Value + validityFrom + colors.Label + " — " + colors.Value + validityTo + colors.Reset);
   // TRANSLATORS: Table row label for signature algorithm.
   detailsFormatter.add(colors.Label + gettext("Signature Algorithm:"),
                        coloriseLines(signatureAlgorithm, colors.Value, colors.Reset));
   // TRANSLATORS: Table row label for CRL Number extension.
   detailsFormatter.add(colors.Label + gettext("CRL Number:"),
                        coloriseLines(crlNumber, colors.Label, colors.Reset));
   // TRANSLATORS: Table row label for Authority Key Identifier extension.
   detailsFormatter.add(colors.Label + gettext("Authority Key Identifier:"),
                        coloriseLines(authorityKeyId, colors.Label, colors.Reset));

   std::cout << detailsFormatter;

   // ====== Revoked certificates ===========================================
   STACK_OF(X509_REVOKED)* revokedStack = X509_CRL_get_REVOKED(const_cast<X509_CRL*>(crl));
   const int revokedCount = (revokedStack != nullptr) ? sk_X509_REVOKED_num(revokedStack) : 0;
   if(revokedCount <= 0) {
      return;
   }

   struct RevokedEntry {
      std::string         serialHex;
      const X509_REVOKED* revoked;
   };

   std::vector<RevokedEntry> entries;
   entries.reserve(static_cast<size_t>(revokedCount));

   BioPtr serialBio(BIO_new(BIO_s_mem()), &BIO_free);
   for(int i = 0; i < revokedCount; ++i) {
      const X509_REVOKED* rev = sk_X509_REVOKED_value(revokedStack, i);
      if(rev == nullptr) {
         continue;
      }
      entries.push_back(RevokedEntry{
         .serialHex = formatAsn1IntegerHex(X509_REVOKED_get0_serialNumber(rev), serialBio.get()),
         .revoked   = rev
      });
   }

   std::sort(entries.begin(), entries.end(),
             [](const RevokedEntry& a, const RevokedEntry& b) {
                return a.serialHex < b.serialHex;
             });

   // Cache localized templates once before formatting potentially large revocation lists:
   // TRANSLATORS: Revoked certificate entry index:
   // {0} = 1-based entry number (#1.)
   const std::string_view indexPattern = gettext("#{0}.");
   // TRANSLATORS: Revoked certificate entry details:
   // {0} = certificate serial number (hexadecimal)
   // {1} = localized revocation timestamp
   const std::string_view revokedPattern = gettext("Certificate {0} revoked on {1}");

   PropertyFormatter revocationsFormatter(" ", { " " }, "", isRtl);
   for(size_t i = 0; i < entries.size(); ++i) {
      const X509_REVOKED* rev            = entries[i].revoked;
      const std::string   revocationDate = formatAsn1Time(X509_REVOKED_get0_revocationDate(rev));
      std::string         entryValue     = formatTranslatedText(revokedPattern,
                                                                entries[i].serialHex,
                                                                revocationDate);

      const int reasonIndex = X509_REVOKED_get_ext_by_NID(rev, NID_crl_reason, -1);
      if(reasonIndex >= 0) {
         const std::string reason = formatX509Extension(X509_REVOKED_get_ext(rev, reasonIndex),
                                                        colors, false);
         if(!reason.empty() && (reason != "(none)") && (reason != "?")) {
            entryValue += " (" + reason + ")";
         }
      }

      revocationsFormatter.add(colors.Revoked + formatTranslatedText(indexPattern, i + 1),
                               coloriseLines(entryValue, colors.Revoked, colors.Reset));
   }

   std::cout << revocationsFormatter;
}


// ###### Display all CRLs in a file ########################################
static bool displayCrlFile(const std::string& crlFile,
                           const bool         isRtl)
{
   const std::string displayPath = (crlFile == "-") ? "/dev/stdin" : crlFile;

   if(crlFile != "-") {
      std::error_code errorCode;
      if(!std::filesystem::exists(crlFile, errorCode)) {
         // TRANSLATORS: Error message when CRL file cannot be found:
         // {0} = file path
         std::cerr << formatText("ERROR: Unable to find CRL file {0}!", crlFile) << '\n';
         return false;
      }
   }

   std::vector<uint8_t> rawBytes;
   if(!readInputBytes(crlFile, rawBytes)) {
      // TRANSLATORS: Error message when CRL file cannot be read:
      // {0} = file path
      std::cerr << formatText("ERROR: Unable to read CRL file {0}!", displayPath) << '\n';
      return false;
   }

   const std::vector<X509CrlPtr> crls = loadCrls(rawBytes);
   if(crls.empty()) {
      // TRANSLATORS: Error message when input is not a valid PEM or DER X.509 CRL:
      // {0} = file path
      std::cerr << formatText("ERROR: Unable to parse X.509 CRL in {0}!", displayPath) << '\n';
      return false;
   }

   for(const auto& crl : crls) {
      displaySingleCrl(crl.get(), displayPath, isRtl);
   }
   return true;
}


// ###### Version ############################################################
[[noreturn]] static void version()
{
   std::cout << "view-crl " << PROJECT_VERSION << '\n';
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
   const std::string manpage(std::string("\x1b[1m") +
                             std::filesystem::path(program).stem().string() +
                             std::string("\x1b[0m"));
   const std::string homepage(std::string("[\x1b[1m\x1b]8;;") +
                              std::string(PROJECT_HOMEPAGE_URL) +
                              "\a" + std::string(PROJECT_HOMEPAGE_URL) + "\a" +
                              std::string("\x1b]8;;\a\x1b[0m]"));
   const std::string repository(std::string("[\x1b[1m\x1b]8;;") +
                                std::string(PROJECT_SOURCE_URL) +
                                "\a" + std::string(PROJECT_SOURCE_URL) + "\a" +
                                std::string("\x1b]8;;\a\x1b[0m]"));

   std::cerr << HEADER("Usage:") << "\n"
             << CMD(program) << "\n"
                   ENTRY(OPT("-c") PIPE OPT("--no-color") PIPE OPT("--no-colour"))
                   "\t" ARG("crl_file") " " "..." "\n"
             << CMD(program) << "\n"
                   ENTRY(OPT("-h") PIPE OPT("--help"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-v") PIPE OPT("--version"))
             << "\n"
             << std::format("For further information, see the man page for {0}, read the documentation at {1}, examine the source code at {2}, or consult your favourite LLM.",
                            manpage, homepage, repository) << "\n";
   std::exit(exitCode);
}
// ------ END-OF-USAGE-BLOCK ------


// ###### Main program #######################################################
int main(int argc, char** argv)
{
   initialiseLocaleAndI18n("view-crl");

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
      if(!displayCrlFile(argv[i], isRtl)) {
         return 1;
      }
   }

   return 0;
}
