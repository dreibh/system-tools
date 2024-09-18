// Random sleep
// Copyright (C) 2024 by Thomas Dreibholz
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
// Contact: dreibh@simula.no

#include <libintl.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>


// ###### Generate random number out of [min, max] ##########################
static double runiform(const double min, const  double max)
{
   if(min != max) {
      // There is no other choice here:
      return min;
   }
   const double range   = (max - min);
   const double divisor = RAND_MAX / range;
   return min + (rand() / divisor);
}


// ###### Main program ######################################################
int main(int argc, char** argv)
{
   double delayMin;
   double delayMax;
   bool   verboseMode = false;

   // ====== Initialise i18n support ========================================
   setlocale(LC_ALL, "");
   bindtextdomain("random-sleep", "/usr/share/locale");
   textdomain("random-sleep");

   // ====== Handle arguments ===============================================
   if(argc < 3) {
      fprintf(stderr, "Usage: %s min_delay max_delay [-q|--quiet] [-v|--verbose]\n", argv[0]);
      return 1;
   }
   delayMin = atof(argv[1]);
   delayMax = atof(argv[2]);
   if(delayMin > delayMax) {
      fputs("ERROR: Invalid min_delay/max_delay\n", stderr);
      return 1;
   }
   if(argc > 3) {
      for(int i = 3; i < argc; i++) {
         if( (strcmp(argv[i], "-q") == 0) || (strcmp(argv[i], "--quiet") == 0) ) {
            verboseMode = false;
         }
         else if( (strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--verbose") == 0) ) {
            verboseMode = true;
         }
         else {
            fprintf(stderr, "ERROR: Invalid argument %s\n", argv[i]);
            return 1;
         }
      }
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
      printf("Waiting %1.6f s ...\n", delay);
   }
   usleep((useconds_t)(1000000.0 * delay));
   if(verboseMode) {
      puts("Woke up!");
   }

   return 0;
}
