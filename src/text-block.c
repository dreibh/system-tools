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
// Text-Block
// Copyright (C) 2025 by Thomas Dreibholz
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

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "package-version.h"


#define DEBUG_MODE

typedef enum textblockmode {
   Extract = 1,
   Insert  = 2,
   Replace = 3,
   Remove  = 4
} textblockmode_t;



// ###### Main program ######################################################
int main (int argc, char** argv)
{
   // ====== Handle arguments ===============================================
   textblockmode_t mode            = Extract;
   const char*     inputFileName   = NULL;
   bool            openInputFile   = false;
   const char*     outputFileName  = NULL;
   bool            openOutputFile  = false;
   const char*     beginTag        = NULL;
   const char*     endTag          = NULL;
   bool            includeTags     = false;
   bool            withTagLines    = false;

   for(int i = 1; i <argc; i++) {
      if( (strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--input") == 0) ) {
         if(i + 1 < argc) {
            inputFileName = argv[i + 1];
         }
         else {
            fputs("ERROR: Missing input file name!\n", stderr);
            return 1;
         }
         i++;
      }
      else if( (strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--output") == 0) ) {
         if(i + 1 < argc) {
            outputFileName = argv[i + 1];
         }
         else {
            fputs("ERROR: Missing output file name!\n", stderr);
            return 1;
         }
         i++;
      }
      else if( (strcmp(argv[i], "-b") == 0) || (strcmp(argv[i], "--begin-tag") == 0) ) {
         if(i + 1 < argc) {
            beginTag = argv[i + 1];
         }
         else {
            fputs("ERROR: Missing begin tag!\n", stderr);
            return 1;
         }
         i++;
      }
      else if( (strcmp(argv[i], "-e") == 0) || (strcmp(argv[i], "--end-tag") == 0) ) {
         if(i + 1 < argc) {
            endTag = argv[i + 1];
         }
         else {
            fputs("ERROR: Missing end tag!\n", stderr);
            return 1;
         }
         i++;
      }
      else if( (strcmp(argv[i], "-t") == 0) || (strcmp(argv[i], "--include-tags") == 0) ) {
         includeTags = true;
      }
      else if( (strcmp(argv[i], "-x") == 0) || (strcmp(argv[i], "--exclude-tags") == 0) ) {
         includeTags = false;
      }
      else if( (strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--full-tag-lines") == 0) ) {
         withTagLines = true;
      }
      else if( (strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--tags-only") == 0) ) {
         withTagLines = false;
      }
      else if( (strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--version") == 0) ) {
         printf("printf-utf8 %s\n", SYSTEMTOOLS_VERSION);
         return 0;
      }
      else if( (strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0) ) {
         fprintf(stderr, "Usage: %s [-i|--input input_file] [-o|--output output_file] [-b|--begin-tag begin_tag] [-e|--end-tag end_tag] [-h|--help] [-v|--version]\n", argv[0]);
         return 1;
      }
      else {
         fprintf(stderr, "ERROR: Bad parameter %s!\n", argv[i]);
         return 1;
      }
   }
   if(beginTag[0] == 0x00) {
      beginTag = NULL;
   }
   if((endTag == NULL) || (endTag[0] == 0x00)) {
      endTag = beginTag;
   }
#ifdef DEBUG_MODE
   printf("Begin Tag=%s\n", beginTag);
   printf("End Tag=%s\n",   endTag);
#endif

   // ====== Open files =====================================================
   FILE* inputFile = stdin;
   if(inputFileName != NULL) {
      inputFile = fopen(inputFileName, "r");
      if(inputFile == NULL) {
         fprintf(stderr, "ERROR: Unable to open input file %s: %s\n",
                 inputFileName, strerror(errno));
         exit(1);
      }
      openInputFile = true;
   }
   FILE* outputFile = stdout;
   if(outputFileName != NULL) {
      outputFile = fopen(outputFileName, "w");
      if(outputFile == NULL) {
         fprintf(stderr, "ERROR: Unable to open output file %s: %s\n",
                 outputFileName, strerror(errno));
         exit(1);
      }
      openOutputFile = true;
   }

   // ====== Read lines from input file =====================================
   const char*        line;
   char               buffer[65536];
   unsigned long long lineNo            = 0;
   const size_t       beginTagLength    = strlen(beginTag);
   const size_t       endTagLength      = strlen(endTag);
   unsigned long long beginMarkerLineNo = 0;   // begin marker not set
   unsigned long long endMarkerLineNo   = 0;   // end marker not set

   while( (line = fgets((char*)&buffer, sizeof(buffer), inputFile)) != NULL ) {
      // ====== Process line ================================================
      lineNo++;

      const char* ptr;
      do {
         const size_t lineLength     = strlen(line);
         const char*  beginMarkerPtr = NULL;
         const char*  endMarkerPtr   = NULL;

         while( (ptr = (beginTag != NULL) ? strstr(line, (beginMarkerLineNo == 0) ? beginTag : endTag) : NULL) != NULL ) {
            // ------ Found tag ------------------------------------------------

            if(beginMarkerLineNo == 0) {
               if(includeTags) {
                  beginMarkerPtr = (withTagLines == true) ? line : ptr;
               }
               else {
                  beginMarkerPtr = (withTagLines == true) ? line : ptr + beginTagLength;
               }
               beginMarkerLineNo = lineNo;
               // puts("M-1!");
            }
            else {
               if(includeTags) {
                  endMarkerPtr = (withTagLines == true) ? line + lineLength : ptr + endTagLength;
               }
               else {
                  endMarkerPtr = (withTagLines == true) ? line + lineLength : ptr;
               }
               endMarkerLineNo = lineNo;
               // puts("M-2!");
               break;
            }

            // ------ Look for next tag ... ------------------------------------
            ptr = strstr(ptr + 1, (beginMarkerLineNo == 0) ? beginTag : endTag);
         }

         if((beginMarkerLineNo > 0) || (endMarkerLineNo > 0)) {
#ifdef DEBUG_MODE
            printf("Line %06llu:\ts=%p %llu\te=%p %llu\t↠ %s",
                   lineNo, beginMarkerPtr, beginMarkerLineNo,
                   endMarkerPtr, endMarkerLineNo, line);
#endif
            if(beginMarkerPtr != NULL) {   // Begin marker set in this line
               if(endMarkerPtr == NULL) {   // No end marker -> mark until end of line
#ifdef DEBUG_MODE
                  puts("CASE-1a: begin marker set in this line, end marker not set");
#endif
                  endMarkerPtr = line + lineLength;
               }
               else {   // End marker set
#ifdef DEBUG_MODE
                  puts("CASE-1b: begin marker and end marker set in this line");
#endif
                  // Done -> reset marking for next iteration
                  beginMarkerLineNo = 0;
                  endMarkerLineNo   = 0;
               }
            }
            else if(endMarkerLineNo > 0) {   // End marker set in this line
#ifdef DEBUG_MODE
                  puts("CASE-2: end marker set in this line");
#endif
               beginMarkerPtr = line;   // Mark from the beginning of the line
               // Done -> reset marking for next iteration
               beginMarkerLineNo = 0;
               endMarkerLineNo   = 0;
            }
            else if(beginMarkerLineNo > 0) {   // Begin marker set, but not in this line
#ifdef DEBUG_MODE
                  puts("CASE-3: begin marker set, but not in this line");
#endif
               // Continue ...
               beginMarkerPtr = line;
               endMarkerPtr   = line + lineLength;   // Mark the full line
            }

            switch(mode) {
               case Extract:
                  const ssize_t extractSize = (ssize_t)endMarkerPtr - (ssize_t)beginMarkerPtr;
                  assert(extractSize  >= 0);
                  if(extractSize > 0) {
                     if(fwrite(beginMarkerPtr, 1, extractSize, outputFile) < extractSize) {
                        fprintf(stderr, "ERROR: Unable to write to output file %s: %s\n",
                                 outputFileName, strerror(errno));
                        if(openOutputFile) {
                           fclose(outputFile);
                        }
                        if(openInputFile) {
                           fclose(inputFile);
                        }
                        exit(1);
                     }
                  }
               break;
            }

            // Advance line pointer, to look for next begin tag in the same line:
            line = (includeTags == true) ? endMarkerPtr : endMarkerPtr + endTagLength;
         }
      } while( (!withTagLines) && (ptr != NULL) );
   }

   // ====== Close files ====================================================
   if(openOutputFile) {
      if(fclose(outputFile) != 0) {
         fprintf(stderr, "ERROR: Unable to open output file %s: %s\n",
                 outputFileName, strerror(errno));
         if(openInputFile) {
            fclose(inputFile);
         }
         exit(1);
      }
      outputFile = NULL;
   }
   if(openInputFile) {
      fclose(inputFile);
      inputFile = NULL;
   }

   return 0;
}
