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
// Print-UTF8
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

#include <algorithm>
#include <cctype>
#include <clocale>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <getopt.h>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#if !defined(_WIN32)
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#else
#include <windows.h>
#include <io.h>
#include <shellapi.h>
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


enum class PrintMode {
   Indent,
   Center,
   MultiLineIndent,
   MultiLineCenter,
   Separator,
   Width,
   TerminalInfo,
   Length,
   Size,
   SizeLengthWidth
};

struct ConsoleDimensions {
   unsigned short columns { 80 };
   unsigned short rows    { 24 };
};


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


// ###### Obtain console dimensions #########################################
static bool getConsoleDimensions(ConsoleDimensions& dimensions)
{
#if !defined(_WIN32)
   struct winsize w { };
   if((ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) && (w.ws_col > 0)) {
      dimensions.columns = w.ws_col;
      dimensions.rows    = w.ws_row;
      return true;
   }

   const int fileDescriptor = open("/dev/tty", O_RDWR);
   if(fileDescriptor >= 0) {
      const int result = ioctl(fileDescriptor, TIOCGWINSZ, &w);
      close(fileDescriptor);
      if((result == 0) && (w.ws_col > 0)) {
         dimensions.columns = w.ws_col;
         dimensions.rows    = w.ws_row;
         return true;
      }
   }
#else
   HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
   CONSOLE_SCREEN_BUFFER_INFO screenBufferInfo { };
   if(GetConsoleScreenBufferInfo(hConsole, &screenBufferInfo)) {
      dimensions.columns = static_cast<unsigned short>(
         screenBufferInfo.srWindow.Right - screenBufferInfo.srWindow.Left + 1);
      dimensions.rows    = static_cast<unsigned short>(
         screenBufferInfo.srWindow.Bottom - screenBufferInfo.srWindow.Top + 1);
      return true;
   }

   hConsole = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          nullptr, OPEN_EXISTING, 0, nullptr);
   if(hConsole != INVALID_HANDLE_VALUE) {
      const bool ok = GetConsoleScreenBufferInfo(hConsole, &screenBufferInfo);
      if(ok) {
         dimensions.columns = static_cast<unsigned short>(
            screenBufferInfo.srWindow.Right - screenBufferInfo.srWindow.Left + 1);
         dimensions.rows    = static_cast<unsigned short>(
            screenBufferInfo.srWindow.Bottom - screenBufferInfo.srWindow.Top + 1);
      }
      CloseHandle(hConsole);
      if(ok) {
         return true;
      }
   }
#endif

   const char* columnsString = std::getenv("COLUMNS");
   const char* rowsString    = std::getenv("LINES");
   if((columnsString != nullptr) && (rowsString != nullptr)) {
      const int columns = std::atoi(columnsString);
      const int rows    = std::atoi(rowsString);
      if((columns > 0) && (rows > 0)) {
         dimensions.columns = static_cast<unsigned short>(columns);
         dimensions.rows    = static_cast<unsigned short>(rows);
         return true;
      }
   }

   return false;
}


// ###### Obtain console width ##############################################
static int getConsoleWidth()
{
   ConsoleDimensions dimensions;
   if(getConsoleDimensions(dimensions)) {
      return dimensions.columns;
   }
   return 80;
}


// ###### Hexadecimal digit to number #######################################
static unsigned int hexDigitToNumber(const char digit)
{
   if((digit >= '0') && (digit <= '9')) {
      return static_cast<unsigned int>(digit - '0');
   }
   if((digit >= 'a') && (digit <= 'f')) {
      return static_cast<unsigned int>(10 + (digit - 'a'));
   }
   if((digit >= 'A') && (digit <= 'F')) {
      return static_cast<unsigned int>(10 + (digit - 'A'));
   }
   // TRANSLATORS: Error message when an invalid hexadecimal character is
   // encountered in escape sequence:
   // {0} = invalid character
   std::cerr << formatText("ERROR: Invalid hexadecimal digit {0}!", digit) << '\n';
   std::exit(1);
}


