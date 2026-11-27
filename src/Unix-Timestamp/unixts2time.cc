// ==========================================================================
//         ____            _                     _____           _
//        / ___| _   _ ___| |_ ___ _ __ ___     |_   _|__   ___ | |___
//        \___ \| | | / __| __/ _ \ '_ ` _ \ _____| |/ _ \ / _ \| / __|
//         ___) | |_| \__ \ ||  __/ | | | | |_____| | (_) | (_) | \__ \.
//        |____/ \__, |___/\__\___|_| |_| |_|     |_|\___/ \___/|_|___/
//               |___/
//                             --- System-Tools ---
//                  [https://www.nntb.no/~dreibh/system-tools/](https://www.nntb.no/~dreibh/system-tools/)
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
// along with this program.  If not, see [http://www.gnu.org/licenses/](http://www.gnu.org/licenses/).
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
#include <filesystem>
#include <format>
#include <iostream>
#include <limits>
#include <locale>
#include <stdexcept>
#include <string>
#include <string_view>

#include <getopt.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#else
#define gettext(string)                          (string)
#define ngettext(singular, plural, count)        (((count) == 1) ? (singular) : (plural))
#define bindtextdomain(domain, directory)        (directory)
#define bind_textdomain_codeset(domain, codeset) (codeset)
#define textdomain(domain)                       (domain)
#endif

#include "package-version.h"

#if (defined(__BITINT_MAXWIDTH__) && (__BITINT_MAXWIDTH__ >= 128))
using int128_type  = _BitInt(128);
using uint128_type = unsigned _BitInt(128);
#elif defined(__SIZEOF_INT128__)
using int128_type  = __int128_t;
using uint128_type = __uint128_t;
#else
// NOTE: A 64-bit signed long long will overflow on April 11, 2262!
using int128_type  = long long;
using uint128_type = unsigned long long;
#endif


// ###### C++20 gettext runtime format helper ################################
template<typename... Args>
static std::string formatText(const char* messageIdentifier,
                              const Args&... arguments)
{
   try {
      return std::vformat(gettext(messageIdentifier),
                          std::make_format_args(arguments...));
   }
   catch(const std::format_error&) {
      return gettext(messageIdentifier);
   }
}


// ###### Convert Unix timestamp in seconds to tm via std::chrono ###########
static bool timepoint_to_tm(const int64_t unixSeconds, std::tm& result) noexcept
{
   constexpr int64_t minValidSeconds = -1096193760000LL;   // -32767-01-01T00:00:00Z
   constexpr int64_t maxValidSeconds =  1033446719999LL;   // +32767-12-31T23:59:59Z
   if((unixSeconds < minValidSeconds) || (unixSeconds > maxValidSeconds)) {
      return false;
   }

   const std::chrono::sys_seconds    tp { std::chrono::seconds { unixSeconds } };
   const std::chrono::sys_days       dp = std::chrono::floor<std::chrono::days>(tp);
   const std::chrono::year_month_day yearMonthDay { dp };
   const std::chrono::weekday        weekday { dp };
   const std::chrono::hh_mm_ss<std::chrono::seconds> time { tp - dp };

   result        = { };
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

   return true;
}


