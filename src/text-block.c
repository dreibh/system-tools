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
   Cat       = 0,
   Enumerate = 1,
   Highlight = 2,
   Extract   = 3,
   Insert    = 4,
   Replace   = 5,
   Remove    = 6
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
                            const ssize_t textLength)
{
   assert(textLength >= 0);
   switch(Mode) {
      case Cat:
         writeToOutputFile(text, textLength);
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
                          const ssize_t textLength)
{
   assert(textLength >= 0);
   switch(Mode) {
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
               (strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "--replace") == 0) ) {
         if( (strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--insert") == 0) ) {
            Mode = Insert;
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
      case Insert:
      case Replace:
         // Just inverse the IncludeTags option, to keep the following code simple:
         IncludeTags  = !IncludeTags;
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
   printf("Begin Tag=%s\n", BeginTag);
   printf("End Tag=%s\n",   EndTag);
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
      if( (Mode == Insert) || (Mode == Replace) ) {
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
   ssize_t lineLength = BufferSize;
   while( (lineLength = (getline((char**)&line, &lineLength, InputFile))) > 0 ) {

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
                     processMarked(Pointer, (ssize_t)(next - Pointer));
                  }
                  else {
                     processUnmarked(Pointer, (ssize_t)(next - Pointer));
                  }
               }
               // ------ Begin of marking with tags only --------------------
               else {
                  processUnmarked(Pointer, (ssize_t)(next - Pointer));
                  if(!IncludeTags) {
                     next += MarkerTagLength;
                  }
               }
               MarkerTag       = EndTag;
               MarkerTagLength = EndTagLength;
            }

            // ====== End marker found ======================================
            else {
               if(WithTagLines) {
                  next += MarkerTagLength;
                  if(IncludeTags) {
                     processMarked(Pointer, (ssize_t)(next - Pointer));
                  }
                  else {
                     processUnmarked(Pointer, (ssize_t)(next - Pointer));
                  }
               }
               else {
                  if(IncludeTags) {
                     next += MarkerTagLength;
                  }
                  processMarked(Pointer, (ssize_t)(next - Pointer));
                  if(!IncludeTags) {
                     next += MarkerTagLength;
                  }
               }
               MarkerTag       = BeginTag;
               MarkerTagLength = BeginTagLength;
            }

            Pointer = next;
         }

         // ====== No (further) marker found ================================
         // Still need to handle the rest of the line

         if( (WithTagLines) && (foundMarker) ) {
            if(IncludeTags) {
               processMarked(Pointer, (ssize_t)(EndOfLine - Pointer));
            }
            else {
               processUnmarked(Pointer, (ssize_t)(EndOfLine - Pointer));
            }
         }
         else {
            if(MarkerTag == BeginTag) {
               processUnmarked(Pointer, (ssize_t)(EndOfLine - Pointer));
            }
            else {
               processMarked(Pointer, (ssize_t)(EndOfLine - Pointer));
            }
         }
      }
      else {
         processUnmarked(Line, (ssize_t)(EndOfLine - line));
      }

      // ====== Prepare next iteration ======================================
      line       = Buffer;
      lineLength = BufferSize;
   }
   if( (lineLength < 0) && (errno != 0) ) {
      fprintf(stderr, "ERROR: Read error: %s\n", strerror(errno));
      cleanUp(1);
   }




   // bool marked = false;
   // const char*  MarkerTag = BeginTag;
   // const size_t markerLength;

   // while( (line = fgets((char*)&buffer, sizeof(buffer), InputFile)) != NULL ) {
      // // ====== Process line ================================================
      // lineNo++;
//       size_t lineLength = strlen(line);
//
//       while( (ptr = (markerTag != NULL) ? strstr(ptr, markerTag) : NULL) != NULL ) {
//          if(margerTag == BeginTag) {
//             processUnmarked(line, (
//             ptr +=
//          }
//       }
//
//       if(BeginTag != NULL) {
//          if(marked == false) {
//             markerTagPtr    = BeginTagPtr;
//             markerTagLength = BeginTagLength;
//          }
//          else if(marked == false) {
//             markerTagPtr    = BeginTagPtr;
//             markerTagLength = BeginTagLength;
//          }
//          ptr = strstr(ptr, markerTagPtr);
//       }
//       else {
//          ptr = NULL;
//       }
//       = (marked == false) ?
//
//
//       while( (ptr = (BeginTag != NULL) ? strstr(ptr, markerTagPtr) : NULL) != NULL ) {
//
//       }



