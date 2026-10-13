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
// Text-Block
// Copyright (C) 2025-2026 by Thomas Dreibholz
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

#include <cassert>
#include <cctype>
#include <cerrno>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <getopt.h>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#if !defined(_WIN32)
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <windows.h>
#include <io.h>
#include <shellapi.h>
#include <sys/stat.h>

#define chown(path, uid, gid) (0)
#define chmod _chmod
#endif

#if defined(ENABLE_NLS)
#include <libintl.h>
#else
#define gettext(string)                   (string)
#define ngettext(singular, plural, count) (((count) == 1) ? (singular) : (plural))
#define bindtextdomain(domain, directory) (directory)
#define textdomain(domain)                (domain)
#endif

#include "package-version.h"


enum class TextBlockMode {
   Cat,
   Discard,
   Enumerate,
   Highlight,
   Extract,
   InsertFront,
   InsertBack,
   Replace,
   Remove
};

static TextBlockMode Mode                = TextBlockMode::Cat;
static int           MinActions          = -1;
static int           MaxActions          = -1;
static std::string   BeginTag;
static std::string   EndTag;
static long long     SelectBegin         = 0;
static long long     SelectEnd           = 0;
static long long     TotalInputLines     = -1;
static bool          IncludeTags         = false;
static bool          FullTagLines        = false;
static std::string   EnumerateFormat     = "%06llu";
static std::string   Enumerate1          = "\x1b[36m";
static std::string   Enumerate2          = "\x1b[0m ";
static std::string   HighlightBegin      = "⭐";
static std::string   HighlightEnd        = "🛑";
static std::string   HighlightUnmarked1  = "\x1b[34m";
static std::string   HighlightUnmarked2  = "\x1b[0m";
static std::string   HighlightMarked1    = "\x1b[31m";
static std::string   HighlightMarked2    = "\x1b[0m";
static std::string   InputFileName;
static FILE*         InputFile           = nullptr;
static bool          OpenInputFile       = false;
static std::string   OutputFileName;
static std::string   OutputTempFileName;
static FILE*         OutputFile          = nullptr;
static bool          OpenOutputFile      = false;
static bool          OpenOutputAppend    = false;
static bool          OutputDeleteOnError = true;
static bool          InPlaceUpdate       = false;
static std::string   InsertFileName;
static FILE*         InsertFile          = nullptr;
static bool          OpenInsertFile      = false;
static bool          InMarkedBlock       = false;

static int           Actions             = 0;
static long long     LineNo              = 0;


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


// ###### Clean up ##########################################################
[[noreturn]] static void cleanUp(int exitCode)
{
   // ====== Close insert file ==============================================
   if(OpenInsertFile) {
      if(InsertFile != nullptr) {
         std::fclose(InsertFile);
      }
      OpenInsertFile = false;
   }
   InsertFile = nullptr;

   // ====== Close input file ===============================================
   if(OpenInputFile) {
      if(InputFile != nullptr) {
         std::fclose(InputFile);
      }
      OpenInputFile = false;
   }
   InputFile = nullptr;

   // ====== Close output file ==============================================
   if(OpenOutputFile) {
      if(OutputFile != nullptr) {
         if(std::fclose(OutputFile) != 0) {
            // TRANSLATORS: Error message when closing output file fails:
            // {0} = output file path
            // {1} = system error message (std::strerror)
            std::cerr << formatText("ERROR: Unable to open output file {0}: {1}!",
                                    OutputFileName, std::strerror(errno)) << '\n';
            exitCode = 1;
         }
      }
      if((exitCode != 0) && OutputDeleteOnError) {
         const std::string& brokenFileName = !OutputTempFileName.empty() ?
                                                OutputTempFileName : OutputFileName;
         if(!brokenFileName.empty()) {
            std::error_code errorCode;
            if(!std::filesystem::remove(brokenFileName, errorCode) && errorCode) {
               // TRANSLATORS: Error message when deleting broken output file
               // fails:
               // {0} = broken file path
               // {1} = system error message (std::strerror)
               std::cerr << formatText("ERROR: Unable to delete broken output file {0}: {1}!",
                                       brokenFileName, errorCode.message()) << '\n';
            }
         }
      }
      else if((exitCode == 0) && !OutputTempFileName.empty()) {
         // ------ Restore original file permissions and ownership ----------
         struct stat fileInfo { };
         if(stat(InputFileName.c_str(), &fileInfo) == 0) {
            const int result = chown(OutputTempFileName.c_str(), fileInfo.st_uid, fileInfo.st_gid);
            (void)result;
            if(chmod(OutputTempFileName.c_str(), fileInfo.st_mode & 07777) != 0) {
               // TRANSLATORS: Warning message when restoring file permissions
               // fails:
               // {0} = input file path
               // {1} = system error message (std::strerror)
               std::cerr << formatText("WARNING: Unable to restore file permissions for {0}: {1}!",
                                       InputFileName, std::strerror(errno)) << '\n';
            }
         }

         // ------ Overwrite original file ----------------------------------
         std::error_code errorCode;
         std::filesystem::rename(OutputTempFileName, InputFileName, errorCode);
         if(errorCode) {
            // TRANSLATORS: Error message when renaming temporary file to
            // target input file name fails:
            // {0} = temporary file path
            // {1} = target input file path
            // {2} = system error message (std::strerror)
            std::cerr << formatText("ERROR: Unable to change name of temporary output file from {0} to {1}: {2}!",
                                    OutputTempFileName, InputFileName, errorCode.message()) << '\n';
         }
      }
      OpenOutputFile = false;
   }
   OutputFile = nullptr;

   std::exit(exitCode);
}


