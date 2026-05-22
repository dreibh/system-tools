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
// Random-Sleep
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#ifndef nullptr
#define nullptr NULL
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#else
#define bindtextdomain(domain, dirname) { }
#define textdomain(domain) { }
#define gettext(string) string
#define ngettext(singular, plural, n) ((n) == 1 ? (singular) : (plural))
#endif

#include "package-version.h"


// ###### Generate random number out of [min, max] ##########################
static double runiform(const double min, const  double max)
{
   if(min == max) {
      // There is no other choice here:
      return min;
   }
   const double range   = (max - min);
   const double divisor = RAND_MAX / range;
   return min + (rand() / divisor);
}


// ###### Version ###########################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void version(void)
{
   printf("random-sleep %s\n", SYSTEMTOOLS_VERSION);
   exit(0);
}


// ###### Usage #############################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void usage(const char* program, const int exitCode)
{
   fprintf(stderr, "%s %s min_delay max_delay"
           " [-q|--quiet]"
           " [-w|--verbose]"
           " [-h|--help]"
           " [-v|--version]\n",
           gettext("Usage:"), program);
   exit(exitCode);
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   bool verboseMode = false;

   // ====== Initialise i18n support ========================================
   if(setlocale(LC_ALL, "") == nullptr) {
      setlocale(LC_ALL, "C.UTF-8");   // "C" should exist on all systems!
   }
   setlocale(LC_NUMERIC, "C.UTF-8");  // Use "." for fractional numbers!
   bindtextdomain("random-sleep", nullptr);
   textdomain("random-sleep");

   // ====== Handle arguments ===============================================
   static const struct option long_options[] = {
      { "verbose", no_argument, 0, 'w' },
      { "quiet",   no_argument, 0, 'q' },
      { "help",    no_argument, 0, 'h' },
      { "version", no_argument, 0, 'v' },
      {  nullptr,  0,           0, 0   }
   };

   int option;
   int longIndex;
   while( (option = getopt_long(argc, argv, "wqhv", long_options, &longIndex)) != -1 ) {
      switch(option) {
         case 'w':
            verboseMode = true;
          break;
         case 'q':
            verboseMode = false;
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
   if(optind + 2 != argc) {
      usage(argv[0], 1);
   }

   // ====== Parse delay bounds =============================================
   char*        endptrDelayMin;
   char*        endptrDelayMax;
   const double delayMin = strtod(argv[optind + 0], &endptrDelayMin);
   const double delayMax = strtod(argv[optind + 1], &endptrDelayMax);
   if( (endptrDelayMin == argv[optind + 0]) || (*endptrDelayMin != 0x00) ||
       (endptrDelayMax == argv[optind + 1]) || (*endptrDelayMax != 0x00) ||
       (delayMin < 0.0) || (delayMin > delayMax) ) {
      fputs(gettext("ERROR: Invalid min_delay/max_delay!"), stderr);
      fputs("\n", stderr);
      return 1;
   }

   // ====== Initialise random number generator =============================
   struct timeval tv;
   if(gettimeofday(&tv, nullptr) == 0) {
      srand(tv.tv_sec ^ tv.tv_usec ^ getpid());
   }

   // ====== Random sleep ===================================================
   double delay = runiform(delayMin, delayMax);
   if(delay < 0.0) {
      delay = 0.0;
   }

   if(verboseMode) {
      printf(gettext("Sleeping for %1.6f s ..."), delay);
      puts("");
   }

   struct timespec sleepTime;
   sleepTime.tv_sec  = (time_t)delay;
   sleepTime.tv_nsec = (long)((delay - (double)sleepTime.tv_sec) * 1000000000.0);

   struct timespec remainingTime;
   while(nanosleep(&sleepTime, &remainingTime) == -1) {
      sleepTime = remainingTime;
   }

   if(verboseMode) {
      puts(gettext("Woke up!"));
   }

   return 0;
}
