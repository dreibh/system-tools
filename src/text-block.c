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
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <libintl.h>
#include <locale.h>
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
   Discard,
   Enumerate,
   Highlight,
   Extract,
   InsertFront,
   InsertBack,
   Replace,
   Remove
} textblockmode_t;


static textblockmode_t Mode                 = Cat;
static const char*     BeginTag             = nullptr;
static size_t          BeginTagLength       = 0;
static const char*     EndTag               = nullptr;
static size_t          EndTagLength         = 0;
static long long       SelectBegin          = 0;
static long long       SelectEnd            = 0;
static long long       TotalInputLines      = -1;
static bool            IncludeTags          = false;
static bool            FullTagLines         = false;
static char            EnumerateFormat[128] = "%06llu";
static const char*     Enumerate1           = "\e[36m";
static const char*     Enumerate2           = "\e[0m ";
static const char*     HighlightBegin       = "⭐";
static const char*     HighlightEnd         = "🛑";
static const char*     HighlightUnmarked1   = "\e[34m";
static const char*     HighlightUnmarked2   = "\e[0m";
static const char*     HighlightMarked1     = "\e[31m";
static const char*     HighlightMarked2     = "\e[0m";
static const char*     InputFileName        = nullptr;
static FILE*           InputFile            = nullptr;
static bool            OpenInputFile        = false;
static const char*     OutputFileName       = nullptr;
static FILE*           OutputFile           = nullptr;
static bool            OpenOutputFile       = false;
static bool            OpenOutputAppend     = false;
static const char*     InsertFileName       = nullptr;
static FILE*           InsertFile           = nullptr;
static bool            InMarkedBlock        = false;
static char*           Buffer               = nullptr;
static size_t          BufferSize           = 65536;

static long long       LineNo;
static const char*     EndOfLine;
static const char*     Line;
static const char*     MarkerTag;
static size_t          MarkerTagLength;
static const char*     Pointer;


// ###### Clean up ##########################################################
[[ noreturn ]] static void cleanUp(int exitCode)
{
   if(InsertFile) {
      fclose(InsertFile);
      InsertFile = nullptr;
   }

   if(OpenOutputFile) {
      if(fclose(OutputFile) != 0) {
         fprintf(stderr, gettext("ERROR: Unable to open output file %s: %s"),
                 OutputFileName, strerror(errno));
         fputs("\n", stderr);
         exitCode = 1;
      }
   }
   OutputFile = nullptr;

   if(OpenInputFile) {
      fclose(InputFile);
   }
   InputFile = nullptr;

   if(Buffer) {
      free(Buffer);
      Buffer = nullptr;
   }

   exit(exitCode);
}


// ###### Write data to output file #########################################
static void writeToOutputFile(const char* data, const size_t length)
{
   if(length > 0) {
      if(fwrite(data, 1, length, OutputFile) < length) {
         fprintf(stderr, gettext("ERROR: Unable to write to output file %s: %s"),
                 OutputFileName, strerror(errno));
         fputs("\n", stderr);
         cleanUp(1);
      }
   }
}


// ###### Count lines of input file #########################################
static long long countLines(FILE* inputFile)
{
   long long lines = 0;

   char* line = Buffer;
   while( (getline((char**)&line, &BufferSize, inputFile)) > 0 ) {
      lines++;
      line = Buffer;
   }
   rewind(inputFile);

   return lines;
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
      fprintf(stderr, gettext("ERROR: Unable to read to insert file %s: %s"),
              InsertFileName, strerror(errno));
      fputs("\n", stderr);
      cleanUp(1);
   }
}


// ###### Process unmarked text #############################################
static void processUnmarked(const char*   text,
                            const ssize_t textLength,
                            const bool    beginOfMarking)
{
   assert(InMarkedBlock == false);
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
            copyInsertFileIntoOutputFile();
         }
       break;
      case InsertBack:
         writeToOutputFile(text, textLength);
       break;
      case Enumerate:
         fputs(Enumerate1, OutputFile);
         fprintf(OutputFile, EnumerateFormat, LineNo);
         fputs(Enumerate2, OutputFile);
         writeToOutputFile(text, textLength);
       break;
      case Highlight:
         if(textLength > 0) {
            fputs(HighlightUnmarked1, OutputFile);
            writeToOutputFile(text, textLength);
            fputs(HighlightUnmarked2, OutputFile);
         }
         if(beginOfMarking) {
            fputs(HighlightBegin, OutputFile);
         }
       break;
      default:
       break;
   }

   if(beginOfMarking) {
      InMarkedBlock = true;
   }
}