// ###### Write data to output file #########################################
static void writeToOutputFile(const char*  data,
                              const size_t length)
{
   if(length > 0) {
      if(std::fwrite(data, 1, length, OutputFile) < length) {
         // TRANSLATORS: Error message when writing to output file fails:
         // {0} = output file path
         // {1} = system error message (std::strerror)
         std::cerr << formatText("ERROR: Unable to write to output file {0}: {1}!",
                                 OutputFileName, std::strerror(errno)) << '\n';
         cleanUp(1);
      }
   }
}

static inline void writeToOutputFile(std::string_view text)
{
   writeToOutputFile(text.data(), text.size());
}


// ###### Read line preserving trailing newline character ##################
static bool readLine(FILE*        inputFile,
                     std::string& line)
{
   line.clear();
   char buffer[4096];
   while(std::fgets(buffer, sizeof(buffer), inputFile) != nullptr) {
      line.append(buffer);
      if(!line.empty() && (line.back() == '\n')) {
         return true;
      }
   }
   return !line.empty();
}


// ###### Count lines of input file #########################################
static long long countLines(FILE* inputFile)
{
   long long   lines = 0;
   std::string buffer;

   while(readLine(inputFile, buffer)) {
      lines++;
   }
   if(inputFile != stdin) {
      std::rewind(inputFile);
   }

   return lines;
}


// ###### Write contents of insert file #####################################
static void copyInsertFileIntoOutputFile()
{
   char   buffer[65536];
   size_t bytesRead = 0;
   while((bytesRead = std::fread(buffer, 1, sizeof(buffer), InsertFile)) > 0) {
      writeToOutputFile(buffer, bytesRead);
   }
   if(InsertFile != stdin) {
      std::rewind(InsertFile);
   }
}


// ###### Process unmarked text #############################################
static void processUnmarked(std::string_view text,
                            const bool       beginOfMarking)
{
   assert(InMarkedBlock == false);

   switch(Mode) {
      case TextBlockMode::Cat:
      case TextBlockMode::Remove:
      case TextBlockMode::Replace:
         writeToOutputFile(text);
         break;
      case TextBlockMode::InsertFront:
         writeToOutputFile(text);
         if(beginOfMarking) {
            copyInsertFileIntoOutputFile();
         }
         break;
      case TextBlockMode::InsertBack:
         writeToOutputFile(text);
         break;
      case TextBlockMode::Enumerate:
         std::fputs(Enumerate1.c_str(), OutputFile);
         std::fprintf(OutputFile, EnumerateFormat.c_str(), LineNo);
         std::fputs(Enumerate2.c_str(), OutputFile);
         writeToOutputFile(text);
         break;
      case TextBlockMode::Highlight:
         if(!text.empty()) {
            std::fputs(HighlightUnmarked1.c_str(), OutputFile);
            writeToOutputFile(text);
            std::fputs(HighlightUnmarked2.c_str(), OutputFile);
         }
         if(beginOfMarking) {
            std::fputs(HighlightBegin.c_str(), OutputFile);
         }
         break;
      default:
         break;
   }

   if(beginOfMarking) {
      InMarkedBlock = true;
      Actions++;
   }
}