// ###### Parse integer/fractional Unix timestamp into nanoseconds ##########
static bool parseUnixTimestamp(const char*   input,
                               unsigned int& divideBy,
                               const char*&  unit,
                               unsigned int& inputFractionalDigits,
                               int128_type&  unixTS) noexcept
{
   if((input == nullptr) || (input[0] == '\0') ||
      (std::isspace(static_cast<unsigned char>(input[0])) != 0)) {
      return false;
   }

   const char* p          = input;
   bool        isNegative = false;
   if(*p == '-') {
      isNegative = true;
      ++p;
   }
   else if(*p == '+') {
      ++p;
   }
   if(*p == '\0') {
      return false;
   }

   const uint128_type maxSignedMagnitude =
      (static_cast<uint128_type>(1) << (sizeof(int128_type) * CHAR_BIT - 1)) -
      (isNegative ? 0u : 1u);

   int          base         = 10;
   const bool   hasDot       = (std::strchr(p, '.') != nullptr);
   size_t       intDigits    = 0;
   uint128_type intMagnitude = 0;

   if((p[0] == '0') && ((p[1] == 'x') || (p[1] == 'X'))) {
      if(hasDot) {
         return false;
      }
      base = 16;
      p += 2;
   }
   else if(!hasDot && (p[0] == '0') && (p[1] != '\0')) {
      base = 8;
   }

   while(*p != '\0') {
      unsigned int digit = 0;
      if((*p >= '0') && (*p <= '9')) {
         digit = static_cast<unsigned int>(*p - '0');
      }
      else if((*p >= 'a') && (*p <= 'f')) {
         digit = static_cast<unsigned int>(10 + (*p - 'a'));
      }
      else if((*p >= 'A') && (*p <= 'F')) {
         digit = static_cast<unsigned int>(10 + (*p - 'A'));
      }
      else {
         break;
      }
      if(digit >= static_cast<unsigned int>(base)) {
         return false;
      }
      if(intMagnitude > (maxSignedMagnitude - digit) / static_cast<unsigned int>(base)) {
         return false;
      }
      intMagnitude = intMagnitude * static_cast<unsigned int>(base) + digit;
      ++intDigits;
      ++p;
   }

   if(divideBy == 0) {
      if(intMagnitude < 5000000000ULL) {
         divideBy = 1000000000;
         unit     = "s";
      }
      else if(intMagnitude < 5000000000000ULL) {
         divideBy = 1000000;
         unit     = "ms";
      }
      else if(intMagnitude < 5000000000000000ULL) {
         divideBy = 1000;
         unit     = "µs";
      }
      else {
         divideBy = 1;
         unit     = "ns";
      }
   }

   unsigned int additionalNS = 0;
   if(*p == '.') {
      ++p;
      if((*p < '0') || (*p > '9')) {
         return false;
      }

      unsigned int maxFracDigits = 0;
      if(divideBy == 1000000000) {
         maxFracDigits = 9;
      }
      else if(divideBy == 1000000) {
         maxFracDigits = 6;
      }
      else if(divideBy == 1000) {
         maxFracDigits = 3;
      }

      unsigned int scaledDigits = 0;
      while((*p >= '0') && (*p <= '9')) {
         if(scaledDigits < maxFracDigits) {
            additionalNS = additionalNS * 10u + static_cast<unsigned int>(*p - '0');
            ++scaledDigits;
         }
         ++inputFractionalDigits;
         ++p;
      }
      while(scaledDigits < maxFracDigits) {
         additionalNS *= 10u;
         ++scaledDigits;
      }
   }
   else if(intDigits == 0) {
      return false;
   }

   if(*p != '\0') {
      return false;
   }

   if((intMagnitude > (maxSignedMagnitude - additionalNS) / divideBy)) {
      return false;
   }
   const uint128_type totalMagnitude = intMagnitude * divideBy + additionalNS;
   if(isNegative) {
      if(totalMagnitude == (static_cast<uint128_type>(1) << (sizeof(int128_type) * CHAR_BIT - 1))) {
         unixTS = -static_cast<int128_type>(totalMagnitude - 1u) - 1;
      }
      else {
         unixTS = -static_cast<int128_type>(totalMagnitude);
      }
   }
   else {
      unixTS = static_cast<int128_type>(totalMagnitude);
   }

   return true;
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
   std::printf("unixts2time %s\n", PROJECT_VERSION);
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

[[noreturn]]
static void usage(const char* program, const int exitCode)
{
   const char* safeProgram = (program != nullptr) ? program : "unixts2time";

   // TRANSLATORS: Text direction: LTR (Left-to-Right) or RTL (Right-to-Left)
   const bool rtl = (std::string(gettext("LTR")) == "RTL");
   const std::string manpage(std::string("\x1b[1m") +
                             std::filesystem::path(safeProgram).stem().string() +
                             std::string("\x1b[0m"));
   const std::string homepage((rtl ? std::string("\xe2\x80\x8e") : std::string("")) +
                              std::string("[\x1b[1m\x1b]8;;") +
                              std::string(PROJECT_HOMEPAGE_URL) +
                              "\a" + std::string(PROJECT_HOMEPAGE_URL) + "\a" +
                              std::string("\x1b]8;;\a\x1b[0m]") +
                              (rtl ? std::string("\xe2\x80\x8f") : std::string("")));
   const std::string repository((rtl ? std::string("\xe2\x80\x8e") : std::string("")) +
                                std::string("[\x1b[1m\x1b]8;;") +
                                std::string(PROJECT_SOURCE_URL) +
                                "\a" + std::string(PROJECT_SOURCE_URL) + "\a" +
                                std::string("\x1b]8;;\a\x1b[0m]") +
                                (rtl ? std::string("\xe2\x80\x8f") : std::string("")));

   // TRANSLATORS: Command-line syntax message prefix:
   // "Usage: <program> [options]".
   std::cerr << HEADER(gettext("Usage:")) << "\n"
             << CMD(safeProgram) << "\n"
                   ENTRY(OPT("-T") " " ARG("time_format_template") PIPE OPT("--template") " " ARG("time_format_template"))
                   ENTRY(OPT("-H") PIPE OPT("--human-readable"))
                   ENTRY(OPT("-s") PIPE OPT("--seconds"))
                   ENTRY(OPT("-m") PIPE OPT("--milliseconds"))
                   ENTRY(OPT("-u") PIPE OPT("--microseconds"))
                   ENTRY(OPT("-n") PIPE OPT("--nanoseconds"))
                   ENTRY(ARG("unix_timestamp") " " "...")
             << CMD(safeProgram) << "\n"
                   ENTRY(OPT("-h") PIPE OPT("--help"))
             << CMD(safeProgram) << "\n"
                   ENTRY(OPT("-v") PIPE OPT("--version"))
             << "\n"
             // TRANSLATORS: Informational message at the end of usage:
             // {0} = manpage name, {1} = documentation URL, {2} = repository URL
             << formatText("For further information, see the man page for {0}, read the documentation at {1}, examine the source code at {2}, or consult your favourite LLM.",
                           manpage, homepage, repository) << "\n";
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
   try {
      const std::locale userLocale(std::locale(""), std::locale::classic(), std::locale::numeric);
      std::cout.imbue(userLocale);
      std::cerr.imbue(userLocale);
   } catch(const std::runtime_error&) {
      try {
         const std::locale fallbackLocale(std::locale("C.UTF-8"), std::locale::classic(), std::locale::numeric);
         std::cout.imbue(fallbackLocale);
         std::cerr.imbue(fallbackLocale);
      } catch(const std::runtime_error&) {
         // Defaults to std::locale::classic() if C.UTF-8 is unavailable
      }
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
                  ( ((textdomaindir != nullptr) && (textdomaindir[0] != '\0')) ?
                       textdomaindir : PROJECT_LOCALEDIR ));
#if defined(_WIN32)
   if(textdomaindir != nullptr) {
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
      { nullptr,          0,                 nullptr, 0   }
   };

   int          option             = 0;
   int          longIndex          = 0;
   bool         humanReadable      = false;
   unsigned int initialDivideBy    = 0;         // auto-detect later
   const char*  unit               = nullptr;   // auto-detect later
   const char*  timeFormatTemplate = "%Y-%m-%d %H:%M:%S";
   while((option = getopt_long(argc, argv, "T:Hsmunvh", long_options, &longIndex)) != -1) {
      switch(option) {
         case 'T':
            timeFormatTemplate = optarg;
            break;
         case 'H':
            humanReadable = true;
            break;
         case 'n':
            initialDivideBy = 1;
            unit            = "ns";
            break;
         case 'u':
            initialDivideBy = 1000;
            unit            = "µs";
            break;
         case 'm':
            initialDivideBy = 1000000;
            unit            = "ms";
            break;
         case 's':
            initialDivideBy = 1000000000;
            unit            = "s";
            break;
         case 'v':
            version();
            break;
         case 'h':
         case '?':
            // Exit with 0 on h/help, exit with 1 on '?' (unknown option):
            usage(argv[0], (option == 'h') ? 0 : 1);
            break;
         default:
            // This should not happen: wrong getopt parameters, or missing case?
            std::fprintf(stderr, "INTERNAL ERROR: Unhandled option code=%x!\n",
                         static_cast<unsigned int>(option));
            return 1;
      }
   }

   // ====== Obtain Unix timestamp in ns ====================================
   for(int i = optind; i <= argc; i++) {
      int128_type  unixTS                = 0;
      unsigned int divideBy              = initialDivideBy;
      const char*  currentUnit           = unit;
      unsigned int inputFractionalDigits = 0;

      // ====== Use current time, if no timestamp is given ==================
      if(i == argc) {
         if(optind == argc) {
            const auto now = std::chrono::system_clock::now();
            const auto ns  =
               std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
            unixTS = static_cast<int128_type>(ns);
            if(divideBy == 0) {
               divideBy    = 1;
               currentUnit = "ns";
            }
         }
         else {
            break;
         }
      }

      // ====== Parse the next Unix timestamp ===============================
      else {
         if(!parseUnixTimestamp(argv[i], divideBy, currentUnit, inputFractionalDigits, unixTS)) {
            // TRANSLATORS: Error message when input timestamp string is invalid.
            std::fputs(gettext("ERROR: Invalid Unix timestamp!"), stderr);
            std::fputs("\n", stderr);
            std::exit(1);
         }
      }

      // ====== Convert Unix timestamp to time ==============================
      const int128_type rawSeconds = unixTS / 1000000000;
      if((rawSeconds < std::numeric_limits<int64_t>::min() + 1) ||
         (rawSeconds > std::numeric_limits<int64_t>::max())) {
         // TRANSLATORS: Error message when input timestamp string is invalid.
         std::fputs(gettext("ERROR: Invalid Unix timestamp!"), stderr);
         std::fputs("\n", stderr);
         std::exit(1);
      }

      int64_t tvSec     = static_cast<int64_t>(rawSeconds);
      long    rawTvNsec = static_cast<long>(unixTS % 1000000000);
      if(rawTvNsec < 0) {
         tvSec--;
         rawTvNsec += 1000000000L;
      }
      const unsigned int subsecNanoseconds =
         static_cast<unsigned int>(rawTvNsec % 1000000000L);

      // ------ Time in seconds-granularity via std::chrono -----------------
      std::tm t { };
      if(!timepoint_to_tm(tvSec, t)) {
         // TRANSLATORS: Error message when input timestamp string is invalid.
         std::fputs(gettext("ERROR: Invalid Unix timestamp!"), stderr);
         std::fputs("\n", stderr);
         std::exit(1);
      }

      // ====== Prepare date/time string ====================================
      char frontTimeString[1024]        = { 0 };
      char secondsString[128]           = { 0 };
      char fractionalSecondsString[128] = { 0 };
      char backTimeString[1024]         = { 0 };

      const char* secondsPlaceholder = findSecondsPlaceholder(timeFormatTemplate);
      if(secondsPlaceholder != nullptr) {
         // ------ Prepare front part (before seconds) ----------------------
         const size_t      frontLength = static_cast<size_t>(secondsPlaceholder - timeFormatTemplate);
         const std::string frontFormatString(timeFormatTemplate, frontLength);

         if(!frontFormatString.empty()) {
            if(std::strftime(frontTimeString, sizeof(frontTimeString), frontFormatString.c_str(), &t) == 0) {
               // TRANSLATORS: Error message when date/time formatting with strftime() fails:
               // %s = time format template
               std::fprintf(stderr, gettext("ERROR: strftime() with format template \"%s\" failed!"),
                            frontFormatString.c_str());
               std::fputs("\n", stderr);
               std::exit(1);
            }
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

         if(precision > 0) {
            char nsDigits[32];
            std::snprintf(nsDigits, sizeof(nsDigits), "%09u", subsecNanoseconds);
            nsDigits[precision] = '\0';
            std::snprintf(fractionalSecondsString, sizeof(fractionalSecondsString),
                          "0.%s", nsDigits);
         }

         // ------ Prepare the back part (after seconds) --------------------
         const char* backFormatString = secondsPlaceholder + 2;
         if(backFormatString[0] != '\0') {
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
         if(timeFormatTemplate[0] != '\0') {
            if(std::strftime(frontTimeString, sizeof(frontTimeString), timeFormatTemplate, &t) == 0) {
               // TRANSLATORS: Error message when date/time formatting with strftime() fails:
               // %s = time format template
               std::fprintf(stderr, gettext("ERROR: strftime() with format template \"%s\" failed!"),
                            timeFormatTemplate);
               std::fputs("\n", stderr);
               std::exit(1);
            }
         }
      }

      // ====== Print result ================================================
      if(!humanReadable) {
         std::fputs(frontTimeString, stdout);
         std::fputs(secondsString, stdout);
         if(fractionalSecondsString[0] != '\0') {
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
                      secondsString, (fractionalSecondsString[0] != '\0') ? fractionalSecondsString + 1 : "",
                      backTimeString,
                      static_cast<long long>(unixTS / divideBy),
                      static_cast<unsigned long long>(unixTS / divideBy),
                      currentUnit);
      }
      std::fputs("\n", stdout);
   }

   return 0;
}