// ###### Process marked text ###############################################
static void processMarked(const char*   text,
                          const ssize_t textLength,
                          const bool    endOfMarking)
{
   assert(InMarkedBlock == true);
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
         writeToOutputFile(text, textLength);
       break;
      case InsertBack:
         writeToOutputFile(text, textLength);
         if(endOfMarking) {
            copyInsertFileIntoOutputFile();
         }
       break;
      case Highlight:
         if(textLength > 0) {
            fputs(HighlightMarked1, OutputFile);
            writeToOutputFile(text, textLength);
            fputs(HighlightMarked2, OutputFile);
         }
         if(endOfMarking) {
            fputs(HighlightEnd, OutputFile);
         }
       break;
      default:
       break;
   }

   if(endOfMarking) {
      InMarkedBlock = false;
   }
}


// ###### Version ###########################################################
[[ noreturn ]] static void version()
{
   printf("text-block %s\n", SYSTEMTOOLS_VERSION);
   exit(0);
}


// ###### Usage #############################################################
[[ noreturn ]] static void usage(const char* program, const int exitCode)
{
   fprintf(stderr, "%s %s [-C|--cat] [-0|--discard] [-H|--highlight] [-E|--enumerate] [-X|--extract] [-D|--delete|--remove] [-F|--insert-front insert_file] [-B|--insert-back insert_file] [-R|--replace insert_file] [-i|--input input_file] [-o|--output output_file] [-a|--append] [-s|--select from_line to_line] [-b|--begin-tag begin_tag] [-e|--end-tag end_tag] [-y|--include-tags] [-x|--exclude-tags] [-f|--full-tag-lines] [-g|--tags-only] [--highlight-[begin|end|unmarked1|unmarked2|marked1|marked2] label] [--highlight-param begin_label end_label unmarked1_label unmarked2_label marked1_label marked2_label] [--enumerate-format format] [--enumerate-label[1|2] string] [-w|--suppress-warnings] [-h|--help] [-v|--version]\n",
           gettext("Usage:"), program);
   exit(exitCode);
}