// ###### Octal digit to number #############################################
static unsigned int octDigitToNumber(const char digit)
{
   if((digit >= '0') && (digit <= '7')) {
      return static_cast<unsigned int>(digit - '0');
   }
   // TRANSLATORS: Error message when an invalid octal character is
   // encountered in escape sequence:
   // {0} = invalid character
   std::cerr << formatText("ERROR: Invalid octal digit {0}!", digit) << '\n';
   std::exit(1);
}


// ###### Encode UTF-32 codepoint directly to UTF-8 bytes ###################
static void encodeCodepointToUtf8(const uint32_t codepoint,
                                  std::string&   output)
{
   if(codepoint <= 0x7fu) {
      output.push_back(static_cast<char>(codepoint));
   }
   else if(codepoint <= 0x7ffu) {
      output.push_back(static_cast<char>(0xc0u | (codepoint >> 6u)));
      output.push_back(static_cast<char>(0x80u | (codepoint & 0x3fu)));
   }
   else if(codepoint <= 0xffffu) {
      output.push_back(static_cast<char>(0xe0u | (codepoint >> 12u)));
      output.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3fu)));
      output.push_back(static_cast<char>(0x80u | (codepoint & 0x3fu)));
   }
   else if(codepoint <= 0x10ffffu) {
      output.push_back(static_cast<char>(0xf0u | (codepoint >> 18u)));
      output.push_back(static_cast<char>(0x80u | ((codepoint >> 12u) & 0x3fu)));
      output.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3fu)));
      output.push_back(static_cast<char>(0x80u | (codepoint & 0x3fu)));
   }
}


// ###### Unescape string ###################################################
static std::string unescape(std::string_view originalString)
{
   std::string unescaped;
   unescaped.reserve(originalString.size());

   size_t index = 0;
   while(index < originalString.size()) {
      if(originalString[index] != '\\') {
         unescaped.push_back(originalString[index]);
      }
      else {
         if(index + 1 < originalString.size()) {
            index++;
            switch(originalString[index]) {
               case 'n':
                  unescaped.push_back('\n');
                  break;
               case 'r':
                  unescaped.push_back('\r');
                  break;
               case '\\':
                  unescaped.push_back('\\');
                  break;
               case 'x':
                  if(index + 2 < originalString.size()) {
                     const unsigned int value =
                        (hexDigitToNumber(originalString[index + 1]) << 4u) +
                        hexDigitToNumber(originalString[index + 2]);
                     unescaped.push_back(static_cast<char>(value));
                     index += 2;
                  }
                  break;
               case '0':
                  if(index + 3 < originalString.size()) {
                     const unsigned int value =
                        (octDigitToNumber(originalString[index + 1]) << 6u) +
                        (octDigitToNumber(originalString[index + 2]) << 3u) +
                        octDigitToNumber(originalString[index + 3]);
                     unescaped.push_back(static_cast<char>(value));
                     index += 3;
                  }
                  break;
               case 'u':
                  if(index + 4 < originalString.size()) {
                     const uint32_t codepoint =
                        (hexDigitToNumber(originalString[index + 1]) << 12u) +
                        (hexDigitToNumber(originalString[index + 2]) << 8u) +
                        (hexDigitToNumber(originalString[index + 3]) << 4u) +
                        hexDigitToNumber(originalString[index + 4]);
                     encodeCodepointToUtf8(codepoint, unescaped);
                     index += 4;
                  }
                  break;
               case 'U':
                  if(index + 8 < originalString.size()) {
                     const uint32_t codepoint =
                        (hexDigitToNumber(originalString[index + 1]) << 28u) +
                        (hexDigitToNumber(originalString[index + 2]) << 24u) +
                        (hexDigitToNumber(originalString[index + 3]) << 20u) +
                        (hexDigitToNumber(originalString[index + 4]) << 16u) +
                        (hexDigitToNumber(originalString[index + 5]) << 12u) +
                        (hexDigitToNumber(originalString[index + 6]) << 8u) +
                        (hexDigitToNumber(originalString[index + 7]) << 4u) +
                        hexDigitToNumber(originalString[index + 8]);
                     encodeCodepointToUtf8(codepoint, unescaped);
                     index += 8;
                  }
                  break;
               case 'a':
                  unescaped.push_back('\a');
                  break;
               case 'b':
                  unescaped.push_back('\b');
                  break;
               case 'e':
               case 'E':
                  unescaped.push_back('\x1b');
                  break;
               case 'f':
                  unescaped.push_back('\f');
                  break;
               case 't':
                  unescaped.push_back('\t');
                  break;
               case 'v':
                  unescaped.push_back('\v');
                  break;
               default:
                  unescaped.push_back('\\');
                  unescaped.push_back(originalString[index]);
                  break;
            }
         }
         else {
            unescaped.push_back(originalString[index]);
         }
      }
      index++;
   }

   return unescaped;
}


