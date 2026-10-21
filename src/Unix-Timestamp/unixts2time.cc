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

#if !defined(__EXTENSIONS__)
#define __EXTENSIONS__
#endif

#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <clocale>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <string_view>

#include <getopt.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#else
#define gettext(string)                   (string)
#define ngettext(singular, plural, count) (((count) == 1) ? (singular) : (plural))
#endif

#include "package-version.h"


// ###### Convert Unix timestamp in seconds to tm via std::chrono ###########
static std::tm timepoint_to_tm(const int64_t unixSeconds) noexcept
{
   const std::chrono::sys_seconds    tp { std::chrono::seconds { unixSeconds } };
   const std::chrono::sys_days       dp = std::chrono::floor<std::chrono::days>(tp);
   const std::chrono::year_month_day yearMonthDay { dp };
   const std::chrono::weekday        weekday { dp };
   const std::chrono::hh_mm_ss       time{tp - dp};

   std::tm result { };
   result.tm_sec  = static_cast<int>(time.seconds().count());
   result.tm_min  = static_cast<int>(time.minutes().count());
   result.tm_hour = static_cast<int>(time.hours().count());
   result.tm_mday = static_cast<int>(static_cast<unsigned>(yearMonthDay.day()));
   result.tm_mon  = static_cast<int>(static_cast<unsigned>(yearMonthDay.month())) - 1;
   result.tm_year = static_cast<int>(yearMonthDay.year()) - 1900;
   result.tm_wday = static_cast<int>(weekday.c_encoding());

   const std::chrono::sys_days january1 {
      std::chrono::year { yearMonthDay.year() } / std::chrono::January / 1
   };
   result.tm_yday  = static_cast<int>((dp - january1).count());
   result.tm_isdst = 0;

   return result;
}


// ###### Find %S placeholder for seconds in time format string #############
static const char* findSecondsPlaceholder(const char* formatString) noexcept
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
[[noreturn]] static void version() noexcept
{
   std::printf("unixts2time %s\n", SYSTEMTOOLS_VERSION);
   std::exit(0);
}


// ------ BEGIN-OF-USAGE-BLOCK ------
// ###### Usage #############################################################
#define HEADER(s)    "\x1b[4m" s "\x1b[0m"
#define CMD(s)       "\x1b[1m" s "\x1b[0m"
#define OPT(s)       "\x1b[1m" s "\x1b[0m"
#define ARG(s)       "\x1b[4m" s "\x1b[0m"
#define SYN(s)       "\x1b[1m" s "\x1b[0m"
#define PIPE         SYN(" | ")
#define ENTRY(s)     "\t" SYN("[") s SYN("]\n")

[[noreturn]] static void usage(const char* program, const int exitCode) noexcept
{
   // TRANSLATORS: Command-line syntax message prefix:
   // "Usage: <program> [options]".
   std::fprintf(stderr, HEADER("%s") "\n"
           CMD("%s\n")
              ENTRY(OPT("-T") " " ARG("time_format_template") PIPE OPT("--template") " " ARG("time_format_template"))
              ENTRY(OPT("-H") PIPE OPT("--human-readable"))
              ENTRY(OPT("-s") PIPE OPT("--seconds"))
              ENTRY(OPT("-m") PIPE OPT("--milliseconds"))
              ENTRY(OPT("-u") PIPE OPT("--microseconds"))
              ENTRY(OPT("-n") PIPE OPT("--nanoseconds"))
              ENTRY(ARG("unix_timestamp") " " "...")
           CMD("%s\n")
              ENTRY(OPT("-h") PIPE OPT("--help"))
           CMD("%s\n")
              ENTRY(OPT("-v") PIPE OPT("--version")),
           gettext("Usage:"), program, program, program);
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
   std::setlocale(LC_NUMERIC, "C");   // Use "." for fractional numbers!

   // ====== Initialise i18n support ========================================
#if defined(ENABLE_NLS)
#if !defined(_WIN32)
   const char* textdomaindir     = std::getenv("TEXTDOMAINDIR");
#else
   char*       textdomaindir     = nullptr;
   size_t      textdomaindirsize = 0;
   _dupenv_s(&textdomaindir, &textdomaindirsize, "TEXTDOMAINDIR");
#endif
   bindtextdomain("unixts2time",
                  ( ((textdomaindir) && (textdomaindir[0] != 0x00)) ?
                       textdomaindir : SYSTEMTOOLS_LOCALEDIR ));
#if defined(_WIN32)
   if(textdomaindir) {
       free(textdomaindir);
   }
#endif
   bind_textdomain_codeset("unixts2time", "UTF-8");
   textdomain("unixts2time");
#endif

   // ====== Handle arguments ===============================================
   static const struct option long_options[] = {
      { "template",       required_argument, nullptr, 'T' },
      { "human-readable", no_argument,       nullptr, 'H' },
      { "seconds",        no_argument,       nullptr, 's' },
      { "milliseconds",   no_argument,       nullptr, 'm' },
      { "microseconds",   no_argument,       nullptr, 'u' },
      { "nanoseconds",    no_argument,       nullptr, 'n' },
      { "help",           no_argument,       nullptr, 'h' },
      { "version",        no_argument,       nullptr, 'v' },
      {  nullptr,         0,                 nullptr, 0   }
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
            std::fprintf(stderr, "INTERNAL ERROR: Unhandled option c=%c code=%x!\n",
                         (std::isprint(option) ? static_cast<char>(option) : ' '), option);
            return 1;
      }
   }

   // ====== Obtain Unix timestamp in ns ====================================
