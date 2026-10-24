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

#if (defined(__BITINT_MAXWIDTH__) && (__BITINT_MAXWIDTH__ >= 128))
using int128_type = _BitInt(128);
#elif defined(__SIZEOF_INT128__)
using int128_type = __int128_t;
#else
using int128_type = long long;
#endif


// ###### Convert tm to Unix timestamp in seconds via std::chrono ###########
static int64_t tm_to_timepoint(const std::tm& t) noexcept
{
   const std::chrono::year           year { t.tm_year + 1900 };
   const std::chrono::month          month { static_cast<unsigned>(t.tm_mon + 1) };
   const std::chrono::day            day { static_cast<unsigned>(t.tm_mday) };
   const std::chrono::year_month_day yearMonthDay { year, month, day };
   const std::chrono::sys_days       days { yearMonthDay };

   const std::chrono::seconds totalSeconds = days.time_since_epoch() +
                                                std::chrono::hours { t.tm_hour } +
                                                std::chrono::minutes { t.tm_min } +
                                                std::chrono::seconds { t.tm_sec };
   return totalSeconds.count();
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


// ###### Print integer value ###############################################
static void printBigInteger(
   int128_type value,
   const int   base,
   const bool  withPrefix) noexcept
{
   if(value < 0) {
      std::putchar('-');
   }
   if(withPrefix && (base == 16)) {
      std::fputs("0x", stdout);
   }

   char buffer[64];
   int  idx = 0;
   if(value < 0) {   // Negative value
      do {
         int remainder = static_cast<int>(value % base);
         if(remainder < 0) {
            remainder = -remainder;
         }
         buffer[idx++] = static_cast<char>((remainder < 10) ? (remainder + '0') : (remainder - 10 + 'a'));
         value /= base;
      } while(value < 0);
   }
   else {            // Non-negative value
      do {
         const int remainder = static_cast<int>(value % base);
         buffer[idx++] = static_cast<char>((remainder < 10) ? (remainder + '0') : (remainder - 10 + 'a'));
         value /= base;
      } while(value > 0);
   }

   while(idx > 0) {
      std::putchar(buffer[--idx]);
   }
}


#if defined(_WIN32)
// ###### strptime() implementation for Windows #############################
static const char* strptime(const char* buffer,
                            const char* format,
                            std::tm*    tm) noexcept
{
   if( (!buffer) || (!format) || (!tm) ) {
      return nullptr;
   }
   while(*format) {
      if(std::isspace(static_cast<unsigned char>(*format))) {
         while(std::isspace(static_cast<unsigned char>(*buffer))) {
            buffer++;
         }
         format++;
         continue;
      }
      if(*format == '%') {
         format++;
         if(*format == '%') {
            if(*buffer != '%') {
               return nullptr;
            }
            buffer++;
            format++;
         }
         else if(*format == 'Y') {
            int year   = 0;
            int digits = 0;
            while( (digits < 4) &&
                   std::isdigit(static_cast<unsigned char>(*buffer)) ) {
               year = year * 10 + (*buffer - '0');
               buffer++;
               digits++;
            }
            if(digits == 0) {
               return nullptr;
            }
            tm->tm_year = year - 1900;
            format++;
         }
         else if(*format == 'm') {
            int mon    = 0;
            int digits = 0;
            while( (digits < 2) &&
                   std::isdigit(static_cast<unsigned char>(*buffer)) ) {
               mon = mon * 10 + (*buffer - '0');
               buffer++;
               digits++;
            }
            if(digits == 0) {
               return nullptr;
            }
            tm->tm_mon = mon - 1;
            format++;
         }
         else if(*format == 'd' || *format == 'e') {
            int day    = 0;
            int digits = 0;
            while( (digits < 2) &&
                   std::isdigit(static_cast<unsigned char>(*buffer)) ) {
               day = day * 10 + (*buffer - '0');
               buffer++;
               digits++;
            }
            if(digits == 0) {
               return nullptr;
            }
            tm->tm_mday = day;
            format++;
         }
         else if(*format == 'H') {
            int hour   = 0;
            int digits = 0;
            while( (digits < 2) &&
                   std::isdigit(static_cast<unsigned char>(*buffer)) ) {
               hour = hour * 10 + (*buffer - '0');
               buffer++;
               digits++;
            }
            if(digits == 0) {
               return nullptr;
            }
            tm->tm_hour = hour;
            format++;
         }
         else if(*format == 'M') {
            int min    = 0;
            int digits = 0;
            while( (digits < 2) &&
                   std::isdigit(static_cast<unsigned char>(*buffer)) ) {
               min = min * 10 + (*buffer - '0');
               buffer++;
               digits++;
            }
            if(digits == 0) {
               return nullptr;
            }
            tm->tm_min = min;
            format++;
         }
         else if(*format == 'S') {
            int sec    = 0;
            int digits = 0;
            while( (digits < 2) &&
                   std::isdigit(static_cast<unsigned char>(*buffer)) ) {
               sec = sec * 10 + (*buffer - '0');
               buffer++;
               digits++;
            }
            if(digits == 0) {
               return nullptr;
            }
            tm->tm_sec = sec;
            format++;
         }
         else {
            return nullptr;
         }
      } else {
         if(*buffer != *format) {
            return nullptr;
         }
         buffer++;
         format++;
      }
   }
   return buffer;
}
#endif


// ###### Version ###########################################################
[[noreturn]] static void version() noexcept
{
   std::printf("time2unixts %s\n", PROJECT_VERSION);
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
              ENTRY(OPT("-F") PIPE OPT("--float"))
              ENTRY(OPT("-I") PIPE OPT("--integer-decimal"))
              ENTRY(OPT("-X") PIPE OPT("--integer-hexadecimal"))
              ENTRY(OPT("-0") PIPE OPT("--integer-0x-hexadecimal"))
              ENTRY(OPT("-H") PIPE OPT("--human-readable"))
              ENTRY(OPT("-s") PIPE OPT("--seconds"))
              ENTRY(OPT("-m") PIPE OPT("--milliseconds"))
              ENTRY(OPT("-u") PIPE OPT("--microseconds"))
              ENTRY(OPT("-n") PIPE OPT("--nanoseconds"))
              ENTRY(ARG("time_string") " " "...")
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
   bindtextdomain("time2unixts",
                  ( ((textdomaindir) && (textdomaindir[0] != 0x00)) ?
                       textdomaindir : PROJECT_LOCALEDIR ));
#if defined(_WIN32)
   if(textdomaindir) {
       free(textdomaindir);
   }
#endif
   bind_textdomain_codeset("time2unixts", "UTF-8");
   textdomain("time2unixts");
#endif

   // ====== Handle arguments ===============================================
   static const struct option long_options[] = {
      { "template",               required_argument, nullptr, 'T' },
      { "float",                  no_argument,       nullptr, 'F' },
      { "integer-decimal",        no_argument,       nullptr, 'I' },
      { "integer-hexadecimal",    no_argument,       nullptr, 'X' },
      { "integer-0x-hexadecimal", no_argument,       nullptr, '0' },
      { "human-readable",         no_argument,       nullptr, 'H' },
      { "seconds",                no_argument,       nullptr, 's' },
      { "milliseconds",           no_argument,       nullptr, 'm' },
      { "microseconds",           no_argument,       nullptr, 'u' },
      { "nanoseconds",            no_argument,       nullptr, 'n' },
      { "help",                   no_argument,       nullptr, 'h' },
      { "version",                no_argument,       nullptr, 'v' },
      {  nullptr,                 0,                 nullptr, 0   }
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
            std::fprintf(stderr, "INTERNAL ERROR: Unhandled option c=%c code=%x!\n",
                         (std::isprint(option) ? static_cast<char>(option) : ' '), option);
            return 1;
      }
   }

   // ====== Obtain Unix timestamp in ns ====================================
   for(int i = optind; i <= argc; i++) {
      int128_type unixTS = 0;

      // ====== Use current time, if no date/time string is given ===========
      if(i == argc) {
         if(optind == argc) {
            const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            const int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
            unixTS = static_cast<int128_type>(ns);
         }
         else {
            break;
         }
      }

      // ====== Parse the next date/time string =============================
      else {
         std::tm     t                  = { };
         const char* timeString         = argv[i];
         const char* remainder          = nullptr;
         const char* secondsPlaceholder = findSecondsPlaceholder(timeFormatTemplate);
         if(secondsPlaceholder != nullptr) {
            long long nanoseconds = 0;

            // ------ Parse front part (before seconds) ---------------------
            const size_t frontLength = static_cast<size_t>(secondsPlaceholder - timeFormatTemplate);
            const std::string frontFormatString(timeFormatTemplate, frontLength);

            const char* remainder1 = strptime(timeString, frontFormatString.c_str(), &t);

            // ------ Parse the middle part (seconds) -----------------------
            if(remainder1 != nullptr) {
               char*  remainder2 = nullptr;
               double seconds    = std::strtod(remainder1, &remainder2);
               if( (remainder2 != remainder1) && (seconds >= 0.0) ) {
                  // Split seconds into integer seconds and nanoseconds:
                  t.tm_sec = static_cast<int>(seconds);
                  const double fraction = seconds - static_cast<double>(t.tm_sec);
                  nanoseconds = static_cast<long long>(fraction * 1e9 + 0.5);   // +0.5 for rounding safety
                  if(nanoseconds >= 1000000000LL) {
                     t.tm_sec += 1;
                     nanoseconds -= 1000000000LL;
                  }

                  // ------ Parse the back part (after seconds) -------------
                  const char* backFormatString = secondsPlaceholder + 2;
                  if(backFormatString[0] != 0x00) {
                     remainder = strptime(remainder2, backFormatString, &t);
                  } else {
                     remainder = remainder2;
                  }
               }
            }

            const int64_t secondsSinceEpoch = tm_to_timepoint(t);
            unixTS = (static_cast<int128_type>(1000000000LL) * secondsSinceEpoch) + nanoseconds;
         }
         else {
            remainder = strptime(argv[i], timeFormatTemplate, &t);
            const int64_t secondsSinceEpoch = tm_to_timepoint(t);
            unixTS = (static_cast<int128_type>(1000000000LL) * secondsSinceEpoch);
         }

         if( (remainder == nullptr) || (*remainder != 0x00) ) {
            // TRANSLATORS: Error message when parsing time string fails:
            // 1st %s = input time string
            // 2nd %s = time format template
            std::fprintf(stderr, gettext("ERROR: Unable to parse time string \"%s\" with format template \"%s\"!\n"),
                         timeString, timeFormatTemplate);
            std::exit(1);
         }
      }

      // ====== Convert timespec to Unix timestamp ==========================
      if(useInteger == 16) {
         printBigInteger(unixTS / divideBy, 16, false);
      }
      else if(useInteger == -16) {
         printBigInteger(unixTS / divideBy, 16, true);
      }
      else {
         printBigInteger(unixTS / divideBy, 10, false);
         if(useInteger == 0) {
            const int128_type rawFractional = unixTS - (unixTS / divideBy) * divideBy;
            const unsigned int fractionalUnixTS =
               static_cast<unsigned int>(rawFractional < 0 ? -rawFractional : rawFractional);
            const char* format;
            if(divideBy == 1000000000) {
               format = ".%09u";
            }
            else if(divideBy == 1000000) {
               format = ".%06u";
            }
            else if(divideBy == 1000) {
               format = ".%03u";
            }
            else {
               format = ".%u";
            }
            std::printf(format, fractionalUnixTS);
         }
      }
      if(humanReadable) {
         // TRANSLATORS: Suffix appended to human-readable timestamp output:
         // %s = time unit (e.g. s, ms, µs, ns)
         std::printf(gettext(" %s since the Unix Epoch"), unit);
      }
      std::fputs("\n", stdout);
   }

   return 0;
}