// ###### Decode UTF-8 string directly to UTF-32 without ANSI ###############
static std::vector<char32_t> convertUtf8ToUtf32WithoutANSI(std::string_view originalString,
                                                           const bool       removeANSISequences,
                                                           size_t*          utf8ByteSize = nullptr)
{
   std::string filtered;
   filtered.reserve(originalString.size());

   bool inANSI = false;
   for(size_t i = 0; i < originalString.size(); ++i) {
      if(!inANSI) {
         if((originalString[i] == '\x1b') && removeANSISequences) {
            inANSI = true;
         }
         else {
            filtered.push_back(originalString[i]);
         }
      }
      else if(std::isalpha(static_cast<unsigned char>(originalString[i]))) {
         inANSI = false;
      }
   }

   if(utf8ByteSize != nullptr) {
      *utf8ByteSize = filtered.size();
   }

   std::vector<char32_t> utf32;
   utf32.reserve(filtered.size());

   size_t i = 0;
   while(i < filtered.size()) {
      const auto firstByte = static_cast<unsigned char>(filtered[i]);
      char32_t   codepoint = 0;
      size_t     bytes     = 1;

      if(firstByte < 0x80u) {
         codepoint = firstByte;
         bytes     = 1;
      }
      else if(((firstByte & 0xe0u) == 0xc0u) && (i + 1 < filtered.size())) {
         codepoint = ((firstByte & 0x1fu) << 6u) |
                     (static_cast<unsigned char>(filtered[i + 1]) & 0x3fu);
         bytes     = 2;
      }
      else if(((firstByte & 0xf0u) == 0xe0u) && (i + 2 < filtered.size())) {
         codepoint = ((firstByte & 0x0fu) << 12u) |
                     ((static_cast<unsigned char>(filtered[i + 1]) & 0x3fu) << 6u) |
                     (static_cast<unsigned char>(filtered[i + 2]) & 0x3fu);
         bytes     = 3;
      }
      else if(((firstByte & 0xf8u) == 0xf0u) && (i + 3 < filtered.size())) {
         codepoint = ((firstByte & 0x07u) << 18u) |
                     ((static_cast<unsigned char>(filtered[i + 1]) & 0x3fu) << 12u) |
                     ((static_cast<unsigned char>(filtered[i + 2]) & 0x3fu) << 6u) |
                     (static_cast<unsigned char>(filtered[i + 3]) & 0x3fu);
         bytes     = 4;
      }
      else {
         codepoint = 0xfffdu;
         bytes     = 1;
      }

      utf32.push_back(codepoint);
      i += bytes;
   }

   return utf32;
}


