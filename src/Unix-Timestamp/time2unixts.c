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

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#else
#define bindtextdomain(domain, dirname) { }
#define textdomain(domain) { }
#define gettext(string) string
#define ngettext(singular, plural, n) ((n) == 1 ? (singular) : (plural))
#endif

#include "package-version.h"

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ < 202311L)
#ifndef nullptr
#define nullptr ((void*)0)
#endif
#endif


// ###### Find %S placeholder for seconds in time format string #############
static const char* findSecondsPlaceholder(const char* formatString)
{
   const char* ptr = formatString;
   while(*ptr) {
      if(*ptr == '%') {
         switch(*(ptr + 1)) {
            case 0x00:
               ptr++;
             break;
            case 'S':
               return ptr;
             break;
            default:
               ptr += 2;
             break;
         }
      } else {
         ptr++;
      }
   }
   return nullptr;
}


// ###### Version ###########################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void version(void)
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
   fprintf(stderr, "%s %s time_string [...]"
           " [-T|--template time_format_template]"
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
   bindtextdomain("time2unixts", nullptr);
   textdomain("time2unixts");

   // ====== Handle arguments ===============================================
   static const struct option long_options[] = {
      { "template",               required_argument, 0, 'T' },
      { "float",                  no_argument,       0, 'F' },
      { "integer-decimal",        no_argument,       0, 'I' },
      { "integer-hexadecimal",    no_argument,       0, 'X' },
      { "integer-0x-hexadecimal", no_argument,       0, '0' },
      { "human-readable",         no_argument,       0, 'H' },
      { "seconds",                no_argument,       0, 's' },
      { "milliseconds",           no_argument,       0, 'm' },
      { "microseconds",           no_argument,       0, 'u' },
      { "nanoseconds",            no_argument,       0, 'n' },
      { "help",                   no_argument,       0, 'h' },
      { "version",                no_argument,       0, 'v' },
      {  nullptr,                 0,                 0, 0   }
   };

   int          option;
   int          longIndex;
   int          useInteger         = 10;
   bool         humanReadable      = false;
   unsigned int divideBy           = 1;
   const char*  unit               = "ns";
   const char*  timeFormatTemplate = "%Y-%m-%d %H:%M:%S";
   while( (option = getopt_long(argc, argv, "T:FIX0Hsmunvh", long_options, &longIndex)) != -1 ) {
      switch(option) {
         case 'T':
            timeFormatTemplate = optarg;
          break;
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

   // ====== Obtain Unix timestamp in ns ====================================
   struct timespec ts;
   for(int i = optind; i <= argc; i++) {
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
      else {
         struct tm   tm                 = { 0 };
         const char* timeString         = argv[i];
         const char* remainder          = nullptr;
         const char* secondsPlaceholder = findSecondsPlaceholder(timeFormatTemplate);
         if(secondsPlaceholder != nullptr) {
            long long nanoseconds = 0;

            // ------ Parse front part (before seconds) ---------------------
            const size_t frontLength = (size_t)(secondsPlaceholder - timeFormatTemplate);
            char* frontFormatString  = malloc(frontLength + 1);
            if(frontFormatString == nullptr) {
               fprintf(stderr, gettext("ERROR: malloc() failed: %s!"), strerror(errno));
               exit(1);
            }
            memcpy(frontFormatString, timeFormatTemplate, frontLength);
            frontFormatString[frontLength] = 0x00;
            const char* remainder1 = strptime(timeString, frontFormatString, &tm);
            free(frontFormatString);

            // ------ Parse the middle part (seconds) -----------------------
            if(remainder1 != nullptr) {
               char*  remainder2    = nullptr;
               double total_seconds = strtod(remainder1, &remainder2);
               if( (remainder2 != remainder1) && (total_seconds >= 0.0) ) {
                  // Split seconds into integer seconds and nanoseconds:
                  tm.tm_sec = (int)total_seconds;
                  double fraction = total_seconds - tm.tm_sec;
                  nanoseconds = (long long)(fraction * 1e9 + 0.5);   // +0.5 for rounding safety

                  // ------ Parse the back part (after seconds) -------------
                  const char* backFormatString = secondsPlaceholder + 2;
                  if(backFormatString[0] != 0x00) {
                     remainder = strptime(remainder2, backFormatString, &tm);
                  } else {
                     remainder = remainder2;
                  }
               }
            }
            ts.tv_sec  = timegm(&tm);
            ts.tv_nsec = nanoseconds;
         }
         else {
            remainder  = strptime(argv[i], timeFormatTemplate, &tm);
            ts.tv_sec  = timegm(&tm);
            ts.tv_nsec = 0;
         }

         if( (remainder == nullptr) || (remainder[0] != 0x00) ) {
            fprintf(stderr, gettext("ERROR: Unable to parse time string \"%s\" with format template \"%s\"!"),
                    timeString, timeFormatTemplate);
            fputs("\n", stderr);
            exit(1);
         }
      }

      // ====== Convert timespec to Unix timestamp =============================
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L)
      const _BitInt(128) unixTS =
#else
      // NOTE: 64-bit signed long long will overflow on April 11, 2262!
      const long long unixTS =
#endif
         (1000000000LL * ts.tv_sec) + ts.tv_nsec;

      if(useInteger != 0) {
         if(useInteger == 16) {
            printf("%llx", (long long)(unixTS / divideBy));
         }
         else if(useInteger == -16) {
            printf("0x%llx", (long long)(unixTS / divideBy));
         }
         else {
            printf("%lld", (long long)(unixTS / divideBy));
         }
      }
      else {
         const char* format;
         if(divideBy == 1000000000) {
            format = "%1.9Lf";
         }
         else if(divideBy == 1000000) {
            format = "%1.6Lf";
         }
         else if(divideBy == 1000) {
            format = "%1.3Lf";
         }
         else {
            format = "%1.0Lf";
         }
         printf(format, (long double)unixTS / (long double)divideBy);
      }
      if(humanReadable) {
         printf(gettext(" %s since the Unix Epoch"), unit);
      }
      fputs("\n", stdout);
   }

   return 0;
}
