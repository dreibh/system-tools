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
// Unix Timestamp to Time Conversion
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
   printf("unixts2time %s\n", SYSTEMTOOLS_VERSION);
   exit(0);
}


// ###### Usage #############################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void usage(const char* program, const int exitCode)
{
   fprintf(stderr, "%s %s [unix_timestamp ...]"
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
   setlocale(LC_NUMERIC, "C.UTF-8");   // Use "." for fractional numbers!
   bindtextdomain("unixts2time", nullptr);
   textdomain("unixts2time");

   // ====== Handle arguments ===============================================
   const static struct option long_options[] = {
      { "float",          no_argument, 0, 'F' },
      { "integer",        no_argument, 0, 'I' },
      { "human-readable", no_argument, 0, 'H' },
      { "seconds",        no_argument, 0, 's' },
      { "milliseconds",   no_argument, 0, 'm' },
      { "microseconds",   no_argument, 0, 'u' },
      { "nanoseconds",    no_argument, 0, 'n' },
      { "help",           no_argument, 0, 'h' },
      { "version",        no_argument, 0, 'v' },
      {  nullptr,         0,           0, 0   }
   };

   int          option;
   int          longIndex;
   bool         useInteger    = true;
   bool         humanReadable = false;
   unsigned int divideBy      = 0;         // auto-detect later
   const char*  unit          = nullptr;   // auto-detect later
   while( (option = getopt_long(argc, argv, "FIHsmunvh", long_options, &longIndex)) != -1 ) {
      switch(option) {
         case 'F':
            useInteger = false;
          break;
         case 'I':
            useInteger = true;
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
         case '-':
          break;
         default:
            fprintf(stderr, gettext("ERROR: Invalid argument %s!"), argv[optind - 1]);
            fputs("\n", stderr);
            return 1;
          break;
      }
   }

   // ====== Obtain Unix timestamp in ns ====================================
   long long unixTS;
   for(unsigned int i = optind; i <= argc; i++) {
      // ====== Use current time, if no timestamp is given ==================
      if(i == argc) {
         if(optind == argc) {
            struct timespec ts;
            if(clock_gettime(CLOCK_REALTIME, &ts) == -1) {
               perror(gettext("clock_gettime() failed"));
               exit(1);
            }
            unixTS = (1000000000ULL * ts.tv_sec) + ts.tv_nsec;   // already in ns
            if(divideBy != 0) {
               divideBy = 1;
               unit     = "ns";
            }
         }
         else {
            break;
         }
      }

      // ====== Parse the next Unix timestamp ===============================
      else {
         char* endptr;
         if(useInteger) {
            unixTS = strtoll(argv[i], &endptr, 0);
            if(divideBy == 0) {
               if( (unixTS > -5000000000LL) && (unixTS < 5000000000LL) ) {
                  divideBy = 1000000000;
                  unit     = "s";
               }
               else if( (unixTS > -5000000000000LL) && (unixTS < 5000000000000LL) ) {
                  divideBy = 1000000;
                  unit     = "ms";
               }
               else if( (unixTS > -5000000000000000LL) && (unixTS < 5000000000000000LL) ) {
                  divideBy = 1000;
                  unit     = "µs";
               }
               else {
                  divideBy = 1;
                  unit     = "ns";
               }
            }
            unixTS *= divideBy;   // convert to ns
         }
         else {
            const double unixTSasDouble = strtod(argv[i], &endptr);
            unixTS = unixTSasDouble * divideBy;
         }
         if( (endptr == nullptr) || (*endptr != 0x00) ) {
            fputs(gettext("ERROR: Invalid Unix timestamp!"), stderr);
            fputs("\n", stderr);
            exit(1);
         }
      }

      // ====== Convert Unix timestamp to time ==============================
      struct timespec ts = {
         unixTS / 1000000000,
         unixTS % 1000000000
      };
      if( (unixTS < 0) && (ts.tv_nsec < 0) ) {
         ts.tv_sec--;
         ts.tv_nsec = 1000000000 + ts.tv_nsec;
      }

      // ------ Time in seconds-granularity ---------------------------------
      const struct tm* t = gmtime(&ts.tv_sec);
      if(t == nullptr) {
         fputs(gettext("ERROR: strftime() failed!"), stderr);
         fputs("\n", stderr);
         exit(1);
      }

      char tstring[96];
      if(strftime(tstring, sizeof(tstring), "%Y-%m-%d %H:%M:%S", t) == 0) {
         fputs(gettext("ERROR: strftime() failed!"), stderr);
         fputs("\n", stderr);
         exit(1);
      }

      // ------ Fractional seconds ------------------------------------------
      char fstring[16];
      const char* format;
      if(divideBy == 1) {
         format = "%1.9f";
      }
      else if(divideBy == 1000) {
         format = "%1.6f";
      }
      else if(divideBy == 1000000) {
         format = "%1.3f";
      }
      else {
         format = "%1.0f";
      }
      snprintf((char*)&fstring, sizeof(fstring), format,
               (double)ts.tv_nsec / 1000000000.0);

      // ====== Print result ================================================
      if(!humanReadable) {
         fputs(tstring, stdout);
         fputs(&fstring[1], stdout);   // Omit "0" before comma
      }
      else {
         char ustring[64];
         snprintf((char*)&ustring, sizeof(ustring), format,
                  unixTS / (double)divideBy);
         fprintf(stdout, gettext("%s%s is %lld (0x%llx) %s since the Unix Epoch"),
               tstring, &fstring[1], unixTS /divideBy, unixTS /divideBy, unit);
      }
      fputs("\n", stdout);
   }

   return 0;
}