// ###### Calculate column width for UTF-32 codepoints ######################
static int utf32DisplayWidth(const std::vector<char32_t>& codepoints)
{
   int width = 0;
   for(size_t i = 0; i < codepoints.size(); ++i) {
      const char32_t code = codepoints[i];

      // Zero-width characters (combining marks, variation selectors, ZWJ):
      if(((code >= 0x0300u) && (code <= 0x036fu)) ||
         ((code >= 0x1ab0u) && (code <= 0x1affu)) ||
         ((code >= 0x1dc0u) && (code <= 0x1dffu)) ||
         ((code >= 0x20d0u) && (code <= 0x20ffu)) ||
         ((code >= 0xfe00u) && (code <= 0xfe0fu)) ||
         ((code >= 0xfe20u) && (code <= 0xfe2fu)) ||
         ((code >= 0xe0100u) && (code <= 0xe01efu)) ||
         (code == 0x200bu) || (code == 0x200cu) || (code == 0x200du) ||
         (code == 0xfeffu) || (code == 0x00adu)) {
         continue;
      }

      // Regional Indicator pairs (e.g. flag emojis) -> total width 2:
      if((code >= 0x1f1e6u) && (code <= 0x1f1ffu)) {
         if((i + 1 < codepoints.size()) &&
            (codepoints[i + 1] >= 0x1f1e6u) && (codepoints[i + 1] <= 0x1f1ffu)) {
            width += 2;
            i++;
            continue;
         }
      }

      // East Asian Wide & Emoji Presentation characters -> width 2:
      if(((code >= 0x1100u) && (code <= 0x115fu)) ||
         ((code >= 0x2e80u) && (code <= 0xa4cfu && code != 0x3030u)) ||
         ((code >= 0xac00u) && (code <= 0xd7a3u)) ||
         ((code >= 0xf900u) && (code <= 0xfaffu)) ||
         ((code >= 0xfe10u) && (code <= 0xfe19u)) ||
         ((code >= 0xff01u) && (code <= 0xff60u)) ||
         ((code >= 0xffe0u) && (code <= 0xffe6u)) ||
         ((code >= 0x1f300u) && (code <= 0x1f6ffu)) ||
         ((code >= 0x1f900u) && (code <= 0x1f9ffu)) ||
         ((code >= 0x1fa70u) && (code <= 0x1faffu)) ||
         ((code >= 0x20000u) && (code <= 0x3fffdu))) {
         width += 2;
      }
      else {
         width += 1;
      }
   }
   return width;
}


// ###### Obtain console printing width of UTF-8 string ######################
static int stringWidth(std::string_view originalString,
                       const bool       removeANSISequences)
{
   const std::vector<char32_t> utf32 =
      convertUtf8ToUtf32WithoutANSI(originalString, removeANSISequences);
   const int width = utf32DisplayWidth(utf32);
   return (width >= 0) ? width : 0;
}


// ###### Print Size / Length / Width #######################################
static void printStringSizeLengthWidth(std::string_view originalString,
                                       const bool       removeANSISequences,
                                       const bool       showSize,
                                       const bool       showLength,
                                       const bool       showWidth)
{
   size_t utf8ByteSize = 0;
   const std::vector<char32_t> utf32 =
      convertUtf8ToUtf32WithoutANSI(originalString, removeANSISequences, &utf8ByteSize);

   std::string output;
   if(showSize) {
      output += std::to_string(utf8ByteSize);
   }
   if(showLength) {
      if(!output.empty()) {
         output += ' ';
      }
      output += std::to_string(utf32.size());
   }
   if(showWidth) {
      if(!output.empty()) {
         output += ' ';
      }
      output += std::to_string(utf32DisplayWidth(utf32));
   }
   std::cout << output << '\n';
}


// ###### Get terminal information ##########################################
static void printTerminalInfo()
{
   ConsoleDimensions dimensions;
   if(!getConsoleDimensions(dimensions)) {
      dimensions.columns = 80;
      dimensions.rows    = 24;
   }
   std::cout << dimensions.columns << ' ' << dimensions.rows << '\n';
}


// ###### Indent with given number of spaces ################################
static void printIndentSpaces(const int indentWidth)
{
   if(indentWidth > 0) {
      std::cout << std::string(static_cast<size_t>(indentWidth), ' ');
   }
}