// ###### Main program ######################################################
int main (int argc, char** argv)
{
   // ====== Initialise i18n support ========================================
   if(setlocale(LC_ALL, "") == nullptr) {
      setlocale(LC_ALL, "C.UTF-8");   // "C" should exist on all systems!
   }
   bindtextdomain("text-block", nullptr);
   textdomain("text-block");

   // ====== Handle arguments ===============================================
   bool showWarnings = true;
   const static struct option long_options[] = {
      { "cat",                 no_argument,       0, 'C' },
      { "discard",             no_argument,       0, '0' },
      { "highlight",           no_argument,       0, 'H' },
      { "enumerate",           no_argument,       0, 'E' },
      { "extract",             no_argument,       0, 'X' },
      { "delete",              no_argument,       0, 'D' },
      { "remove",              no_argument,       0, 'D' },   // Alias for "delete"
      { "insert-front",        required_argument, 0, 'F' },
      { "insert-back",         required_argument, 0, 'B' },
      { "replace",             required_argument, 0, 'R' },

      { "input",               required_argument, 0, 'i' },
      { "output",              required_argument, 0, 'o' },
      { "append",              no_argument,       0, 'a' },

      { "select",              required_argument, 0, 's' },
      { "begin-tag",           required_argument, 0, 'b' },
      { "end-tag",             required_argument, 0, 'e' },
      { "tag",                 required_argument, 0, 't' },
      { "exclude-tags",        no_argument,       0, 'x' },
      { "include-tags",        no_argument,       0, 'y' },
      { "full-tag-lines",      no_argument,       0, 'f' },
      { "tags-only",           no_argument,       0, 'g' },

      { "enumerate-format",    required_argument, 0, 0x1000 },
      { "enumerate-label1",    required_argument, 0, 0x1001 },
      { "enumerate-label2",    required_argument, 0, 0x1002 },
      { "highlight-begin",     required_argument, 0, 0x2000 },
      { "highlight-end",       required_argument, 0, 0x2001 },
      { "highlight-unmarked1", required_argument, 0, 0x2002 },
      { "highlight-unmarked2", required_argument, 0, 0x2003 },
      { "highlight-marked1",   required_argument, 0, 0x2004 },
      { "highlight-marked2",   required_argument, 0, 0x2005 },
      { "highlight-params",    required_argument, 0, 0x2006 },

      { "suppress-warnings",   no_argument,       0, 'q' },
      { "help",                no_argument,       0, 'h' },
      { "version",             no_argument,       0, 'v' },
      {  nullptr,                 0,                 0, 0   }
   };

   int option;
   int longIndex;
   while( (option = getopt_long(argc, argv, "C0HEXDF:B:R:i:o:a:b:e:t:s:xyfgqhv", long_options, &longIndex)) != -1 ) {
      switch(option) {
         case 'C':
            Mode = Cat;
          break;
         case '0':
            Mode = Discard;
          break;
         case 'H':
            Mode = Highlight;
          break;
         case 'E':
            Mode = Enumerate;
          break;
         case 'X':
            Mode = Extract;
          break;
         case 'D':
            Mode = Remove;
          break;
         case 'F':
            Mode           = InsertFront;
            InsertFileName = optarg;
          break;
         case 'B':
            Mode           = InsertBack;
            InsertFileName = optarg;
          break;
         case 'R':
            Mode           = Replace;
            InsertFileName = optarg;
          break;
         case 'i':
            InputFileName  = optarg;
          break;
         case 'o':
            OutputFileName = optarg;
          break;
         case 'a':
            OpenOutputAppend = true;
          break;
         case 's':
            if(optind < argc) {
               SelectBegin = atoll(argv[optind - 1]);
               SelectEnd   = atoll(argv[optind]);
               optind++;
            }
            else {
               fputs(gettext("ERROR: Too few arguments for select!"), stderr);
               fputs("\n", stderr);
               return 1;
            }
          break;
         case 'b':
            BeginTag = optarg;
          break;
         case 'e':
            EndTag = optarg;
          break;
         case 't':
            BeginTag = optarg;
            EndTag   = nullptr;
          break;
         case 'x':
            IncludeTags = false;
          break;
         case 'y':
            IncludeTags = true;
          break;
         case 'f':
            FullTagLines = true;
          break;
         case 'g':
            FullTagLines = false;
          break;
         case 'q':
            showWarnings = false;
          break;
         case 0x1000:
            for(unsigned int i = 0; i < strlen(optarg); i++) {
               if( (optarg[i] == '%') ||
                   !( (isalnum(optarg[i]) || (optarg[i] != '-') || (optarg[i] != '\'') ) ) ) {
                  fputs(gettext("ERROR: Invalid value for enumeration-format!"), stderr);
                  fputs("\n", stderr);
                  return 1;
               }
            }
            snprintf((char*)&EnumerateFormat, sizeof(EnumerateFormat), "%%%sllu", optarg);
          break;
         case 0x1001:
            Enumerate1 = optarg;
          break;
         case 0x1002:
            Enumerate2 = optarg;
          break;
         case 0x2000:
            HighlightBegin = optarg;
          break;
         case 0x2001:
            HighlightEnd = optarg;
          break;
         case 0x2002:
            HighlightUnmarked1 = optarg;
          break;
         case 0x2003:
            HighlightUnmarked2 = optarg;
          break;
         case 0x2004:
            HighlightMarked1 = optarg;
          break;
         case 0x2005:
            HighlightMarked2 = optarg;
          break;
         case 0x2006:
            // NOTE: optind already points to the next option here!
            //       For options with two or more parameters, this
            //       will be the *second* parameter!
            if(optind + 4 < argc) {
               // NOTE: optind points to the *second* parameter here!
               HighlightBegin     = argv[optind - 1];
               HighlightEnd       = argv[optind];
               HighlightUnmarked1 = argv[optind + 1];
               HighlightUnmarked2 = argv[optind + 2];
               HighlightMarked1   = argv[optind + 3];
               HighlightMarked2   = argv[optind + 4];
               optind += 5;
            }
            else {
               fputs(gettext("ERROR: Too few arguments for highlight-params!"), stderr);
               fputs("\n", stderr);
               return 1;
            }
          break;
         case 'v':
            version();
          break;
         case 'h':
            usage(argv[0], 0);
          break;
         default:
            fprintf(stderr, gettext("ERROR: Invalid argument %s!"), argv[optind - 1]);
            fputs("\n", stderr);
            return 1;
          break;
      }
   }
   if(optind < argc) {
      usage(argv[0], 1);
   }

   // ====== Check parameters ===============================================
   if( (BeginTag != nullptr) && (BeginTag[0] == 0x00) ) {
      BeginTag = nullptr;
   }
   if( (EndTag == nullptr) || (EndTag[0] == 0x00) || (strcmp(EndTag, BeginTag) == 0) ) {
      EndTag = BeginTag;
   }
   if( (BeginTag) && ((SelectBegin != 0) || (SelectEnd != 0)) ) {
      fputs(gettext("ERROR: Select (--select|-s) and tags (--begin-tag/-b/--end-tag/-e/--tag/-t) are mutually exclusive!"), stderr);
      fputs("\n", stderr);
      return 1;
   }
   BeginTagLength = (BeginTag != nullptr) ? strlen(BeginTag) : 0;
   EndTagLength   = (EndTag != nullptr)   ? strlen(EndTag)   : 0;

   switch(Mode) {
      case Cat:
      case Discard:
      case Enumerate:
         if( showWarnings && ( (BeginTag != nullptr) || (EndTag != nullptr) ) ) {
            fputs(gettext("WARNING: Begin/end tags (--begin-tag/-b/--end-tag/-e/--tag/-t) have no effect in Cat, Discard, or Enumerate Mode!"), stderr);
            fputs("\n", stderr);
         }
         BeginTag = nullptr;
         EndTag   = nullptr;
       break;
      case Extract:
      case Remove:
      case Replace:
         if( showWarnings && (BeginTag != nullptr) && (BeginTag == EndTag) && (!IncludeTags) )  {
            fputs(gettext("WARNING: Identical begin/end tags (--tag/-t) with excluded tags (--exclude-tags/-x) are not useful in Extract, Remove, or Replace Mode!"), stderr);
            fputs("\n", stderr);
         }
       break;
      default:
       break;
   }

   // ====== Allocate buffer ================================================
   Buffer = (char*)malloc(BufferSize);
   if(Buffer == nullptr) {
      cleanUp(1);
   }

   // ====== Open input file ================================================
   InputFile = stdin;
   if(InputFileName != nullptr) {
      InputFile = fopen(InputFileName, "r");
      if(InputFile == nullptr) {
         fprintf(stderr, gettext("ERROR: Unable to open input file %s: %s"),
                 InputFileName, strerror(errno));
         fputs("\n", stderr);
         cleanUp(1);
      }
      OpenInputFile = true;
#ifdef POSIX_FADV_SEQUENTIAL
      const int advice = ((SelectBegin >= 0) && (SelectEnd >= 0)) ?
         POSIX_FADV_SEQUENTIAL|POSIX_FADV_WILLNEED|POSIX_FADV_NOREUSE :
         POSIX_FADV_SEQUENTIAL|POSIX_FADV_WILLNEED;
      posix_fadvise(fileno(InputFile), 0, 0, advice);
#endif
      TotalInputLines = countLines(InputFile);
      if(SelectBegin < 0) {
         SelectBegin = TotalInputLines + SelectBegin + 1;
      }
      if(SelectEnd < 0) {
         SelectEnd = TotalInputLines + SelectEnd + 1;
      }
      if(showWarnings && ( (SelectEnd < SelectBegin) && (SelectEnd != 0)) ) {
         fprintf(stderr,
                 gettext("WARNING: Select range (%lld — %lld) is not useful. Input file too short?"),
                 SelectBegin, SelectEnd);
         fputs("\n", stderr);
      }
   }
   else {
      if((SelectBegin < 0) || (SelectEnd < 0)) {
         fputs(gettext("ERROR: Select from end of file (negative line number) is only possible with input file!"), stderr);
         fputs("\n", stderr);
         return 1;
      }
   }

   // ====== Open output file ===============================================
   OutputFile = stdout;
   if(OutputFileName != nullptr) {
      OutputFile = fopen(OutputFileName, (OpenOutputAppend == false) ? "w" : "a");
      if(OutputFile == nullptr) {
         fprintf(stderr, gettext("ERROR: Unable to open output file %s: %s"),
                 OutputFileName, strerror(errno));
         fputs("\n", stderr);
         cleanUp(1);
      }
      OpenOutputFile = true;
#ifdef POSIX_FADV_SEQUENTIAL
      posix_fadvise(fileno(OutputFile), 0, 0, POSIX_FADV_SEQUENTIAL|POSIX_FADV_NOREUSE);
#endif
   }

   // ====== Open insert file ===============================================
   if(InsertFileName != nullptr) {
      InsertFile = fopen(InsertFileName, "r");
      if(InsertFile == nullptr) {
         fprintf(stderr, gettext("ERROR: Unable to open insert file %s: %s"),
                 InsertFileName, strerror(errno));
         fputs("\n", stderr);
         cleanUp(1);
      }
#ifdef POSIX_FADV_SEQUENTIAL
      posix_fadvise(fileno(InputFile), 0, 0, POSIX_FADV_SEQUENTIAL|POSIX_FADV_WILLNEED);
#endif
   }
   else {
      if( (Mode == InsertFront) || (Mode == InsertBack) || (Mode == Replace) ) {
         fputs(gettext("ERROR: No insert file provided!"), stderr);
         fputs("\n", stderr);
         cleanUp(1);
      }
   }

   // ====== Read lines from input file =====================================
#ifdef DEBUG_MODE
   printf("Select Begin=%lld\n", SelectBegin);
   printf("Select End=%lld\n", SelectEnd);
   printf("Begin Tag=%s\n",   BeginTag);
   printf("End Tag=%s\n",     EndTag);
   printf("IncludeTags=%u\n", IncludeTags);
   printf("FullTagLines=%u\n", FullTagLines);
#endif

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


      // ====== Tag handling ================================================
      if(MarkerTag != nullptr) {
         Pointer = Line;
         const char* next;
         bool        foundMarker = false;
         while( (next = (MarkerTag != nullptr) ? strstr(Pointer, MarkerTag) : nullptr) != nullptr ) {
            foundMarker = true;

            // ====== Begin marker found ====================================
            if(MarkerTag == BeginTag) {
               // ------ Begin of marking with full tag lines ---------------
               if(FullTagLines) {
                  next += MarkerTagLength;
                  if(IncludeTags) {
                     processUnmarked(Line, 0, true);
                     processMarked(Pointer, (ssize_t)(next - Pointer), false);
                  }
                  else {
                     processUnmarked(Pointer, (ssize_t)(next - Pointer), false);
                     // Postponed start of block until end of the line!
                  }
               }
               // ------ Begin of marking with tags only --------------------
               else {
                  if(!IncludeTags) {
                     next += MarkerTagLength;
                  }
                  processUnmarked(Pointer, (ssize_t)(next - Pointer), true);
                  // ------ Special case: BeginTag == EndTag ----------------
                  if(BeginTag == EndTag) {
                     if(IncludeTags) {
                        processMarked(next, MarkerTagLength, true);
                        next += MarkerTagLength;
                     }
                     else {
                        processMarked(next, 0, true);
                     }
                  }
               }
               MarkerTag       = EndTag;
               MarkerTagLength = EndTagLength;
            }

            // ====== End marker found ======================================
            else /* if(MarkerTag == EndTag) */ {
               if(FullTagLines) {
                  next += MarkerTagLength;
                  if(IncludeTags) {
                     processMarked(Pointer, (ssize_t)(next - Pointer), false);
                     // Postponed end of block until end of the line!
                  }
                  else {
                     processMarked(Line, 0, true);
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

            // ====== Special case: FullTagLines with multiple markers ======
            if((FullTagLines) && (foundMarker)) {
               // FullTagLines with multiple markers in a line is not useful
               // => Interpret only the *first* marker here!
               break;
            }
         }

         // ====== No (further) marker found ================================
         // Still need to handle the rest of the line ...
         if( (FullTagLines) && (foundMarker) ) {
            if(IncludeTags) {
               processMarked(Pointer, (ssize_t)(EndOfLine - Pointer), (MarkerTag == BeginTag));
            }
            else {
               if(BeginTag != EndTag) {
                  processUnmarked(Pointer, (ssize_t)(EndOfLine - Pointer), (MarkerTag == EndTag));
               }
               // ------ Special case: BeginTag == EndTag -------------------
               else {
                  processUnmarked(Pointer, (ssize_t)(EndOfLine - Pointer), true);
                  processMarked(EndOfLine, 0, true);
               }
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

      // ====== Select handling =============================================
      else if( (SelectBegin > 0) && (LineNo >= SelectBegin) &&
               ( (SelectEnd == 0) || (LineNo <= SelectEnd) ) ) {
         if(LineNo == SelectBegin) {
            processUnmarked(Line, 0, true);
         }
         processMarked(Line, (ssize_t)(EndOfLine - Line), false);
         if(LineNo == SelectEnd) {
            processMarked(EndOfLine, 0, true);
         }
      }

      // ====== Just process a regular, unmarked line =======================
      else {
         processUnmarked(Line, (ssize_t)(EndOfLine - line), false);
      }


      // ====== Prepare next iteration ======================================
      line = Buffer;
   }
   if( (lineLength < 0) && (errno != 0) ) {
      fprintf(stderr, gettext("ERROR: Read error: %s"), strerror(errno));
      fputs("\n", stderr);
      cleanUp(1);
   }

   // ====== Clean up =======================================================
   if(InMarkedBlock) {
      // Finish the marked block, if it is still active:
      processMarked(Line, 0, true);
   }
   cleanUp(0);
}
