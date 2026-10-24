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

#if defined(__sun)
#define __EXTENSIONS__ 1
#endif

#include <chrono>
#include <clocale>
#include <cmath>
#include <cstdlib>
#include <format>
#include <getopt.h>
#include <iostream>
#include <random>
#include <string>
#include <thread>

#if defined(ENABLE_NLS)
#include <libintl.h>
#else
#define gettext(string)                   (string)
#define ngettext(singular, plural, count) (((count) == 1) ? (singular) : (plural))
#define bindtextdomain(domain, directory) (directory)
#define textdomain(domain)                (domain)
#endif

#include "package-version.h"


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


// ###### Version ###########################################################
[[noreturn]] static void version()
{
   std::cout << "random-sleep " << PROJECT_VERSION << '\n';
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

[[ noreturn ]]
static void usage(const char* program, const int exitCode)
{
   // TRANSLATORS: Command-line syntax message prefix:
   // "Usage: <program> [options]".
   std::cerr << HEADER(gettext("Usage:")) << "\n"
             << CMD(program) << " "
                ARG("min_delay") " " ARG("max_delay") "\n"
                   ENTRY(OPT("-q") PIPE OPT("--quiet"))
                   ENTRY(OPT("-w") PIPE OPT("--verbose"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-h") PIPE OPT("--help"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-v") PIPE OPT("--version"));
   exit(exitCode);
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
      std::locale userLocale("");
      std::cout.imbue(userLocale);
      std::cerr.imbue(userLocale);
   } catch (const std::runtime_error&) {
      try {
         std::locale fallbackLocale("C.UTF-8");
         std::cout.imbue(fallbackLocale);
         std::cerr.imbue(fallbackLocale);
      } catch (const std::runtime_error&) {
         // Defaults to std::locale::classic() if C.UTF-8 is unavailable
      }
   }
   std::setlocale(LC_NUMERIC, "C"); // Use "." for fractional numbers!

   // ====== Initialise i18n support ========================================
#if defined(ENABLE_NLS)
#if !defined(_WIN32)
   const char* textdomaindir     = std::getenv("TEXTDOMAINDIR");
#else
   char*       textdomaindir     = nullptr;
   size_t      textdomaindirsize = 0;
   _dupenv_s(&textdomaindir, &textdomaindirsize, "TEXTDOMAINDIR");
#endif
   bindtextdomain("random-sleep",
                  ( ((textdomaindir) && (textdomaindir[0] != 0x00)) ?
                       textdomaindir : PROJECT_LOCALEDIR ));
#if defined(_WIN32)
   if(textdomaindir) {
       free(textdomaindir);
   }
#endif
   bind_textdomain_codeset("random-sleep", "UTF-8");
   textdomain("random-sleep");
#endif

   // ====== Handle arguments ===============================================
   static const struct option longOptions[] = {
      { "verbose", no_argument, nullptr, 'w' },
      { "quiet",   no_argument, nullptr, 'q' },
      { "help",    no_argument, nullptr, 'h' },
      { "version", no_argument, nullptr, 'v' },
      { nullptr,   0,           nullptr, 0   }
   };

   bool verboseMode { false };
   int  option      { 0 };
   int  longIndex   { 0 };
   while((option = getopt_long(argc, argv, "wqhv", longOptions, &longIndex)) != -1) {
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
            usage(argv[0], (option == 'h') ? 0 : 1);
            break;
         default:
            std::cerr << "INTERNAL ERROR: Unhandled option!\n";
            return 1;
      }
   }
   if(optind + 2 != argc) {
      usage(argv[0], 1);
   }

   // ====== Parse delay bounds =============================================
   char*        endptrDelayMin { nullptr };
   char*        endptrDelayMax { nullptr };
   const double delayMin = std::strtod(argv[optind + 0], &endptrDelayMin);
   const double delayMax = std::strtod(argv[optind + 1], &endptrDelayMax);
   if((endptrDelayMin == argv[optind + 0]) || (*endptrDelayMin != '\0') ||
      (endptrDelayMax == argv[optind + 1]) || (*endptrDelayMax != '\0') ||
      (delayMin < 0.0) || (delayMin > delayMax) ||
      std::isnan(delayMin) || std::isnan(delayMax)) {
      // TRANSLATORS: Error message when min_delay or max_delay are negative,
      // non-numeric, or min_delay > max_delay.
      std::cerr << gettext("ERROR: Invalid min_delay/max_delay!") << '\n';
      return 1;
   }

   // ====== Random delay generation ========================================
   double delay = delayMin;
   if(delayMin < delayMax) {
      std::random_device rd;
      std::seed_seq seedSequence {
         rd(),
         static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()),
         static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count())
      };
      std::mt19937_64 rng(seedSequence);
      std::uniform_real_distribution<double> distribution(delayMin, delayMax);
      delay = distribution(rng);
   }
   if(delay < 0.0) {
      delay = 0.0;
   }

   if(verboseMode) {
      // TRANSLATORS: Verbose status message before entering sleep:
      // {0:1.6f} = sleep duration in seconds (fractional floating point)
      std::cout << formatText("Sleeping for {0:1.6f} s ...", delay) << '\n';
   }

   // ====== Sleep ==========================================================
   std::this_thread::sleep_for(std::chrono::duration<double>(delay));

   if(verboseMode) {
      // TRANSLATORS: Verbose status message displayed after sleep duration
      // completes.
      std::cout << gettext("Woke up!") << '\n';
   }

   return 0;
}