#if (defined(__BITINT_MAXWIDTH__) && (__BITINT_MAXWIDTH__ >= 128))
   _BitInt(128) unixTS;
#elif defined(__SIZEOF_INT128__)
   __int128_t   unixTS;
#else
   // NOTE: A 64-bit signed long long will overflow on April 11, 2262!
   long long    unixTS;
#endif
   for(int i = optind; i <= argc; i++) {
      unsigned int divideBy              = initialDivideBy;
      unsigned int inputFractionalDigits = 0;

      // ====== Use current time, if no timestamp is given ==================
      if(i == argc) {
         if(optind == argc) {
            struct timespec ts;
            if(timespec_get(&ts, TIME_UTC) != TIME_UTC) {
               // TRANSLATORS: Error message prefix when obtaining current time
               // via timespec_get() fails (system error message appended by perror).
               std::perror(gettext("timespec_get() failed"));
               std::exit(1);
            }
#if (defined(__BITINT_MAXWIDTH__) && (__BITINT_MAXWIDTH__ >= 128))
            unixTS = ((_BitInt(128))1000000000LL * ts.tv_sec) + ts.tv_nsec;
#elif defined(__SIZEOF_INT128__)
            unixTS = (static_cast<__int128_t>(1000000000LL) * ts.tv_sec) + ts.tv_nsec;
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
         // ------ Try to parse integer part --------------------------------
         char* endptr = nullptr;
         unixTS = std::strtoll(argv[i], &endptr, 0);
         if(endptr != nullptr) {
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

            // ------ Try to parse fractional part --------------------------
            if(*endptr != 0x00) {
               if(*endptr == '.') {
                  const char* p = endptr + 1;
                  while((*p >= '0') && (*p <= '9')) {
                     inputFractionalDigits++;
                     p++;
                  }
               }

               const double fractionalUnixTS = std::strtod(endptr, &endptr);
               if(endptr != nullptr) {
                  const unsigned int additionalNS =
                     static_cast<unsigned int>(fractionalUnixTS * divideBy);
                  unixTS = (unixTS >= 0) ? (unixTS + additionalNS) :
                                           (unixTS - additionalNS);
               }
            }
         }
         if( (endptr == nullptr) || (*endptr != 0x00) ) {
            // TRANSLATORS: Error message when input timestamp string is invalid.
            std::fputs(gettext("ERROR: Invalid Unix timestamp!"), stderr);
            std::fputs("\n", stderr);
            std::exit(1);
         }
      }

      // ====== Convert Unix timestamp to time ==============================
      struct timespec ts = {
         static_cast<time_t>(unixTS / 1000000000),
         static_cast<long>(unixTS % 1000000000)
      };
      if( (unixTS < 0) && (ts.tv_nsec < 0) ) {
         ts.tv_sec--;
         ts.tv_nsec = 1000000000 + ts.tv_nsec;
      }

      // ------ Time in seconds-granularity via std::chrono -----------------
      const struct tm t = timepoint_to_tm(ts.tv_sec);

      // ====== Prepare date/time string ====================================
      char frontTimeString[1024]        = { 0 };
      char secondsString[128]           = { 0 };
      char fractionalSecondsString[128] = { 0 };
      char backTimeString[1024]         = { 0 };

      const char* secondsPlaceholder = findSecondsPlaceholder(timeFormatTemplate);
      if(secondsPlaceholder != nullptr) {
         // ------ Prepare front part (before seconds) ----------------------
         const size_t frontLength = static_cast<size_t>(secondsPlaceholder - timeFormatTemplate);
         std::string frontFormatString(timeFormatTemplate, frontLength);

         if(std::strftime(frontTimeString, sizeof(frontTimeString), frontFormatString.c_str(), &t) == 0) {
            // TRANSLATORS: Error message when date/time formatting with strftime() fails:
            // %s = time format template
            std::fprintf(stderr, gettext("ERROR: strftime() with format template \"%s\" failed!"),
                         frontFormatString.c_str());
            std::fputs("\n", stderr);
            std::exit(1);
         }

         // ------ Parse the middle part (seconds) --------------------------
         if(std::strftime(secondsString, sizeof(secondsString), "%S", &t) == 0) {
            // TRANSLATORS: Error message when date/time formatting with strftime() fails:
            // %s = time format template
            std::fprintf(stderr, gettext("ERROR: strftime() with format template \"%s\" failed!"),
                         "%S");
            std::fputs("\n", stderr);
            std::exit(1);
         }

         // ------ Prepare the fractional seconds string --------------------
         unsigned int precision = 0;
         if(divideBy == 1) {
            precision = 9;
         }
         else if(divideBy == 1000) {
            precision = 6;
         }
         else if(divideBy == 1000000) {
            precision = 3;
         }
         precision += inputFractionalDigits;
         if(precision > 9) {
            precision = 9;
         }

         char fractionalSecondsFormatString[32];
         std::snprintf(fractionalSecondsFormatString, sizeof(fractionalSecondsFormatString),
                       "%%1.%uf", precision);
         std::snprintf(fractionalSecondsString, sizeof(fractionalSecondsString),
                       fractionalSecondsFormatString,
                       static_cast<double>(ts.tv_nsec) / 1000000000.0);

         // ------ Prepare the back part (after seconds) --------------------
         const char* backFormatString = secondsPlaceholder + 2;
         if(backFormatString[0] != 0x00) {
            if(std::strftime(backTimeString, sizeof(backTimeString), backFormatString, &t) == 0) {
               // TRANSLATORS: Error message when date/time formatting with strftime() fails:
               // %s = time format template
               std::fprintf(stderr, gettext("ERROR: strftime() with format template \"%s\" failed!"),
                            backFormatString);
               std::fputs("\n", stderr);
               std::exit(1);
            }
         }
      }
      else {
         if(std::strftime(frontTimeString, sizeof(frontTimeString), timeFormatTemplate, &t) == 0) {
            // TRANSLATORS: Error message when date/time formatting with strftime() fails:
            // %s = time format template
            std::fprintf(stderr, gettext("ERROR: strftime() with format template \"%s\" failed!"),
                         timeFormatTemplate);
            std::fputs("\n", stderr);
            std::exit(1);
         }
      }

      // ====== Print result ================================================
      if(!humanReadable) {
         std::fputs(frontTimeString, stdout);
         std::fputs(secondsString, stdout);
         if(fractionalSecondsString[0] != 0x00) {
            std::fputs(&fractionalSecondsString[1], stdout);
         }
         std::fputs(backTimeString, stdout);
      }
      else {
         // TRANSLATORS: Human-readable timestamp conversion output:
         // 1st %s   = formatted date/time prefix (before seconds)
         // 2nd %s   = seconds string
         // 3rd %s   = fractional seconds string
         // 4th %s   = formatted date/time suffix (after seconds)
         // 1st %lld = decimal timestamp value
         // 1st %llx = hexadecimal timestamp value
         // 5th %s   = time unit (e.g. s, ms, µs, ns)
         std::fprintf(stdout, gettext("%s%s%s%s is %lld (0x%llx) %s since the Unix Epoch"),
                      frontTimeString,
                      secondsString, (fractionalSecondsString[0] != 0x00) ? fractionalSecondsString + 1 : "",
                      backTimeString,
                      static_cast<long long>(unixTS / divideBy),
                      static_cast<unsigned long long>(unixTS / divideBy),
                      unit);
      }
      std::fputs("\n", stdout);
   }

   return 0;
}
