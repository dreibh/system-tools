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

#include <ctype.h>
#include <getopt.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <libsysteminfo.h>

#include "package-version.h"

#if !defined(HAVE_C23_NULLPTR)
#define nullptr ((void*)0)
#endif


// ###### Version ###########################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void version(void)
{
   printf("get-system-info %s\n", SYSTEMTOOLS_VERSION);
   exit(0);
}


// ###### Usage #############################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void usage(const char* program, const int exitCode)
{
   fprintf(stderr, "Usage: %s [-h|--help] [-v|--version] [-c|--compatibility version]\n", program);
   exit(exitCode);
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   unsigned int compatibilityVersion = LSI_COMPATIBILITY_VERSION;

   // ====== Initialise locale support ======================================
   if(setlocale(LC_ALL, "") == nullptr) {
      setlocale(LC_ALL, "C.UTF-8");   // "C" should exist on all systems!
   }

   // ====== Handle arguments ===============================================
   static const struct option long_options[] = {
      { "help",          no_argument,       0, 'h' },
      { "version",       no_argument,       0, 'v' },
      { "compatibility", required_argument, 0, 'c' },
      {  nullptr,        0,                 0, 0   }
   };

   int          option;
   int          longIndex;
   while( (option = getopt_long(argc, argv, "hvc:", long_options, &longIndex)) != -1 ) {
      switch(option) {
         case 'c':
            compatibilityVersion = atoll(optarg);
            if(compatibilityVersion > LSI_COMPATIBILITY_VERSION) {
               fprintf(stderr, "ERROR: Requested compatibility version %u > available version %u!\n",
                       compatibilityVersion, LSI_COMPATIBILITY_VERSION);
               return 1;
            }
          break;
         case 'v':
            version();
          break;
         case 'h':
         case '?':
            // Exit with 0 on h/help, exit with 1 on '?' (unknown option):
            usage(argv[0], (option == 'h') ? 0 : 1);
          break;
         case '-':
          break;
         default:
            // This should not happen: wrong getopt parameters, or missing case?
            fprintf(stderr, "INTERNAL ERROR: Unhandled option c=%c code=%x!\n",
                    (isprint(option) ? (char)option : ' '), option);
            return 1;
          break;
      }
   }
   if(optind < argc) {
      usage(argv[0], 1);
   }

   // ====== Show system information in machine-readable form ===============
   struct SystemInfo* systemInfo = systemInfoObtain(LSI_COMPATIBILITY_VERSION, 0xffffffff);
   if(systemInfo) {
      systemInfoPrint(systemInfo, stdout);
      systemInfoRelease(systemInfo);
   }
   return 0;
}
