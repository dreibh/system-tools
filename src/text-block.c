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


// #define DEBUG_MODE


typedef enum textblockmode {
   Cat,
   Enumerate,
   Highlight,
   Extract,
   InsertFront,
   InsertBack,
   Replace,
   Remove
} textblockmode_t;


static textblockmode_t    Mode               = Cat;
static const char*        BeginTag           = NULL;
static size_t             BeginTagLength     = 0;
static const char*        EndTag             = NULL;
static size_t             EndTagLength       = 0;
static bool               IncludeTags        = true;
static bool               WithTagLines       = false;
static const char*        HighlightUnmarked1 = "\x1b[34m";
static const char*        HighlightUnmarked2 = "\x1b[0m";
static const char*        HighlightMarked1   = "\x1b[31m";
static const char*        HighlightMarked2   = "\x1b[0m";
static const char*        InputFileName      = NULL;
static FILE*              InputFile          = NULL;
static bool               OpenInputFile      = NULL;
static const char*        OutputFileName     = NULL;
static FILE*              OutputFile         = NULL;
static bool               OpenOutputFile     = NULL;
static const char*        InsertFileName     = NULL;
static FILE*              InsertFile         = NULL;
static bool               InsertOnNextLine   = false;
static char*              Buffer             = NULL;
static size_t             BufferSize         = 65536;

static unsigned long long LineNo;
static const char*        EndOfLine;
static const char*        Line;
static const char*        MarkerTag;
static size_t             MarkerTagLength;
static const char*        Pointer;


// ###### Clean up ##########################################################
static void cleanUp(int exitCode)
{
   if(Buffer) {
      free(Buffer);
      Buffer = NULL;
   }

   if(InsertFile) {
      fclose(InsertFile);
      InsertFile = NULL;
   }

   if(OpenOutputFile) {
      if(fclose(OutputFile) != 0) {
         fprintf(stderr, "ERROR: Unable to open output file %s: %s\n",
                 OutputFileName, strerror(errno));
         exitCode = 1;
      }
   }
   OutputFile = NULL;

   if(OpenInputFile) {
      fclose(InputFile);
   }
   InputFile = NULL;

   exit(exitCode);
}


// ###### Write data to output file #########################################
static void writeToOutputFile(const char* data, const size_t length)
{
   if(length > 0) {
      if(fwrite(data, 1, length, OutputFile) < length) {
         fprintf(stderr, "ERROR: Unable to write to output file %s: %s\n",
                 OutputFileName, strerror(errno));
         cleanUp(1);
      }
   }
}


// ###### Write contents of insert file #####################################
static void copyInsertFileIntoOutputFile()
{
   rewind(InsertFile);

   char    buffer[65536];
   ssize_t bytesRead;
   while( (bytesRead = fread((char*)&buffer, 1, sizeof(buffer), InsertFile)) > 0 ) {
      writeToOutputFile(buffer, bytesRead);
   }
   if(bytesRead < 0) {
      fprintf(stderr, "ERROR: Unable to read to insert file %s: %s\n",
              InsertFileName, strerror(errno));
      cleanUp(1);
   }
}


// ###### Process unmarked text #############################################
static void processUnmarked(const char*   text,
                            const ssize_t textLength,
                            const bool    beginOfMarking)
{
   assert(textLength >= 0);
   switch(Mode) {
      case Cat:
      case Remove:
      case Replace:
         writeToOutputFile(text, textLength);
       break;
      case InsertFront:
         writeToOutputFile(text, textLength);
         if(beginOfMarking) {
            if(!WithTagLines) {
               copyInsertFileIntoOutputFile();
            }
            else {
               // Postpone insertion in case of WithTagLines:
               InsertOnNextLine = true;
            }
         }
       break;
      case InsertBack:
         printf("<qq%d,%d>", beginOfMarking,InsertOnNextLine);
         writeToOutputFile(text, textLength);
         if(beginOfMarking) {
            if(WithTagLines) {
               // Postpone insertion in case of WithTagLines:
               InsertOnNextLine = true;
            }
         }
         printf("<QQ>");
       break;
      case Enumerate:
         fprintf(OutputFile, "%06llu ", LineNo);
         writeToOutputFile(text, textLength);
       break;
      case Highlight:
         fputs(HighlightUnmarked1, OutputFile);
         writeToOutputFile(text, textLength);
         fputs(HighlightUnmarked2, OutputFile);
       break;
      default:
       break;
   }
}


