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
   printf("unixts2time %s\n", SYSTEMTOOLS_VERSION);
   exit(0);
}


// ###### Usage #############################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void usage(const char* program, const int exitCode)
{
   fprintf(stderr, "%s %s unix_timestamp [...]"
           " [-T|--template time_format_template]"
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
   setlocale(LC_NUMERIC, "C");   // Use "." for fractional numbers!
   bindtextdomain("unixts2time", nullptr);
   textdomain("unixts2time");

   // ====== Handle arguments ===============================================
   static const struct option long_options[] = {
      { "template",       required_argument, 0, 'T' },
      { "human-readable", no_argument,       0, 'H' },
      { "seconds",        no_argument,       0, 's' },
      { "milliseconds",   no_argument,       0, 'm' },
      { "microseconds",   no_argument,       0, 'u' },
      { "nanoseconds",    no_argument,       0, 'n' },
      { "help",           no_argument,       0, 'h' },
      { "version",        no_argument,       0, 'v' },
      {  nullptr,         0,                 0, 0   }
   };

   int          option;
   int          longIndex;
   bool         humanReadable      = false;
   unsigned int initialDivideBy    = 0;         // auto-detect later
   const char*  unit               = nullptr;   // auto-detect later
   const char*  timeFormatTemplate = "%Y-%m-%d %H:%M:%S";
   while( (option = getopt_long(argc, argv, "T:Hsmunvh", long_options, &longIndex)) != -1 ) {
      switch(option) {
         case 'T':
            timeFormatTemplate = optarg;
          break;
         case 'H':
            humanReadable = true;
          break;
         case 'n':
            initialDivideBy = 1;
            unit     = "ns";
          break;
         case 'u':
            initialDivideBy = 1000;
            unit     = "µs";
          break;
         case 'm':
            initialDivideBy = 1000000;
            unit     = "ms";
          break;
         case 's':
            initialDivideBy = 1000000000;
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
#if (defined(__BITINT_MAXWIDTH__) && (__BITINT_MAXWIDTH__ >= 128))
   _BitInt(128) unixTS;
#else
   // NOTE: A 64-bit signed long long will overflow on April 11, 2262!
   long long    unixTS;
#endif
   for(int i = optind; i <= argc; i++) {
      unsigned int divideBy = initialDivideBy;

      // ====== Use current time, if no timestamp is given ==================
      if(i == argc) {
         if(optind == argc) {
            struct timespec ts;
            if(clock_gettime(CLOCK_REALTIME, &ts) == -1) {
               perror(gettext("clock_gettime() failed"));
               exit(1);
            }
#if (defined(__BITINT_MAXWIDTH__) && (__BITINT_MAXWIDTH__ >= 128))
            unixTS = ((_BitInt(128))1000000000LL * ts.tv_sec) + ts.tv_nsec;
#else
            unixTS = (1000000000LL * ts.tv_sec) + ts.tv_nsec;
#endif
            if(divideBy == 0) {
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
         // ------ Try to parse integer -------------------------------------
         char* endptr;
         unixTS = strtoll(argv[i], &endptr, 0);
         if( (endptr != nullptr) && (*endptr == 0x00) ) {
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

         // ------ Try to parse double --------------------------------------
         else {
            const long double unixTSasDouble = strtold(argv[i], &endptr);
            if( (endptr != nullptr) && (*endptr == 0x00) ) {
               if(divideBy == 0) {
                  if( (unixTSasDouble > -5000000000.0) && (unixTSasDouble < 5000000000.0) ) {
                     divideBy = 1000000000;
                     unit     = "s";
                  }
                  else if( (unixTSasDouble > -5000000000000.0) && (unixTSasDouble < 5000000000000.0) ) {
                     divideBy = 1000000;
                     unit     = "ms";
                  }
                  else if( (unixTSasDouble > -5000000000000000.0) && (unixTSasDouble < 5000000000000000.0) ) {
                     divideBy = 1000;
                     unit     = "µs";
                  }
                  else {
                     divideBy = 1;
                     unit     = "ns";
                  }
               }
               unixTS = unixTSasDouble * divideBy;   // convert to ns
            }
            else {
               fputs(gettext("ERROR: Invalid Unix timestamp!"), stderr);
               fputs("\n", stderr);
               exit(1);
            }
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
         fputs(gettext("ERROR: gmtime() failed!"), stderr);
         fputs("\n", stderr);
         exit(1);
      }

      // ====== Prepare date/time string ====================================
      char frontTimeString[1024]        = { 0 };
      char secondsString[128]           = { 0 };
      char fractionalSecondsString[128] = { 0 };
      char backTimeString[1024]         = { 0 };

      const char* secondsPlaceholder = findSecondsPlaceholder(timeFormatTemplate);
      if(secondsPlaceholder != nullptr) {
         // ------ Prepare front part (before seconds) ----------------------
         const size_t frontLength = (size_t)(secondsPlaceholder - timeFormatTemplate);
         char* frontFormatString  = malloc(frontLength + 1);
         if(frontFormatString == nullptr) {
            fprintf(stderr, gettext("ERROR: malloc() failed: %s!"), strerror(errno));
            exit(1);
         }
         memcpy(frontFormatString, timeFormatTemplate, frontLength);
         frontFormatString[frontLength] = 0x00;
         if(strftime(frontTimeString, sizeof(frontTimeString), frontFormatString, t) == 0) {
            fprintf(stderr, gettext("ERROR: strftime() with format template \"%s\" failed!"),
                    frontFormatString);
            fputs("\n", stderr);
            free(frontFormatString);
            exit(1);
         }
         free(frontFormatString);

         // ------ Parse the middle part (seconds) --------------------------
         if(strftime(secondsString, sizeof(secondsString), "%S", t) == 0) {
            fprintf(stderr, gettext("ERROR: strftime() with format template \"%s\" failed!"),
                    "%S");
            fputs("\n", stderr);
            exit(1);
         }

         const char* fractionalSecondsFormatString;
         if(divideBy == 1) {
            fractionalSecondsFormatString = "%1.9f";
         }
         else if(divideBy == 1000) {
            fractionalSecondsFormatString = "%1.6f";
         }
         else if(divideBy == 1000000) {
            fractionalSecondsFormatString = "%1.3f";
         }
         else {
            fractionalSecondsFormatString = "%1.0f";
         }
         snprintf(fractionalSecondsString, sizeof(fractionalSecondsString), fractionalSecondsFormatString,
                  (double)ts.tv_nsec / 1000000000.0);

         // ------ Prepare the back part (after seconds) --------------------
         const char* backFormatString = secondsPlaceholder + 2;
         if(backFormatString[0] != 0x00) {
            if(strftime(backTimeString, sizeof(backTimeString), backFormatString, t) == 0) {
               fprintf(stderr, gettext("ERROR: strftime() with format template \"%s\" failed!"),
                       backFormatString);
               fputs("\n", stderr);
               exit(1);
            }
         }
      }
      else {
         if(strftime(frontTimeString, sizeof(frontTimeString), timeFormatTemplate, t) == 0) {
            fprintf(stderr, gettext("ERROR: strftime() with format template \"%s\" failed!"),
                    timeFormatTemplate);
            fputs("\n", stderr);
            exit(1);
         }
      }

      // ====== Print result ================================================
      if(!humanReadable) {
         fputs(frontTimeString, stdout);
         fputs(secondsString, stdout);
         if(fractionalSecondsString[0] != 0x00) {
            fputs(&fractionalSecondsString[1], stdout);
         }
         fputs(backTimeString, stdout);
      }
      else {
         fprintf(stdout, gettext("%s%s%s%s is %lld (0x%llx) %s since the Unix Epoch"),
                 frontTimeString,
                 secondsString, (fractionalSecondsString[0] != 0x00) ? fractionalSecondsString + 1 : "",
                 backTimeString,
                 (long long)(unixTS / divideBy),
                 (unsigned long long)(unixTS / divideBy),
                 unit);
      }
      fputs("\n", stdout);
   }

   return 0;
}