// ###### Process marked text ###############################################
static void processMarked(std::string_view text,
                          const bool       endOfMarking)
{
   assert(InMarkedBlock == true);

   switch(Mode) {
      case TextBlockMode::Extract:
         writeToOutputFile(text);
         break;
      case TextBlockMode::Replace:
         if(endOfMarking) {
            copyInsertFileIntoOutputFile();
         }
         break;
      case TextBlockMode::InsertFront:
         writeToOutputFile(text);
         break;
      case TextBlockMode::InsertBack:
         writeToOutputFile(text);
         if(endOfMarking) {
            copyInsertFileIntoOutputFile();
         }
         break;
      case TextBlockMode::Highlight:
         if(!text.empty()) {
            std::fputs(HighlightMarked1.c_str(), OutputFile);
            writeToOutputFile(text);
            std::fputs(HighlightMarked2.c_str(), OutputFile);
         }
         if(endOfMarking) {
            std::fputs(HighlightEnd.c_str(), OutputFile);
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
[[noreturn]] static void version()
{
   std::cout << "text-block " << SYSTEMTOOLS_VERSION << '\n';
   std::exit(0);
}


// ###### Usage #############################################################
[[noreturn]] static void usage(const char* program,
                               const int   exitCode)
{
   // TRANSLATORS: Command-line syntax message prefix:
   // "Usage: <program> [options]".
   std::cerr << gettext("Usage:") << " " << program
             << " [-C|--cat]"
             << " [-0|--discard]"
             << " [-H|--highlight]"
             << " [-E|--enumerate]"
             << " [-X|--extract]"
             << " [-D|--delete|--remove]"
             << " [-F|--insert-front insert_file]"
             << " [-B|--insert-back insert_file]"
             << " [-R|--replace insert_file]"
             << " [-i|--input input_file]"
             << " [-o|--output output_file]"
             << " [-a|--append]"
             << " [-p|--in-place]"
             << " [-k|--keep-broken]"
             << " [--min-actions|-m actions]"
             << " [--max-actions|-M actions]"
             << " [-s|--select from_line to_line]"
             << " [-b|--begin-tag begin_tag]"
             << " [-e|--end-tag end_tag]"
             << " [-t|--tag tag]"
             << " [-y|--include-tags]"
             << " [-x|--exclude-tags]"
             << " [-f|--full-tag-lines]"
             << " [-g|--tags-only]"
             << " [--highlight-[begin|end|unmarked1|unmarked2|marked1|marked2] label]"
             << " [--highlight-params begin_label end_label unmarked1_label unmarked2_label marked1_label marked2_label]"
             << " [--enumerate-format format]"
             << " [--enumerate-label[1|2] string]"
             << " [-q|--suppress-warnings]"
             << " [-h|--help]"
             << " [-v|--version]\n";
   std::exit(exitCode);
}


// ###### Main program ######################################################
int main(int argc, char** argv)
{
   // ====== Initialise i18n support ========================================
   if(std::setlocale(LC_ALL, "") == nullptr) {
      std::setlocale(LC_ALL, "C.UTF-8");
   }

#if defined(_WIN32)
   SetConsoleCP(CP_UTF8);
   SetConsoleOutputCP(CP_UTF8);

   std::vector<std::string> argumentStrings;
   std::vector<char*>       argPointers;

   int wideArgc = 0;
   wchar_t** wideArgv = CommandLineToArgvW(GetCommandLineW(), &wideArgc);
   if(wideArgv != nullptr) {
      for(int i = 0; i < wideArgc; ++i) {
         const int length = WideCharToMultiByte(CP_UTF8, 0, wideArgv[i], -1,
                                                nullptr, 0, nullptr, nullptr);
         if(length > 0) {
            std::string buffer(static_cast<size_t>(length), '\0');
            WideCharToMultiByte(CP_UTF8, 0, wideArgv[i], -1,
                                buffer.data(), length, nullptr, nullptr);
            buffer.pop_back();
            argumentStrings.push_back(std::move(buffer));
         }
         else {
            argumentStrings.emplace_back();
         }
      }
      LocalFree(wideArgv);
   }

   argPointers.reserve(argumentStrings.size() + 1);
   for(auto& stringArgument : argumentStrings) {
      argPointers.push_back(stringArgument.data());
   }
   argPointers.push_back(nullptr);

   argc = static_cast<int>(argumentStrings.size());
   argv = argPointers.data();
#endif

   bindtextdomain("text-block", SYSTEMTOOLS_LOCALEDIR);
   textdomain("text-block");

   // ====== Handle arguments ===============================================
   bool showWarnings = true;
   static const struct option longOptions[] = {
      { "cat",                 no_argument,       nullptr, 'C' },
      { "discard",             no_argument,       nullptr, '0' },
      { "highlight",           no_argument,       nullptr, 'H' },
      { "enumerate",           no_argument,       nullptr, 'E' },
      { "extract",             no_argument,       nullptr, 'X' },
      { "delete",              no_argument,       nullptr, 'D' },
      { "remove",              no_argument,       nullptr, 'D' },
      { "insert-front",        required_argument, nullptr, 'F' },
      { "insert-back",         required_argument, nullptr, 'B' },
      { "replace",             required_argument, nullptr, 'R' },

      { "input",               required_argument, nullptr, 'i' },
      { "output",              required_argument, nullptr, 'o' },
      { "append",              no_argument,       nullptr, 'a' },
      { "in-place",            no_argument,       nullptr, 'p' },
      { "keep-broken",         no_argument,       nullptr, 'k' },
      { "min-actions",         required_argument, nullptr, 'm' },
      { "max-actions",         required_argument, nullptr, 'M' },

      { "select",              required_argument, nullptr, 's' },
      { "begin-tag",           required_argument, nullptr, 'b' },
      { "end-tag",             required_argument, nullptr, 'e' },
      { "tag",                 required_argument, nullptr, 't' },
      { "exclude-tags",        no_argument,       nullptr, 'x' },
      { "include-tags",        no_argument,       nullptr, 'y' },
      { "full-tag-lines",      no_argument,       nullptr, 'f' },
      { "tags-only",           no_argument,       nullptr, 'g' },

      { "enumerate-format",    required_argument, nullptr, 0x1000 },
      { "enumerate-label1",    required_argument, nullptr, 0x1001 },
      { "enumerate-label2",    required_argument, nullptr, 0x1002 },
      { "highlight-begin",     required_argument, nullptr, 0x2000 },
      { "highlight-end",       required_argument, nullptr, 0x2001 },
      { "highlight-unmarked1", required_argument, nullptr, 0x2002 },
      { "highlight-unmarked2", required_argument, nullptr, 0x2003 },
      { "highlight-marked1",   required_argument, nullptr, 0x2004 },
      { "highlight-marked2",   required_argument, nullptr, 0x2005 },
      { "highlight-params",    required_argument, nullptr, 0x2006 },

      { "suppress-warnings",   no_argument,       nullptr, 'q' },
      { "help",                no_argument,       nullptr, 'h' },
      { "version",             no_argument,       nullptr, 'v' },
      { nullptr,               0,                 nullptr, 0   }
   };

   int option    { 0 };
   int longIndex { 0 };
   while((option = getopt_long(argc, argv, "C0HEXDF:B:R:i:o:apkm:M:b:e:t:s:xyfgqhv", longOptions, &longIndex)) != -1) {
      switch(option) {
         case 'C':
            Mode = TextBlockMode::Cat;
            break;
         case '0':
            Mode = TextBlockMode::Discard;
            break;
         case 'H':
            Mode = TextBlockMode::Highlight;
            break;
         case 'E':
            Mode = TextBlockMode::Enumerate;
            break;
         case 'X':
            Mode = TextBlockMode::Extract;
            break;
         case 'D':
            Mode = TextBlockMode::Remove;
            break;
         case 'F':
            Mode           = TextBlockMode::InsertFront;
            InsertFileName = optarg;
            break;
         case 'B':
            Mode           = TextBlockMode::InsertBack;
            InsertFileName = optarg;
            break;
         case 'R':
            Mode           = TextBlockMode::Replace;
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
         case 'p':
            InPlaceUpdate = true;
            break;
         case 'k':
            OutputDeleteOnError = false;
            break;
         case 'm':
            MinActions = std::atoi(optarg);
            break;
         case 'M':
            MaxActions = std::atoi(optarg);
            break;
         case 's':
            if(optind < argc) {
               SelectBegin = std::atoll(optarg);
               SelectEnd   = std::atoll(argv[optind]);
               optind++;
            }
            else {
               // TRANSLATORS: Error message when line selection (--select|-s)
               // is missing the to_line parameter.
               std::cerr << gettext("ERROR: Too few arguments for line selection (--select|-s)!") << '\n';
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
            EndTag.clear();
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
            for(size_t i = 0; i < std::strlen(optarg); ++i) {
               if((optarg[i] == '%') ||
                  (!std::isalnum(static_cast<unsigned char>(optarg[i])) &&
                   (optarg[i] != '-') && (optarg[i] != '\''))) {
                  // TRANSLATORS: Error message when format string for line
                  // numbering contains invalid characters.
                  std::cerr << gettext("ERROR: Invalid value for enumeration format (--enumerate-format)!") << '\n';
                  return 1;
               }
            }
            EnumerateFormat = std::format("%{}llu", optarg);
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
            if(optind + 4 < argc) {
               HighlightBegin     = optarg;
               HighlightEnd       = argv[optind];
               HighlightUnmarked1 = argv[optind + 1];
               HighlightUnmarked2 = argv[optind + 2];
               HighlightMarked1   = argv[optind + 3];
               HighlightMarked2   = argv[optind + 4];
               optind += 5;
            }
            else {
               // TRANSLATORS: Error message when --highlight-params is given
               // fewer than the required 6 arguments.
               std::cerr << gettext("ERROR: Too few arguments for highlight parameters (--highlight-params)!") << '\n';
               return 1;
            }
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
   if(optind < argc) {
      usage(argv[0], 1);
   }

   // ====== Check parameters ===============================================
   if((MaxActions >= 0) && (MinActions > MaxActions)) {
      // TRANSLATORS: Error message when --min-actions exceeds
      // --max-actions.
      std::cerr << gettext("ERROR: Invalid min/max settings (--min-actions/-m/--max-actions/-M)!") << '\n';
      return 1;
   }
   if(EndTag.empty() || BeginTag.empty() || (EndTag == BeginTag)) {
      EndTag = BeginTag;
   }
   if(!BeginTag.empty() && ((SelectBegin != 0) || (SelectEnd != 0))) {
      // TRANSLATORS: Error message when line selection and tag matching
      // options are specified together.
      std::cerr << gettext("ERROR: Line selection (--select|-s) and tags (--begin-tag/-b/--end-tag/-e/--tag/-t) are mutually exclusive!") << '\n';
      return 1;
   }

   switch(Mode) {
      case TextBlockMode::Cat:
      case TextBlockMode::Discard:
      case TextBlockMode::Enumerate:
         if(showWarnings && (!BeginTag.empty() || !EndTag.empty())) {
            // TRANSLATORS: Warning message when begin/end tags are provided
            // in modes that do not support tags (Cat, Discard, Enumerate).
            std::cerr << gettext("WARNING: Begin/end tags (--begin-tag/-b/--end-tag/-e/--tag/-t) have no effect for Cat, Discard, or Enumerate!") << '\n';
         }
         BeginTag.clear();
         EndTag.clear();
         break;
      case TextBlockMode::Extract:
      case TextBlockMode::Remove:
      case TextBlockMode::Replace:
         if(showWarnings && !BeginTag.empty() && (BeginTag == EndTag) && !IncludeTags) {
            // TRANSLATORS: Warning message when identical begin/end tags
            // are used with excluded tags in Extract, Remove, or Replace
            // mode.
            std::cerr << gettext("WARNING: Identical begin/end tags (--tag/-t) with excluded tags (--exclude-tags/-x) are not useful for Extract, Remove, or Replace!") << '\n';
         }
         break;
      default:
         break;
   }

   // ====== Open input file ================================================
   InputFile = stdin;
   if(InputFileName == "-") {
      InputFileName.clear();
   }
   if(!InputFileName.empty() && InPlaceUpdate) {
      std::error_code errorCode;
      const auto canonicalPath = std::filesystem::canonical(InputFileName, errorCode);
      if(errorCode) {
         // TRANSLATORS: Error message when resolving canonical path of input
         // file fails during in-place update:
         // {0} = input file path
         // {1} = system error message (std::strerror)
         std::cerr << formatText("ERROR: Unable to resolve absolute path for input file {0}: {1}!",
                                 InputFileName, errorCode.message()) << '\n';
         cleanUp(1);
      }
      InputFileName = canonicalPath.string();
   }
   if(!InputFileName.empty()) {
      InputFile = std::fopen(InputFileName.c_str(), "r");
      if(InputFile == nullptr) {
         // TRANSLATORS: Error message when opening input file fails:
         // {0} = input file path
         // {1} = system error message (std::strerror)
         std::cerr << formatText("ERROR: Unable to open input file {0}: {1}!",
                                 InputFileName, std::strerror(errno)) << '\n';
         cleanUp(1);
      }
      OpenInputFile = true;
#ifdef POSIX_FADV_SEQUENTIAL
      const int advice = ((SelectBegin >= 0) && (SelectEnd >= 0)) ?
         POSIX_FADV_SEQUENTIAL | POSIX_FADV_WILLNEED | POSIX_FADV_NOREUSE :
         POSIX_FADV_SEQUENTIAL | POSIX_FADV_WILLNEED;
      posix_fadvise(fileno(InputFile), 0, 0, advice);
#endif
      TotalInputLines = countLines(InputFile);
      if(SelectBegin < 0) {
         SelectBegin = TotalInputLines + SelectBegin + 1;
      }
      if(SelectEnd < 0) {
         SelectEnd = TotalInputLines + SelectEnd + 1;
      }
      if(showWarnings && ((SelectEnd < SelectBegin) && (SelectEnd != 0))) {
         // TRANSLATORS: Warning message when line selection range is empty or
         // inverted (start line exceeds end line):
         // {0} = start line number
         // {1} = end line number
         std::cerr << formatText("WARNING: Line selection range ({0} — {1}) is not useful. Input file too short?",
                                 SelectBegin, SelectEnd) << '\n';
      }
   }
   else {
      if((SelectBegin < 0) || (SelectEnd < 0)) {
         // TRANSLATORS: Error message when negative line offset is used
         // without a file input source.
         std::cerr << gettext("ERROR: Select from end of file (negative line number) is only possible with input file!") << '\n';
         cleanUp(1);
      }
      if(InPlaceUpdate) {
         // TRANSLATORS: Error message when in-place update is requested
         // without specifying an input file.
         std::cerr << gettext("ERROR: In-place update (--in-place|-p) requires an input file (--input/-i)!") << '\n';
         cleanUp(1);
      }
   }

   // ====== Open output file ===============================================
   OutputFile = stdout;
   if(OutputFileName == "-") {
      OutputFileName.clear();
   }
   if(InPlaceUpdate) {
      if(!OutputFileName.empty()) {
         // TRANSLATORS: Error message when in-place update and output file
         // are specified simultaneously.
         std::cerr << gettext("ERROR: In-place update (--in-place|-p) and output file (--output/-o) are mutually exclusive!") << '\n';
         cleanUp(1);
      }
      OutputTempFileName = InputFileName + "~";
   }
   if(!OutputFileName.empty() || !OutputTempFileName.empty()) {
      const std::string& destinationFile = !OutputTempFileName.empty() ?
                                              OutputTempFileName : OutputFileName;
      OutputFile = std::fopen(destinationFile.c_str(), !OpenOutputAppend ? "w" : "a");
      if(OutputFile == nullptr) {
         // TRANSLATORS: Error message when opening or creating output file
         // fails:
         // {0} = output file path
         // {1} = system error message (std::strerror)
         std::cerr << formatText("ERROR: Unable to create output file {0}: {1}!",
                                 destinationFile, std::strerror(errno)) << '\n';
         cleanUp(1);
      }
      OpenOutputFile = true;
#ifdef POSIX_FADV_SEQUENTIAL
      posix_fadvise(fileno(OutputFile), 0, 0, POSIX_FADV_SEQUENTIAL | POSIX_FADV_NOREUSE);
#endif
   }

   // ====== Open insert file ===============================================
   if(InsertFileName == "-") {
      InsertFileName.clear();
      if(InputFile == stdin) {
         // TRANSLATORS: Error message when reading insert content from stdin
         // while main input is also stdin.
         std::cerr << gettext("ERROR: Insert from standard input requires an input file!") << '\n';
         cleanUp(1);
      }
      InsertFile = std::tmpfile();
      if(InsertFile == nullptr) {
         // TRANSLATORS: Error message when creating temporary buffer file for
         // insertion fails:
         // {0} = system error message (std::strerror)
         std::cerr << formatText("ERROR: Unable to create temporary file for insertion: {0}!",
                                 std::strerror(errno)) << '\n';
         cleanUp(1);
      }
      char   buffer[65536];
      size_t bytesRead = 0;
      while((bytesRead = std::fread(buffer, 1, sizeof(buffer), stdin)) > 0) {
         if(std::fwrite(buffer, 1, bytesRead, InsertFile) < bytesRead) {
            // TRANSLATORS: Error message when writing stdin to temporary
            // insert file fails:
            // {0} = system error message (std::strerror)
            std::cerr << formatText("ERROR: Failed to write to temporary file: {0}!",
                                    std::strerror(errno)) << '\n';
            cleanUp(1);
         }
      }
      std::rewind(InsertFile);
      OpenInsertFile = true;
   }
   if(!InsertFileName.empty()) {
      InsertFile = std::fopen(InsertFileName.c_str(), "r");
      if(InsertFile == nullptr) {
         // TRANSLATORS: Error message when opening insert file fails:
         // {0} = insert file path
         // {1} = system error message (std::strerror)
         std::cerr << formatText("ERROR: Unable to open insert file {0}: {1}!",
                                 InsertFileName, std::strerror(errno)) << '\n';
         cleanUp(1);
      }
      OpenInsertFile = true;
   }
   if(InsertFile != nullptr) {
#ifdef POSIX_FADV_SEQUENTIAL
      posix_fadvise(fileno(InsertFile), 0, 0, POSIX_FADV_SEQUENTIAL | POSIX_FADV_WILLNEED);
#endif
   }
   else {
      if((Mode == TextBlockMode::InsertFront) ||
         (Mode == TextBlockMode::InsertBack) ||
         (Mode == TextBlockMode::Replace)) {
         // TRANSLATORS: Error message when an insert or replace mode is
         // selected but no insert file was provided.
         std::cerr << gettext("ERROR: No insert file provided!") << '\n';
         cleanUp(1);
      }
   }

   // ====== Read lines from input file =====================================
   LineNo  = 0;
   Actions = 0;
   std::string activeMarker = BeginTag;

   std::string currentLine;
   while(readLine(InputFile, currentLine)) {
      LineNo++;

      // ====== Tag handling ================================================
      if(!activeMarker.empty()) {
         size_t pointerIndex = 0;
         bool   foundMarker  = false;
         size_t foundIndex   = std::string::npos;

         while(!activeMarker.empty() &&
               (foundIndex = currentLine.find(activeMarker, pointerIndex)) != std::string::npos) {
            foundMarker = true;

            // ====== Begin marker found ====================================
            if(activeMarker == BeginTag) {
               if(FullTagLines) {
                  size_t nextIndex = foundIndex + BeginTag.size();
                  if(IncludeTags) {
                     processUnmarked({}, true);
                     processMarked(std::string_view(currentLine).substr(pointerIndex, nextIndex - pointerIndex), false);
                  }
                  else {
                     processUnmarked(std::string_view(currentLine).substr(pointerIndex, nextIndex - pointerIndex), false);
                  }
                  pointerIndex = nextIndex;
               }
               else {
                  size_t nextIndex = foundIndex;
                  if(!IncludeTags) {
                     nextIndex += BeginTag.size();
                  }
                  processUnmarked(std::string_view(currentLine).substr(pointerIndex, nextIndex - pointerIndex), true);
                  if(BeginTag == EndTag) {
                     if(IncludeTags) {
                        processMarked(std::string_view(currentLine).substr(nextIndex, BeginTag.size()), true);
                        nextIndex += BeginTag.size();
                     }
                     else {
                        processMarked({}, true);
                     }
                  }
                  pointerIndex = nextIndex;
               }
               activeMarker = EndTag;
            }
            // ====== End marker found ======================================
            else {
               if(FullTagLines) {
                  size_t nextIndex = foundIndex + EndTag.size();
                  if(IncludeTags) {
                     processMarked(std::string_view(currentLine).substr(pointerIndex, nextIndex - pointerIndex), false);
                  }
                  else {
                     processMarked({}, true);
                     processUnmarked(std::string_view(currentLine).substr(pointerIndex, nextIndex - pointerIndex), false);
                  }
                  pointerIndex = nextIndex;
               }
               else {
                  size_t nextIndex = foundIndex;
                  if(IncludeTags) {
                     nextIndex += EndTag.size();
                  }
                  processMarked(std::string_view(currentLine).substr(pointerIndex, nextIndex - pointerIndex), true);
                  pointerIndex = nextIndex;
               }
               activeMarker = BeginTag;
            }

            // ====== Special case: FullTagLines with multiple markers ======
            if(FullTagLines && foundMarker) {
               break;
            }
         }

         // ====== No (further) marker found ================================
         if(FullTagLines && foundMarker) {
            if(IncludeTags) {
               processMarked(std::string_view(currentLine).substr(pointerIndex), activeMarker == BeginTag);
            }
            else {
               if(BeginTag != EndTag) {
                  processUnmarked(std::string_view(currentLine).substr(pointerIndex), activeMarker == EndTag);
               }
               else {
                  processUnmarked(std::string_view(currentLine).substr(pointerIndex), true);
                  processMarked({}, true);
               }
            }
         }
         else {
            if(activeMarker == BeginTag) {
               processUnmarked(std::string_view(currentLine).substr(pointerIndex), false);
            }
            else {
               processMarked(std::string_view(currentLine).substr(pointerIndex), false);
            }
         }
      }
      // ====== Select handling =============================================
      else if( (SelectBegin > 0) && (LineNo >= SelectBegin) &&
               ((SelectEnd == 0) || (LineNo <= SelectEnd)) ) {
         if(LineNo == SelectBegin) {
            processUnmarked({}, true);
         }
         processMarked(currentLine, false);
         if(LineNo == SelectEnd) {
            processMarked({}, true);
         }
      }
      // ====== Regular, unmarked line ======================================
      else {
         processUnmarked(currentLine, false);
      }
   }

   if(std::ferror(InputFile) != 0) {
      // TRANSLATORS: Error message when reading from input file fails:
      // {0} = system error message (std::strerror)
      std::cerr << formatText("ERROR: Read error: {0}!", std::strerror(errno)) << '\n';
      cleanUp(1);
   }

   // ====== Clean up =======================================================
   if(InMarkedBlock) {
      processMarked({}, true);
   }

   const bool success =
      ((MinActions < 0) || (Actions >= MinActions)) &&
      ((MaxActions < 0) || (Actions <= MaxActions));
   if(!success) {
      // TRANSLATORS: Error message when the number of performed actions is
      // outside of configured min/max limits.
      std::cerr << gettext("ERROR: Number of actions outside of set limits (--min-actions/-m/--max-actions/-M)!") << '\n';
   }

   cleanUp(success ? 0 : 1);
}