// ###### Process marked text ###############################################
static void processMarked(const char*   text,
                          const ssize_t textLength,
                          const bool    endOfMarking)
{
   assert(textLength >= 0);
   switch(Mode) {
      case Extract:
         writeToOutputFile(text, textLength);
       break;
      case Replace:
         if(endOfMarking) {
            copyInsertFileIntoOutputFile();
         }
       break;
      case InsertFront:
         if( (InsertOnNextLine) && (text == Line) ) {
            // Apply a postponed insertion for WithTagLines:
            copyInsertFileIntoOutputFile();
            InsertOnNextLine = false;
         }
         writeToOutputFile(text, textLength);
       break;
      case InsertBack:
         printf("<ee%d,%d>", endOfMarking, InsertOnNextLine);
         if(endOfMarking) {
            if(!WithTagLines) {
               printf("<ZZZ>");
               copyInsertFileIntoOutputFile();
            }
            else {
               // Postpone insertion to next line!
               InsertOnNextLine = true;
            }
         }
         writeToOutputFile(text, textLength);
         printf("<EE>");
       break;
      case Highlight:
         fputs(HighlightMarked1, OutputFile);
         writeToOutputFile(text, textLength);
         fputs(HighlightMarked2, OutputFile);
       break;
      default:
       break;
   }
}



