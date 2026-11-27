// ==========================================================================
//         ____            _                     _____           _
//        / ___| _   _ ___| |_ ___ _ __ ___     |_   _|__   ___ | |___
//        \___ \| | | / __| __/ _ \ '_ ` _ \ _____| |/ _ \ / _ \| / __|
//         ___) | |_| \__ \ ||  __/ | | | | |_____| | (_) | (_) | \__ \.
//        |____/ \__, |___/\__\___|_| |_| |_|     |_|\___/ \___/|_|___/
//               |___/
//                             --- System-Tools ---
//                  https://www.nntb.no/~dreibh/system-tools/
// ==========================================================================
//
// Get-System-Info
// Copyright (C) 2024-2026 by Thomas Dreibholz
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
// Contact: thomas.dreibholz@gmail.com

#include <cctype>
#include <cerrno>
#include <clocale>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <getopt.h>
#include <iostream>
#include <libsysteminfo.h>
#include <locale>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
#include <winsock2.h>
#endif

#include "package-version.h"


// ###### Locale helper to disable the thousands separator ##################
struct no_grouping : std::numpunct<char> {
   explicit no_grouping(const std::locale& base)
      : m_dp(std::use_facet<std::numpunct<char>>(base).decimal_point()),
        m_truename(std::use_facet<std::numpunct<char>>(base).truename()),
        m_falsename(std::use_facet<std::numpunct<char>>(base).falsename()) {}

protected:
   char do_decimal_point()    const override { return m_dp;        }
   std::string do_grouping()  const override {
      // Disables thousands separators
      return "";
   }
   std::string do_truename()  const override { return m_truename;  }
   std::string do_falsename() const override { return m_falsename; }

private:
   char        m_dp;
   std::string m_truename;
   std::string m_falsename;
};

// Helper to construct a new locale replacing only the numpunct<char> facet:
inline std::locale disableThousandsSeparator(const std::locale& base)
{
   return std::locale(base, new no_grouping(base));
}


// ###### Parse non-negative unsigned integer argument ######################
static bool parseUnsignedVersion(const char* argument, unsigned long& value) noexcept
{
   if((argument == nullptr) || (argument[0] == '\0') ||
      !std::isdigit(static_cast<unsigned char>(argument[0]))) {
      return false;
   }

   char* endptr = nullptr;
   errno        = 0;
   const unsigned long parsed = std::strtoul(argument, &endptr, 10);
   if((errno == ERANGE) || (endptr == argument) || (*endptr != '\0')) {
      return false;
   }

   value = parsed;
   return true;
}