#if 0
      const char* ptr = line;
      do {
         const size_t lineLength      = strlen(line);
         const char*  beginMarkerPtr1 = NULL;
         const char*  beginMarkerPtr2 = NULL;
         const char*  endMarkerPtr1   = NULL;
         const char*  endMarkerPtr2   = NULL;

         // ------ Look for next tag ... ------------------------------------
         while( (ptr = (BeginTag != NULL) ? strstr(ptr, (beginMarkerLineNo == 0) ? BeginTag : EndTag) : NULL) != NULL ) {
            // ------ Found tag ---------------------------------------------

            if(beginMarkerLineNo == 0) {
               // Set begin marker
               beginMarkerPtr1   = ptr;
               beginMarkerPtr2   = beginMarkerPtr1 + BeginTagLength;
               beginMarkerLineNo = lineNo;
               // ------ Usual case: different tags for begin and end: ------
               if(BeginTag != EndTag) {
#ifdef DEBUG_MODE
                  puts("M-1!");
#endif
                  ptr = beginMarkerPtr2;
               }
               // ------ Special case: identical tag for begin/end: ---------
               else {
                  if(IncludeTags) {
                     // There is no end marker
                     // => set both pointers to end of begin marker:
                     endMarkerPtr1 = beginMarkerPtr2;
                     endMarkerPtr2 = endMarkerPtr1;
                  }
                  else {
                     // Set end marker
                     endMarkerPtr1 = ptr;
                     endMarkerPtr2 = endMarkerPtr1 + EndTagLength;
                  }
                  endMarkerLineNo = lineNo;
#ifdef DEBUG_MODE
                  puts("M-1/2!");
#endif
                  ptr = endMarkerPtr2;
                  break;
               }
            }
            else {
               endMarkerPtr1   = ptr;
               endMarkerPtr2   = ptr + EndTagLength;
               endMarkerLineNo = lineNo;
#ifdef DEBUG_MODE
               puts("M-2!");
#endif
               ptr += EndTagLength;
               break;
            }
         }

         // ====== Handle marked part of line ===============================
         if((beginMarkerLineNo > 0) || (endMarkerLineNo > 0)) {
         // if( (beginMarkerPtr1) || (endMarkerPtr1) ) {

            // ------
            if(beginMarkerPtr1 == NULL) {
               beginMarkerPtr1 = line;
               beginMarkerPtr2 = beginMarkerPtr1;
            }

            if(endMarkerPtr1 == NULL) {
               endMarkerPtr1 = line + lineLength;
               endMarkerPtr2 = endMarkerPtr1;
            }

            puts("qqq");
#if 0
#ifdef DEBUG_MODE
            printf("Line %06llu:\tb=%p %llu\te=%p %llu\t↠ %s",
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
#endif

            printf("L%d=%s", lineNo, line);

            switch(Mode) {
               case Extract:
                  puts("e-1");
                  if( (WithTagLines) &&
                      (IncludeTags) &&
                         (beginMarkerLineNo == lineNo)) {
                        puts("e-3");
                        writeToOutputFile(line, (size_t)lineLength);
                     // }
                  }
                  else {
                     puts("e-20");
                     if(IncludeTags) {
                        puts("e-21");
                        printf("B=%s\n", beginMarkerPtr1);
                        printf("E=%s\n", endMarkerPtr2);
                        writeToOutputFile(beginMarkerPtr1, (ssize_t)(endMarkerPtr2 - beginMarkerPtr1));
                     }
                     else {
                        puts("e-22");
                        writeToOutputFile(beginMarkerPtr2, (ssize_t)(endMarkerPtr1 - beginMarkerPtr2));
                     }
                  }
/*
                  {
                     const char* extractPtr;
                     ssize_t     extractSize;
                     if(WithTagLines) {
                        extractPtr  = line;
                        extractSize = lineLength;
                     }
                     else {
                        const ssize_t extractSize =
                           (IncludeTags == true) ? ((ssize_t)endMarkerPtr2 - (ssize_t)beginMarkerPtr1)

                        (ssize_t)endMarkerPtr - (ssize_t)beginMarkerPtr;
                     }
                     const ssize_t extractSize = (ssize_t)endMarkerPtr - (ssize_t)beginMarkerPtr;

                     assert(extractSize >= 0);
                     writeToOutputFile(beginMarkerPtr, (size_t)extractSize);*/
                  // }
                break;
               case Remove:
               case Insert:
               case Replace:
                  {
                     // if( (!IncludeTags) && (WithTagLines) && (beginMarkerLineNo == lineNo) ) {
                     //    writeToOutputFile(line, (size_t)lineLength);
                     // }
                     // else {
                     //    const ssize_t extractSize = (ssize_t)(beginMarkerPtr - line);
                     //    assert(extractSize >= 0);
                     //    writeToOutputFile(line, (size_t)extractSize);
                     // }
                     //
                     // if( (Mode == Insert) || (Mode == Replace) ) {
                     //    if(beginMarkerLineNo == lineNo) {   // Only insert on begin marker's line!
                     //       copyInsertFileIntoOutputFile();
                     //    }
                     //    if(Mode == Insert) {
                     //       const ssize_t extractSize = (ssize_t)(endMarkerPtr - beginMarkerPtr);
                     //       assert(extractSize >= 0);
                     //       writeToOutputFile(beginMarkerPtr, extractSize);
                     //    }
                     // }
                     //
                     // if(beginMarkerLineNo == 0) {   // End marker is in this line!
                     //    if(!IncludeTags == true) {
                     //       if(WithTagLines) {
                     //          writeToOutputFile(line, (size_t)lineLength);
                     //       }
                     //       else {
                     //          const ssize_t extractSize = (ssize_t)(endMarkerPtr - line);
                     //          assert(extractSize >= 0);
                     //          writeToOutputFile(EndTag, EndTagLength);
                     //       }
                     //    }
                     // }
                  }
                break;
               default:
                  // Nothing to do here!
                break;
            }

            // Advance line pointer, to look for next begin tag in the same line:
            line = endMarkerPtr2;
         }

         // ====== Handle unmarked line =====================================
         else {
            switch(Mode) {
               case Cat:
               case Remove:
               case Replace:
               case Insert:
                  writeToOutputFile(line, lineLength);
                break;
               default:
                  // Nothing to do here!
                break;
            }
         }

      } while( (!WithTagLines) && (ptr != NULL) );
#endif
   // }

   // ====== Clean up =======================================================
   cleanUp(0);
}
