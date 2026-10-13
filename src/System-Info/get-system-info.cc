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

#include <libsysteminfo.h>

#include "package-version.h"

#if defined(_WIN32)
#include <winsock2.h>
#endif


// ###### Version ###########################################################
[[noreturn]] static void version()
{
   std::cout << "get-system-info " << SYSTEMTOOLS_VERSION << '\n';
   std::exit(0);
}


// ###### Usage #############################################################
[[noreturn]] static void usage(const char* program, const int exitCode)
{
   std::cerr << "Usage: " << program
             << " [-h|--help] [-v|--version] [-c|--compatibility version]\n";
   std::exit(exitCode);
}


// ###### Main program ######################################################
int main(int argc, char** argv)
{
   unsigned int compatibilityVersion = LSI_COMPATIBILITY_VERSION;

   // ====== Initialise locale support ======================================
   if (std::setlocale(LC_ALL, "") == nullptr) {
      std::setlocale(LC_ALL, "C.UTF-8");
   }

   // ====== Handle arguments ===============================================
   static const struct option long_options[] = {
      { "help",          no_argument,       nullptr, 'h' },
      { "version",       no_argument,       nullptr, 'v' },
      { "compatibility", required_argument, nullptr, 'c' },
      {  nullptr,        0,                 nullptr, 0   }
   };

   int option = 0;
   int longIndex = 0;
   while ((option = getopt_long(argc, argv, "hvc:", long_options, &longIndex)) != -1) {
      switch (option) {
         case 'c':
            compatibilityVersion = static_cast<unsigned int>(std::strtoul(optarg, nullptr, 10));
            if (compatibilityVersion > LSI_COMPATIBILITY_VERSION) {
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
   if (optind < argc) {
      usage(argv[0], 1);
   }

#if defined(_WIN32)
   struct WinSockInit {
      WSADATA data;
      WinSockInit() { WSAStartup(MAKEWORD(2, 2), &data); }
      ~WinSockInit() { WSACleanup(); }
   } wsInit;
#endif

   // ====== Initialize and print SystemInfo within try-catch ===============
   try {
      SystemInfo systemInfo(compatibilityVersion, SystemInfo::Flags::All);
      systemInfo.print(std::cout);
   }
   catch (const std::exception& e) {
      std::cerr << "ERROR: " << e.what() << '\n';
      return 1;
   }

   return 0;
}