// ###### Version ###########################################################
[[noreturn]] static void version()
{
   std::cout << "get-system-info " << PROJECT_VERSION << '\n';
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
                   ENTRY(OPT("-c") " " ARG("version") PIPE OPT("--compatibility") " " ARG("version"))
                   ENTRY(OPT("-A") PIPE OPT("--all"))
                   ENTRY(OPT("-a") PIPE OPT("--none"))
                   ENTRY(OPT("-B") PIPE OPT("--with-battery"))
                   ENTRY(OPT("-b") PIPE OPT("--without-battery"))
                   ENTRY(OPT("-E") PIPE OPT("--with-os"))
                   ENTRY(OPT("-e") PIPE OPT("--without-os"))
                   ENTRY(OPT("-F") PIPE OPT("--with-ssh"))
                   ENTRY(OPT("-f") PIPE OPT("--without-ssh"))
                   ENTRY(OPT("-K") PIPE OPT("--with-kernel"))
                   ENTRY(OPT("-k") PIPE OPT("--without-kernel"))
                   ENTRY(OPT("-L") PIPE OPT("--with-load"))
                   ENTRY(OPT("-l") PIPE OPT("--without-load"))
                   ENTRY(OPT("-M") PIPE OPT("--with-memory"))
                   ENTRY(OPT("-m") PIPE OPT("--without-memory"))
                   ENTRY(OPT("-N") PIPE OPT("--with-network"))
                   ENTRY(OPT("-n") PIPE OPT("--without-network"))
                   ENTRY(OPT("-O") PIPE OPT("--with-hostname"))
                   ENTRY(OPT("-o") PIPE OPT("--without-hostname"))
                   ENTRY(OPT("-S") PIPE OPT("--with-storage"))
                   ENTRY(OPT("-s") PIPE OPT("--without-storage"))
                   ENTRY(OPT("-T") PIPE OPT("--with-uptime"))
                   ENTRY(OPT("-t") PIPE OPT("--without-uptime"))
                   ENTRY(OPT("-U") PIPE OPT("--with-user"))
                   ENTRY(OPT("-u") PIPE OPT("--without-user"))
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


// ###### Main program ######################################################
int main(int argc, char** argv)
{
   // ====== Initialise locale support ======================================
   if(std::setlocale(LC_ALL, "") == nullptr) {
      std::setlocale(LC_ALL, "C.UTF-8");
   }
   try {
      const std::locale userLocale = disableThousandsSeparator(std::locale(""));
      std::cout.imbue(userLocale);
      std::cerr.imbue(userLocale);
   } catch(const std::runtime_error&) {
      try {
         const std::locale fallbackLocale = disableThousandsSeparator(std::locale("C.UTF-8"));
         std::cout.imbue(fallbackLocale);
         std::cerr.imbue(fallbackLocale);
      } catch(const std::runtime_error&) {
         // Defaults to std::locale::classic() if C.UTF-8 is unavailable
      }
   }

   // ====== Handle arguments ===============================================
   unsigned int compatibilityVersion { LSI_COMPATIBILITY_VERSION };
   uint32_t     flags                { static_cast<uint32_t>(SystemInfo::Flags::All) };

   static const struct option long_options[] = {
      { "help",              no_argument,       nullptr, 'h' },
      { "version",           no_argument,       nullptr, 'v' },
      { "compatibility",     required_argument, nullptr, 'c' },
      { "all",               no_argument,       nullptr, 'A' },
      { "none",              no_argument,       nullptr, 'a' },
      { "with-battery",      no_argument,       nullptr, 'B' },
      { "without-battery",   no_argument,       nullptr, 'b' },
      { "with-hostname",     no_argument,       nullptr, 'O' },
      { "without-hostname",  no_argument,       nullptr, 'o' },
      { "with-kernel",       no_argument,       nullptr, 'K' },
      { "without-kernel",    no_argument,       nullptr, 'k' },
      { "with-load",         no_argument,       nullptr, 'L' },
      { "without-load",      no_argument,       nullptr, 'l' },
      { "with-memory",       no_argument,       nullptr, 'M' },
      { "without-memory",    no_argument,       nullptr, 'm' },
      { "with-network",      no_argument,       nullptr, 'N' },
      { "without-network",   no_argument,       nullptr, 'n' },
      { "with-os",           no_argument,       nullptr, 'E' },
      { "without-os",        no_argument,       nullptr, 'e' },
      { "with-ssh",          no_argument,       nullptr, 'F' },
      { "without-ssh",       no_argument,       nullptr, 'f' },
      { "with-storage",      no_argument,       nullptr, 'S' },
      { "without-storage",   no_argument,       nullptr, 's' },
      { "with-uptime",       no_argument,       nullptr, 'T' },
      { "without-uptime",    no_argument,       nullptr, 't' },
      { "with-user",         no_argument,       nullptr, 'U' },
      { "without-user",      no_argument,       nullptr, 'u' },
      { nullptr,             0,                 nullptr, 0   }
   };

   int option    { 0 };
   int longIndex { 0 };
   while((option = getopt_long(argc,
                               argv,
                               "hvc:AaBbEeFfKkLlMmNnOoSsTtUu",
                               long_options,
                               &longIndex)) != -1) {
      switch(option) {
         case 'A':
            flags = static_cast<uint32_t>(SystemInfo::Flags::All);
            break;
         case 'a':
            flags = 0;
            break;
         case 'B':
            flags |= static_cast<uint32_t>(SystemInfo::Flags::Battery);
            break;
         case 'b':
            flags &= ~static_cast<uint32_t>(SystemInfo::Flags::Battery);
            break;
         case 'E':
            flags |= static_cast<uint32_t>(SystemInfo::Flags::OS);
            break;
         case 'e':
            flags &= ~static_cast<uint32_t>(SystemInfo::Flags::OS);
            break;
         case 'F':
            flags |= static_cast<uint32_t>(SystemInfo::Flags::SSH);
            break;
         case 'f':
            flags &= ~static_cast<uint32_t>(SystemInfo::Flags::SSH);
            break;
         case 'O':
            flags |= static_cast<uint32_t>(SystemInfo::Flags::Hostname);
            break;
         case 'o':
            flags &= ~static_cast<uint32_t>(SystemInfo::Flags::Hostname);
            break;
         case 'K':
            flags |= static_cast<uint32_t>(SystemInfo::Flags::Kernel);
            break;
         case 'k':
            flags &= ~static_cast<uint32_t>(SystemInfo::Flags::Kernel);
            break;
         case 'L':
            flags |= static_cast<uint32_t>(SystemInfo::Flags::Load);
            break;
         case 'l':
            flags &= ~static_cast<uint32_t>(SystemInfo::Flags::Load);
            break;
         case 'M':
            flags |= static_cast<uint32_t>(SystemInfo::Flags::Memory);
            break;
         case 'm':
            flags &= ~static_cast<uint32_t>(SystemInfo::Flags::Memory);
            break;
         case 'N':
            flags |= static_cast<uint32_t>(SystemInfo::Flags::Network);
            break;
         case 'n':
            flags &= ~static_cast<uint32_t>(SystemInfo::Flags::Network);
            break;
         case 'S':
            flags |= static_cast<uint32_t>(SystemInfo::Flags::Storage);
            break;
         case 's':
            flags &= ~static_cast<uint32_t>(SystemInfo::Flags::Storage);
            break;
         case 'T':
            flags |= static_cast<uint32_t>(SystemInfo::Flags::Uptime);
            break;
         case 't':
            flags &= ~static_cast<uint32_t>(SystemInfo::Flags::Uptime);
            break;
         case 'U':
            flags |= static_cast<uint32_t>(SystemInfo::Flags::User);
            break;
         case 'u':
            flags &= ~static_cast<uint32_t>(SystemInfo::Flags::User);
            break;
         case 'c': {
            unsigned long parsedVersion = 0;
            if(!parseUnsignedVersion(optarg, parsedVersion)) {
               std::cerr << "ERROR: Invalid compatibility version \""
                         << optarg << "\"!\n";
               return 1;
            }
            if(parsedVersion > static_cast<unsigned long>(LSI_COMPATIBILITY_VERSION)) {
               std::cerr << "ERROR: Requested compatibility version "
                         << parsedVersion << " > available version "
                         << LSI_COMPATIBILITY_VERSION << "!\n";
               return 1;
            }
            compatibilityVersion = static_cast<unsigned int>(parsedVersion);
            break;
         }
         case 'v':
            version();
            break;
         case 'h':
         case '?':
            usage(argv[0], (option == 'h') ? 0 : 1);
            break;
         default:
            std::cerr << "INTERNAL ERROR: Unhandled option code=" << option << "!\n";
            return 1;
      }
   }
   if(optind < argc) {
      usage(argv[0], 1);
   }

#if defined(_WIN32)
   struct WinSockScope {
      WSADATA Data        { };
      bool    Initialized { false };
      WinSockScope() noexcept {
         Initialized = (WSAStartup(MAKEWORD(2, 2), &Data) == 0);
      }
      ~WinSockScope() noexcept {
         if(Initialized) {
            WSACleanup();
         }
      }
      WinSockScope(const WinSockScope&)            = delete;
      WinSockScope& operator=(const WinSockScope&) = delete;
   } winSockScope;
#endif

   // ====== Initialize and print SystemInfo within try-catch ===============
   try {
      SystemInfo systemInfo(compatibilityVersion, flags);
      systemInfo.print(std::cout);
      std::cout.flush();
      if(!std::cout) {
         std::cerr << "ERROR: Failed to write output!\n";
         return 1;
      }
   }
   catch(const std::exception& exception) {
      std::cerr << "ERROR: " << exception.what() << '\n';
      return 1;
   }

   return 0;
}
