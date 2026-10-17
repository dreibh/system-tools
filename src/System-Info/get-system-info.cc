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

#include <clocale>
#include <cstdlib>
#include <getopt.h>
#include <iostream>

#if defined(_WIN32)
#include <winsock2.h>
#endif

#include <libsysteminfo.h>

#include "package-version.h"


// ###### Version ###########################################################
[[noreturn]] static void version()
{
   std::cout << "get-system-info " << SYSTEMTOOLS_VERSION << '\n';
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
                   ENTRY(OPT("-v") PIPE OPT("--version"));
   exit(exitCode);
}
// ------ END-OF-USAGE-BLOCK ------



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   unsigned int compatibilityVersion { LSI_COMPATIBILITY_VERSION };
   uint32_t     flags                { SystemInfo::Flags::All };

   // ====== Initialise locale support ======================================
   if(std::setlocale(LC_ALL, "") == nullptr) {
      std::setlocale(LC_ALL, "C.UTF-8");
   }

   // ====== Handle arguments ===============================================
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
      {  nullptr,            0,                 nullptr, 0   }
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
            flags = SystemInfo::Flags::All;
            break;
         case 'a':
            flags = 0;
            break;
         case 'B':
            flags |= SystemInfo::Flags::Battery;
            break;
         case 'b':
            flags &= ~SystemInfo::Flags::Battery;
            break;
         case 'E':
            flags |= SystemInfo::Flags::OS;
            break;
         case 'e':
            flags &= ~SystemInfo::Flags::OS;
            break;
         case 'F':
            flags |= SystemInfo::Flags::SSH;
            break;
         case 'f':
            flags &= ~SystemInfo::Flags::SSH;
            break;
         case 'O':
            flags |= SystemInfo::Flags::Hostname;
            break;
         case 'o':
            flags &= ~SystemInfo::Flags::Hostname;
            break;
         case 'K':
            flags |= SystemInfo::Flags::Kernel;
            break;
         case 'k':
            flags &= ~SystemInfo::Flags::Kernel;
            break;
         case 'L':
            flags |= SystemInfo::Flags::Load;
            break;
         case 'l':
            flags &= ~SystemInfo::Flags::Load;
            break;
         case 'M':
            flags |= SystemInfo::Flags::Memory;
            break;
         case 'm':
            flags &= ~SystemInfo::Flags::Memory;
            break;
         case 'N':
            flags |= SystemInfo::Flags::Network;
            break;
         case 'n':
            flags &= ~SystemInfo::Flags::Network;
            break;
         case 'S':
            flags |= SystemInfo::Flags::Storage;
            break;
         case 's':
            flags &= ~SystemInfo::Flags::Storage;
            break;
         case 'T':
            flags |= SystemInfo::Flags::Uptime;
            break;
         case 't':
            flags &= ~SystemInfo::Flags::Uptime;
            break;
         case 'U':
            flags |= SystemInfo::Flags::User;
            break;
         case 'u':
            flags &= ~SystemInfo::Flags::User;
            break;
         case 'c':
            compatibilityVersion = static_cast<unsigned int>(
               std::strtoul(optarg, nullptr, 10));
            if(compatibilityVersion > LSI_COMPATIBILITY_VERSION) {
               std::cerr << "ERROR: Requested compatibility version "
                         << compatibilityVersion << " > available version "
                         << LSI_COMPATIBILITY_VERSION << "!\n";
               return 1;
            }
            break;
         case 'v':
            version();
            break;
         case 'h':
         case '?':
            usage(argv[0], (option == 'h') ? 0 : 1);
            break;
         case '-':
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
      WSADATA Data { };
      WinSockScope() { WSAStartup(MAKEWORD(2, 2), &Data); }
      ~WinSockScope() { WSACleanup(); }
   } winSockScope;
#endif

   // ====== Initialize and print SystemInfo within try-catch ===============
   try {
      SystemInfo systemInfo(compatibilityVersion, flags);
      systemInfo.print(std::cout);
   }
   catch(const std::exception& exception) {
      std::cerr << "ERROR: " << exception.what() << '\n';
      return 1;
   }

   return 0;
}