// ###### Indent ############################################################
static void printIndented(std::string_view utf8String,
                          const int        totalIndent)
{
   if(totalIndent > 0) {
      printIndentSpaces(totalIndent - stringWidth(utf8String, true));
   }
   std::cout << utf8String;
   if(totalIndent < 0) {
      printIndentSpaces((-totalIndent) - stringWidth(utf8String, true));
   }
}


// ###### Center ############################################################
static void printCentered(std::string_view utf8String,
                          const int        consoleWidth,
                          const bool       padding)
{
   const int additionalSpacing = consoleWidth - stringWidth(utf8String, true);
   const int indentation       = additionalSpacing / 2;

   printIndentSpaces(indentation);
   std::cout << utf8String;
   if(padding) {
      printIndentSpaces(additionalSpacing - indentation);
   }
}


// ###### Separator #########################################################
static void printSeparator(std::string_view borderLeft,
                           std::string_view separatorString,
                           std::string_view borderRight,
                           const int        consoleWidth)
{
   const int borderLeftWidth  = stringWidth(borderLeft, true);
   const int borderRightWidth = stringWidth(borderRight, true);
   int       separatorWidth   = stringWidth(separatorString, true);

   if(separatorWidth < 1) {
      separatorString = " ";
      separatorWidth  = 1;
   }

   std::cout << borderLeft;
   int currentWidth = borderLeftWidth + borderRightWidth;
   while(currentWidth + separatorWidth <= consoleWidth) {
      std::cout << separatorString;
      currentWidth += separatorWidth;
   }
   std::cout << borderRight;
}


// ###### Multi-Line Indent or Center #######################################
static void doMultiLineIndentOrCenter(std::string_view borderLeft,
                                      std::string_view borderRight,
                                      int              consoleWidth,
                                      const int        indentWidth,
                                      const PrintMode  mode)
{
   std::vector<std::string> lines;
   std::string              currentLine;

   while(std::getline(std::cin, currentLine)) {
      if(!currentLine.empty() && (currentLine.back() == '\r')) {
         currentLine.pop_back();
      }
      lines.push_back(currentLine);
      if(lines.size() >= 1024) {
         break;
      }
   }

   if(!borderLeft.empty()) {
      const int leftWidth = stringWidth(borderLeft, true);
      if(leftWidth > 0) {
         consoleWidth -= leftWidth;
      }
   }
   if(!borderRight.empty()) {
      const int rightWidth = stringWidth(borderRight, true);
      if(rightWidth > 0) {
         consoleWidth -= rightWidth;
      }
   }

   for(size_t i = 0; i < lines.size(); ++i) {
      std::cout << borderLeft;
      if(mode == PrintMode::MultiLineIndent) {
         printIndented(lines[i], indentWidth);
      }
      else {
         printCentered(lines[i], consoleWidth, true);
      }
      std::cout << borderRight;
      if(i + 1 < lines.size()) {
         std::cout << '\n';
      }
   }
}