// ###### Main program ######################################################
int main (int argc, char** argv)
{
   // ====== Handle arguments ===============================================
   bool warnings = true;
   for(int i = 1; i <argc; i++) {
      if( (strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--cat") == 0) ) {
         Mode = Cat;
      }
      else if( (strcmp(argv[i], "-n") == 0) || (strcmp(argv[i], "--enumerate") == 0) ) {
         Mode = Enumerate;
      }
      else if( (strcmp(argv[i], "-x") == 0) || (strcmp(argv[i], "--extract") == 0) ) {
         Mode = Extract;
      }
      else if( (strcmp(argv[i], "-r") == 0) || (strcmp(argv[i], "--remove") == 0) ) {
         Mode = Remove;
      }
      else if( (strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--insert") == 0) ||
               (strcmp(argv[i], "-z") == 0) || (strcmp(argv[i], "--insert-back") == 0) ||
               (strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "--replace") == 0) ) {
         if( (strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--insert") == 0) ) {
            Mode = InsertFront;
         }
         else if( (strcmp(argv[i], "-z") == 0) || (strcmp(argv[i], "--insert-back") == 0) ) {
            Mode = InsertBack;
         }
         else {
            Mode = Replace;
         }
         if(i + 1 < argc) {
            InsertFileName = argv[i + 1];
         }
         else {
            fputs("ERROR: Missing insert file name!\n", stderr);
            return 1;
         }
         i++;
      }
      else if( (strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--input") == 0) ) {
         if(i + 1 < argc) {
            InputFileName = argv[i + 1];
         }
         else {
            fputs("ERROR: Missing input file name!\n", stderr);
            return 1;
         }
         i++;
      }
      else if( (strcmp(argv[i], "-g") == 0) || (strcmp(argv[i], "--highlight") == 0) ) {
         Mode = Highlight;
      }
      else if( (strcmp(argv[i], "-G") == 0) || (strcmp(argv[i], "--highlight-with") == 0) ) {
         if(i + 4 < argc) {
            Mode = Highlight;
            HighlightUnmarked1 = argv[i + 1];
            HighlightUnmarked2 = argv[i + 2];
            HighlightMarked1   = argv[i + 3];
            HighlightMarked2   = argv[i + 4];
         }
         else {
            fputs("ERROR: Not enough parameters for highlight-with!\n", stderr);
            return 1;
         }
         i += 4;
      }
      else if( (strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--output") == 0) ) {
         if(i + 1 < argc) {
            OutputFileName = argv[i + 1];
         }
         else {
            fputs("ERROR: Missing output file name!\n", stderr);
            return 1;
         }
         i++;
      }
      else if( (strcmp(argv[i], "-b") == 0) || (strcmp(argv[i], "--begin-tag") == 0) ) {
         if(i + 1 < argc) {
            BeginTag = argv[i + 1];
         }
         else {
            fputs("ERROR: Missing begin tag!\n", stderr);
            return 1;
         }
         i++;
      }
      else if( (strcmp(argv[i], "-e") == 0) || (strcmp(argv[i], "--end-tag") == 0) ) {
         if(i + 1 < argc) {
            EndTag = argv[i + 1];
         }
         else {
            fputs("ERROR: Missing end tag!\n", stderr);
            return 1;
         }
         i++;
      }
      else if( (strcmp(argv[i], "-t") == 0) || (strcmp(argv[i], "--include-tags") == 0) ) {
         IncludeTags = true;
      }
      else if(strcmp(argv[i], "--exclude-tags") == 0) {
         IncludeTags = false;
      }
      else if( (strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--full-tag-lines") == 0) ) {
         WithTagLines = true;
      }
      else if(strcmp(argv[i], "--tags-only") == 0) {
         WithTagLines = false;
      }
      else if( (strcmp(argv[i], "-w") == 0) || (strcmp(argv[i], "--suppress-warnings") == 0) ) {
         warnings = false;
      }
      else if( (strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--version") == 0) ) {
         printf("printf-utf8 %s\n", SYSTEMTOOLS_VERSION);
         return 0;
      }
      else if( (strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0) ) {
         fprintf(stderr, "Usage: %s [-c|--cat] [-n|--enumerate] [-g|--highlight] [-G|--highlight-with u1 u2 m1 m2] [-x|--extract] [-r|--remove] [-s|--insert insert_file] [-p|--replace insert_file] [-i|--input input_file] [-o|--output output_file] [-b|--begin-tag begin_tag] [-e|--end-tag end_tag] [-t|--include-tags] [--exclude-tags] [-f|--full-tag-lines] [--tags-only] [-w|--suppress-warnings] [-h|--help] [-v|--version]\n", argv[0]);
         return 1;
      }
      else {
         fprintf(stderr, "ERROR: Bad parameter %s!\n", argv[i]);
         return 1;
      }
   }

   // ====== Check parameters ===============================================
   if( (BeginTag != NULL) && (BeginTag[0] == 0x00) ) {
      BeginTag = NULL;
   }
   if((EndTag == NULL) || (EndTag[0] == 0x00)) {
      EndTag = BeginTag;
   }
   BeginTagLength = (BeginTag != NULL) ? strlen(BeginTag) : 0;
   EndTagLength   = (EndTag != NULL)   ? strlen(EndTag)   : 0;

   if(Mode != Highlight) {
      if( warnings && ( (HighlightUnmarked1 != NULL) || (HighlightUnmarked2 != NULL) ||
                        (HighlightMarked1 != NULL) || (HighlightMarked2 != NULL) ) ) {
         fputs("WARNING: Not in Highlight Mode but Highlight parameters set!\n", stderr);
      }
   }

   switch(Mode) {
      case Cat:
      case Enumerate:
         // Cat means 1:1 copy -> no tags!
         if( warnings && ( (BeginTag != NULL) || (EndTag != NULL) ) ) {
            fputs("WARNING: Cat or Enumerate Mode is not useful with begin/end tags!\n", stderr);
         }
         BeginTag = NULL;
         EndTag   = NULL;
       break;
      case Remove:
      case Replace:
         // Just inverse the IncludeTags option, to keep the following code simple:
         IncludeTags  = !IncludeTags;
       break;
      case InsertFront:
      case InsertBack:
         if( warnings && IncludeTags ) {
            fputs("WARNING: Insert Mode is not useful with --include-tags!\n", stderr);
         }
         IncludeTags = false;
       break;
      case Extract:
         if(BeginTag == EndTag) {
            fputs("ERROR: Extract Mode is not useful with identical begin/end tags!\n", stderr);
            return 1;
         }
       break;
      case Highlight:
       break;
   }
#ifdef DEBUG_MODE
   printf("Begin Tag=%s\n",   BeginTag);
   printf("End Tag=%s\n",     EndTag);
   printf("IncludeTags=%u\n", IncludeTags);
#endif

   // ====== Open files =====================================================
   InputFile = stdin;
   if(InputFileName != NULL) {
      InputFile = fopen(InputFileName, "r");
      if(InputFile == NULL) {
         fprintf(stderr, "ERROR: Unable to open input file %s: %s\n",
                 InputFileName, strerror(errno));
         cleanUp(1);
      }
      OpenInputFile = true;
   }
   OutputFile = stdout;
   if(OutputFileName != NULL) {
      OutputFile = fopen(OutputFileName, "w");
      if(OutputFile == NULL) {
         fprintf(stderr, "ERROR: Unable to open output file %s: %s\n",
                 OutputFileName, strerror(errno));
         cleanUp(1);
      }
      OpenOutputFile = true;
   }
   if(InsertFileName != NULL) {
      InsertFile = fopen(InsertFileName, "r");
      if(InsertFile == NULL) {
         fprintf(stderr, "ERROR: Unable to open insert file %s: %s\n",
                 InsertFileName, strerror(errno));
         cleanUp(1);
      }
   }
   else {
      if( (Mode == InsertFront) || (Mode == InsertBack) || (Mode == Replace) ) {
         fputs("ERROR: No insert file provided!\n", stderr);
         cleanUp(1);
      }
   }

   // ====== Allocate buffer ================================================
   Buffer = (char*)malloc(BufferSize);
   if(Buffer == NULL) {
      cleanUp(1);
   }

   // ====== Read lines from input file =====================================
   Line            = Buffer;
   LineNo          = 0;
   MarkerTag       = BeginTag;
   MarkerTagLength = BeginTagLength;

   char*   line       = Buffer;
   ssize_t lineLength;
   while( (lineLength = (getline((char**)&line, &BufferSize, InputFile))) > 0 ) {

      // ====== Process line ================================================
      LineNo++;
      EndOfLine = Line + lineLength;
#ifdef DEBUG_MODE
      printf("%llu (l=%u m=%s):\t%s",
             (unsigned long long)LineNo, (unsigned int)lineLength, MarkerTag,
             Line);
#endif

      if(MarkerTag != NULL) {
         Pointer = Line;
         const char* next;
         bool foundMarker = false;
         while( (next = (MarkerTag != NULL) ? strstr(Pointer, MarkerTag) : NULL) != NULL ) {
            foundMarker = true;

            // ====== Begin marker found ====================================
            if(MarkerTag == BeginTag) {
               // ------ Begin of marking with full tag lines ---------------
               if(WithTagLines) {
                  next += MarkerTagLength;
                  if(IncludeTags) {
                     processMarked(Pointer, (ssize_t)(next - Pointer), false);
                  }
                  else {
                     processUnmarked(Pointer, (ssize_t)(next - Pointer), true);
                  }
               }
               // ------ Begin of marking with tags only --------------------
               else {
                  if(!IncludeTags) {
                     next += MarkerTagLength;
                  }
                  processUnmarked(Pointer, (ssize_t)(next - Pointer), true);
               }
               MarkerTag       = EndTag;
               MarkerTagLength = EndTagLength;
            }

            // ====== End marker found ======================================
            else {
               if(WithTagLines) {
                  next += MarkerTagLength;
                  if(IncludeTags) {
                     processMarked(Pointer, (ssize_t)(next - Pointer), true);
                  }
                  else {
                     processUnmarked(Pointer, (ssize_t)(next - Pointer), false);
                  }
               }
               else {
                  if(IncludeTags) {
                     next += MarkerTagLength;
                  }
                  processMarked(Pointer, (ssize_t)(next - Pointer), true);
               }
               MarkerTag       = BeginTag;
               MarkerTagLength = BeginTagLength;
            }

            Pointer = next;
         }

         // ====== No (further) marker found ================================
         // Still need to handle the rest of the line ...
         if( (WithTagLines) && (foundMarker) ) {
            if(IncludeTags) {
               processMarked(Pointer, (ssize_t)(EndOfLine - Pointer), false);
            }
            else {
               processUnmarked(Pointer, (ssize_t)(EndOfLine - Pointer), false);
            }
         }
         else {
            if(MarkerTag == BeginTag) {
               processUnmarked(Pointer, (ssize_t)(EndOfLine - Pointer), false);
            }
            else {
               processMarked(Pointer, (ssize_t)(EndOfLine - Pointer), false);
            }
         }
      }
      else {
         processUnmarked(Line, (ssize_t)(EndOfLine - line), false);
      }

      // ====== Prepare next iteration ======================================
      line       = Buffer;
      lineLength = BufferSize;
   }
   if( (lineLength < 0) && (errno != 0) ) {
      fprintf(stderr, "ERROR: Read error: %s\n", strerror(errno));
      cleanUp(1);
   }

   // ====== Clean up =======================================================
   cleanUp(0);
}
