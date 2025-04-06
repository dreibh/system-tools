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
// Random-Sleep
// Copyright (C) 2024-2025 by Thomas Dreibholz
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
#include <sys/time.h>
#include <unistd.h>

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
static void version()
{
   printf("random-sleep %s\n", SYSTEMTOOLS_VERSION);
   exit(0);
}


// ###### Usage #############################################################
static void usage(const char* program, const int exitCode)
{
   fprintf(stderr, "%s %s min_delay max_delay [-q|--quiet] [-w|--verbose] [-h|--help] [-v|--version]\n",
           gettext("Usage:"), program);
   exit(exitCode);
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   double delayMin = -1.0;
   double delayMax = -1.0;
   bool   verboseMode = false;

   // ====== Initialise i18n support ========================================
   if(setlocale(LC_ALL, "") == NULL) {
      setlocale(LC_ALL, "C.UTF-8");   // "C" should exist on all systems!
   }
   bindtextdomain("random-sleep", NULL);
   textdomain("random-sleep");

   // ====== Handle arguments ===============================================
   const static struct option long_options[] = {
      { "verbose", no_argument, 0, 'w' },
      { "quiet",   no_argument, 0, 'q' },
      { "help",    no_argument, 0, 'h' },
      { "version", no_argument, 0, 'v' },
      {  NULL,     0,           0, 0   }
   };

   if(optind + 1 < argc) {
      delayMin = atof(argv[optind + 0]);
      delayMax = atof(argv[optind + 1]);
      optind += 2;
   }

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
            usage(argv[0], 0);
          break;
         default:
            fprintf(stderr, gettext("ERROR: Invalid argument %s!"), argv[optind - 1]);
            return 1;
          break;
      }
   }
   if(optind < argc) {
      usage(argv[0], 1);
   }
   if( (delayMin < 0.0) || (delayMin > delayMax) ) {
      fputs(gettext("ERROR: Invalid min_delay/max_delay"), stderr);
      fputs("\n", stderr);
      return 1;
   }

   // ====== Initialise random number generator =============================
   struct timeval tv;
   if(gettimeofday(&tv, NULL) == 0) {
      srand(tv.tv_usec);
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
   usleep((useconds_t)(1000000.0 * delay));
   if(verboseMode) {
      puts(gettext("Woke up!"));
   }

   return 0;
}
