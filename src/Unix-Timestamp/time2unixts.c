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
// Time to Unix Timestamp Conversion
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
// Contact: thomas.dreibholz@gmail.com

#define _GNU_SOURCE   /* for timegm() */
#include <getopt.h>
#include <libintl.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifndef nullptr
#define nullptr NULL
#endif

#include "package-version.h"


// ###### Version ###########################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void version()
{
   printf("time2unixts %s\n", SYSTEMTOOLS_VERSION);
   exit(0);
}


// ###### Usage #############################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void usage(const char* program, const int exitCode)
{
   fprintf(stderr, "%s %s [time_string ...]"
           " [-F|--float]"
           " [-I|--integer]"
           " [-H|--human-readable]"
           " [-s|--seconds]"
           " [-m|--milliseconds]"
           " [-u|--microseconds]"
           " [-n|--nanoseconds]"
           " [-h|--help]"
           " [-v|--version]\n",
           gettext("Usage:"), program);
   exit(exitCode);
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   // ====== Initialise i18n support ========================================
   if(setlocale(LC_ALL, "") == nullptr) {
      setlocale(LC_ALL, "C.UTF-8");   // "C" should exist on all systems!
   }
   bindtextdomain("time2unixts", nullptr);
   textdomain("time2unixts");

   // ====== Handle arguments ===============================================
   const static struct option long_options[] = {
      { "float",                  no_argument, 0, 'F' },
      { "integer-decimal",        no_argument, 0, 'I' },
      { "integer-hexadecimal",    no_argument, 0, 'X' },
      { "integer-0x-hexadecimal", no_argument, 0, '0' },
      { "human-readable",         no_argument, 0, 'H' },
      { "seconds",                no_argument, 0, 's' },
      { "milliseconds",           no_argument, 0, 'm' },
      { "microseconds",           no_argument, 0, 'u' },
      { "nanoseconds",            no_argument, 0, 'n' },
      { "help",                   no_argument, 0, 'h' },
      { "version",                no_argument, 0, 'v' },
      {  nullptr,                 0,           0, 0   }
   };

   int          option;
   int          longIndex;
   int          useInteger    = 10;
   bool         humanReadable = false;
   unsigned int divideBy      = 1;
   const char*  unit          = "ns";
   while( (option = getopt_long(argc, argv, "FIX0Hsmunvh", long_options, &longIndex)) != -1 ) {
      switch(option) {
         case 'F':
            useInteger = 0;
          break;
         case 'I':
            useInteger = 10;
          break;
         case 'X':
            useInteger = 16;
          break;
         case '0':
            useInteger = -16;
          break;
         case 'H':
            humanReadable = true;
          break;
         case 'n':
            divideBy = 1;
            unit     = "ns";
          break;
         case 'u':
            divideBy = 1000;
            unit     = "µs";
          break;
         case 'm':
            divideBy = 1000000;
            unit     = "ms";
          break;
         case 's':
            divideBy = 1000000000;
            unit     = "s";
          break;
         case 'v':
            version();
          break;
         case 'h':
            usage(argv[0], 0);
          break;
         default:
            fprintf(stderr, gettext("ERROR: Invalid argument %s!"), argv[optind - 1]);
            fputs("\n", stderr);
            return 1;
          break;
      }
   }

   // ====== Obtain Unix timestamp in ns ====================================
   struct timespec ts;
   for(unsigned int i = optind; i <= argc; i++) {
      // ====== Use current time, if no date/time string is given ===========
      if(i == argc) {
         if(optind == argc) {
            if(clock_gettime(CLOCK_REALTIME, &ts) == -1) {
               perror(gettext("clock_gettime() failed"));
               exit(1);
            }
         }
         else {
            break;
         }
      }

      // ====== Parse the next date/time string =============================
      struct tm tm = { };
      const char* remainder = strptime(argv[i], "%Y-%m-%d %H:%M:%S", &tm);
      if(remainder != nullptr) {
         ts.tv_sec = timegm(&tm);
         if(remainder[0] != 0x00) {
            char*        endptr;
            const double fraction = strtod(remainder, &endptr);
            if( (endptr == nullptr) || (*endptr != 0x00) || (fraction < 0.0) ) {
               remainder = nullptr;   // parse error
            }
            else {
               ts.tv_nsec = (long long)(fraction * 1e9);
            }
         }
         else {
            ts.tv_nsec = 0;
         }
      }
      if(remainder == nullptr) {
         fputs(gettext("ERROR: Invalid time string!"), stderr);
         fputs("\n", stderr);
         exit(1);
      }

      // ====== Convert timespec to Unix timestamp =============================
      const long long unixTS =
         (1000000000ULL * ts.tv_sec) + ts.tv_nsec;

      if(useInteger != 0) {
         if(useInteger == 16) {
            printf("%llx", unixTS / divideBy);
         }
         else if(useInteger == -16) {
            printf("0x%llx", unixTS / divideBy);
         }
         else {
            printf("%lld", unixTS / divideBy);
         }
      }
      else {
         const char* format;
         if(divideBy == 1000000000) {
            format = "%1.9f";
         }
         else if(divideBy == 1000000) {
            format = "%1.6f";
         }
         else if(divideBy == 1000) {
            format = "%1.3f";
         }
         else {
            format = "%1.0f";
         }
         printf(format, (double)unixTS / (double)divideBy);
      }
      if(humanReadable) {
         printf(gettext(" %s since the Unix Epoch"), unit);
      }
      fputs("\n", stdout);
   }

   return 0;
}