// ###### Version ###########################################################
[[noreturn]] static void version()
{
   std::cout << "print-utf8 " << SYSTEMTOOLS_VERSION << '\n';
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
             << CMD(program) << "\n"
                   ENTRY(OPT("-n") PIPE OPT("--newline"))
                   ENTRY(OPT("-i") " " ARG("indentation") PIPE OPT("--indent") " " ARG("indentation"))
                   ENTRY(ARG("string"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-n") PIPE OPT("--newline"))
                   ENTRY(OPT("-x") " " ARG("console_width") PIPE OPT("--columns") " " ARG("console_width"))
                   ENTRY(OPT("-c") PIPE OPT("--center"))
                   ENTRY(ARG("string"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-n") PIPE OPT("--newline"))
                   ENTRY(OPT("-x") " " ARG("console_width") PIPE OPT("--columns") " " ARG("console_width"))
                   ENTRY(OPT("-I") " " ARG("indentation") " " ARG("left") " " ARG("right") PIPE OPT("--multiline-indent") " " ARG("indentation") " " ARG("left") " " ARG("right"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-n") PIPE OPT("--newline"))
                   ENTRY(OPT("-x") " " ARG("console_width") PIPE OPT("--columns") " " ARG("console_width"))
                   ENTRY(OPT("-C") " " ARG("left") " " ARG("right") PIPE OPT("--multiline-center") " " ARG("left") " " ARG("right"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-n") PIPE OPT("--newline"))
                   ENTRY(OPT("-x") " " ARG("console_width") PIPE OPT("--columns") " " ARG("console_width"))
                   ENTRY(OPT("-s") " " ARG("left") " " ARG("separator") " " ARG("right") PIPE OPT("--separator") " " ARG("left") " " ARG("separator") " " ARG("right"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-b") PIPE OPT("--size"))
                   ENTRY(ARG("string"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-l") PIPE OPT("--length"))
                   ENTRY(ARG("string"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-w") PIPE OPT("--width"))
                   ENTRY(ARG("string"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-a") PIPE OPT("--size-length-width"))
                   ENTRY(ARG("string"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-t") PIPE OPT("--terminal-info"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-h") PIPE OPT("--help"))
             << CMD(program) << "\n"
                   ENTRY(OPT("-v") PIPE OPT("--version"));
   exit(exitCode);
}
// ------ END-OF-USAGE-BLOCK ------


// ###### Main program #######################################################
int main(int argc, char** argv)
{
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

   // ====== Initialise i18n support ========================================
#if defined(ENABLE_NLS)
#if !defined(_WIN32)
   const char* textdomaindir     = std::getenv("TEXTDOMAINDIR");
#else
   char*       textdomaindir     = nullptr;
   size_t      textdomaindirsize = 0;
   _dupenv_s(&textdomaindir, &textdomaindirsize, "TEXTDOMAINDIR");
#endif
   bindtextdomain("print-utf8",
                  ( ((textdomaindir) && (textdomaindir[0] != 0x00)) ?
                       textdomaindir : SYSTEMTOOLS_LOCALEDIR ));
#if defined(_WIN32)
   if(textdomaindir) {
       free(textdomaindir);
   }
#endif
   bind_textdomain_codeset("print-utf8", "UTF-8");
   textdomain("print-utf8");
#endif

   PrintMode   mode                = PrintMode::Indent;
   int         indentWidth         = 0;
   bool        newline             = false;
   const int   defaultConsoleWidth = getConsoleWidth();
   int         consoleWidth        = defaultConsoleWidth;

   std::string utf8String;
   std::string borderLeft;
   std::string borderRight;

   static const struct option longOptions[] = {
      { "indent",            required_argument, nullptr, 'i' },
      { "multiline-indent",  required_argument, nullptr, 'I' },
      { "center",            no_argument,       nullptr, 'c' },
      { "multiline-center",  required_argument, nullptr, 'C' },
      { "separator",         required_argument, nullptr, 's' },
      { "newline",           no_argument,       nullptr, 'n' },
      { "terminal-info",     no_argument,       nullptr, 't' },
      { "width",             no_argument,       nullptr, 'w' },
      { "length",            no_argument,       nullptr, 'l' },
      { "size",              no_argument,       nullptr, 'b' },
      { "size-length-width", no_argument,       nullptr, 'a' },
      { "columns",           required_argument, nullptr, 'x' },
      { "help",              no_argument,       nullptr, 'h' },
      { "version",           no_argument,       nullptr, 'v' },
      { nullptr,             0,                 nullptr, 0   }
   };

   int option    { 0 };
   int longIndex { 0 };
   while((option = getopt_long(argc, argv, "i:cI:C:s:ntwlbax:hv", longOptions, &longIndex)) != -1) {
      switch(option) {
         case 'i':
            mode        = PrintMode::Indent;
            indentWidth = std::atoi(optarg);
            break;
         case 'c':
            mode = PrintMode::Center;
            break;
         case 'I':
            mode = PrintMode::MultiLineIndent;
            if(optind + 1 < argc) {
               indentWidth = std::atoi(optarg);
               borderLeft  = unescape(argv[optind + 0]);
               borderRight = unescape(argv[optind + 1]);
               optind += 2;
            }
            else {
               // TRANSLATORS: Error message when --multiline-indent/-I is given
               // fewer than the required arguments (indentation, left, right).
               std::cerr << gettext("ERROR: Invalid arguments for multiline-indent!") << '\n';
               return 1;
            }
            break;
         case 'C':
            mode = PrintMode::MultiLineCenter;
            if(optind < argc) {
               borderLeft  = unescape(optarg);
               borderRight = unescape(argv[optind]);
               optind++;
            }
            else {
               // TRANSLATORS: Error message when --multiline-center/-C is given
               // fewer than the required arguments (left, right).
               std::cerr << gettext("ERROR: Invalid arguments for multiline-center!") << '\n';
               return 1;
            }
            break;
         case 's':
            mode = PrintMode::Separator;
            if(optind + 1 < argc) {
               borderLeft  = unescape(optarg);
               utf8String  = unescape(argv[optind + 0]);
               borderRight = unescape(argv[optind + 1]);
               optind += 2;
            }
            else {
               // TRANSLATORS: Error message when --separator/-s is given
               // fewer than the required arguments (border_left, separator,
               // border_right).
               std::cerr << gettext("ERROR: Invalid separator setting!") << '\n';
               return 1;
            }
            break;
         case 'x':
            consoleWidth = std::atoi(optarg);
            if(consoleWidth <= 0) {
               consoleWidth = defaultConsoleWidth + consoleWidth;
            }
            if(consoleWidth > 4096) {
               // TRANSLATORS: Error message when console width exceeds
               // maximum permitted width of 4096 columns:
               // {0} = specified console width
               std::cerr << formatText("ERROR: Invalid console width {0}!", consoleWidth) << '\n';
               return 1;
            }
            break;
         case 'n':
            newline = true;
            break;
         case 't':
            mode = PrintMode::TerminalInfo;
            break;
         case 'w':
            mode = PrintMode::Width;
            break;
         case 'l':
            mode = PrintMode::Length;
            break;
         case 'b':
            mode = PrintMode::Size;
            break;
         case 'a':
            mode = PrintMode::SizeLengthWidth;
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
      if(!utf8String.empty() && (mode == PrintMode::Separator)) {
         usage(argv[0], 1);
      }
      utf8String = unescape(argv[optind++]);
      while(optind < argc) {
         // TRANSLATORS: Warning message when extraneous unrecognized command
         // line parameters are encountered:
         // {0} = unhandled parameter argument
         std::cerr << formatText("WARNING: Unhandled parameter {0}!", argv[optind++]) << '\n';
      }
   }

   // ====== Handle command =================================================
   switch(mode) {
      case PrintMode::Indent:
         if(!utf8String.empty()) {
            printIndented(utf8String, indentWidth);
         }
         break;
      case PrintMode::Center:
         if(!utf8String.empty()) {
            printCentered(utf8String, consoleWidth, false);
         }
         break;
      case PrintMode::MultiLineIndent:
      case PrintMode::MultiLineCenter:
         doMultiLineIndentOrCenter(borderLeft, borderRight,
                                   consoleWidth, indentWidth, mode);
         break;
      case PrintMode::Separator:
         if(!utf8String.empty()) {
            printSeparator(borderLeft, utf8String, borderRight, consoleWidth);
         }
         break;
      case PrintMode::Width:
         printStringSizeLengthWidth(utf8String, true, false, false, true);
         break;
      case PrintMode::Length:
         printStringSizeLengthWidth(utf8String, true, false, true, false);
         break;
      case PrintMode::Size:
         printStringSizeLengthWidth(utf8String, true, true, false, false);
         break;
      case PrintMode::SizeLengthWidth:
         printStringSizeLengthWidth(utf8String, true, true, true, true);
         break;
      case PrintMode::TerminalInfo:
         printTerminalInfo();
         break;
   }

   if(newline) {
      std::cout << '\n';
   }

   return 0;
}
