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
// Print-UTF8
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

#define _XOPEN_SOURCE 500
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <locale.h>
#include <sys/ioctl.h>

#include "package-version.h"


typedef enum printmode {
   Indent          = 1,
   Center          = 2,
   MultiLineIndent = 3,
   MultiLineCenter = 4,
   Separator       = 5,
   Width           = 6,
   TerminalInfo    = 7,
   Length          = 8,
   Size            = 9,
   SizeLengthWidth = 10
} printmode_t;


// ###### Obtain console size ###############################################
static bool ioctlTIOCGWINSZ(struct winsize* w)
{
   int fd = open("/dev/tty", O_RDWR);
   if(fd >= 0) {
      if(ioctl(fd, TIOCGWINSZ, w) == 0) {
         close(fd);
         return true;
      }
      close(fd);
   }
   return false;
}


// ###### Obtain console width ##############################################
static int getConsoleWidth()
{
   struct winsize w;
   if(ioctlTIOCGWINSZ(&w)) {
      return w.ws_col;
   }
   return 80;   // Use default!
}


// ###### Digit to number ###################################################
static unsigned int hexDigitToNumber(const char digit)
{
   if( (digit >= '0') && (digit <= '9') ) {
      return (int)digit - (int)'0';
   }
   else if( (digit >= 'a') && (digit <= 'f') ) {
      return 10 + ((int)digit - (int)'a');
   }
   else if( (digit >= 'A') && (digit <= 'F') ) {
      return 10 + ((int)digit - (int)'A');
   }
   fprintf(stderr, "hexDigitToNumber: Bad digit %c!\n", digit);
   exit(1);
}


// ###### Unescape string ###################################################
static char* unescape(const char* originalString)
{
   size_t original_string_length = strlen(originalString);
   char*  unescapedString        = malloc(original_string_length + 1);
   if(unescapedString == NULL) {
      fputs("ERROR: malloc() failed!\n", stderr);
      exit(1);
   }

   size_t i = 0;
   size_t j = 0;
   while(i < original_string_length) {
      if(originalString[i] != '\\') {
         unescapedString[j++] = originalString[i];
      }
      else {
         if(i + 1 < original_string_length) {
            i++;
            switch(originalString[i]) {
               case 'n':
                  unescapedString[j++] = '\n';
                break;
               case 'r':
                  unescapedString[j++] = '\r';
                break;
               case '\\':
                  unescapedString[j++] = '\\';
                break;
               case 'x':
                  if(i + 2 < original_string_length) {
                    const unsigned int c =
                        (hexDigitToNumber(originalString[i + 1]) << 4) +
                        hexDigitToNumber(originalString[i + 2]);
                     unescapedString[j++] = (char)c;
                     i += 2;
                  }
                break;
               case '0':
                  if(i + 3 < original_string_length) {
                     const unsigned int c =
                        (hexDigitToNumber(originalString[i + 1]) * 100) +
                        (hexDigitToNumber(originalString[i + 2]) * 10) +
                        hexDigitToNumber(originalString[i + 3]);
                     unescapedString[j++] = (char)c;
                     i += 3;
                  }
                break;
               case 'u':
                  if(i + 4 < original_string_length) {
                    const unsigned int c1 =
                        (hexDigitToNumber(originalString[i + 1]) << 4) +
                        hexDigitToNumber(originalString[i + 2]);
                    const unsigned int c2 =
                        (hexDigitToNumber(originalString[i + 3]) << 4) +
                        hexDigitToNumber(originalString[i + 4]);
                     unescapedString[j++] = (char)c1;
                     unescapedString[j++] = (char)c2;
                     i += 4;
                  }
                break;
               case 'U':
                  if(i + 8 < original_string_length) {
                    const unsigned int c1 =
                        (hexDigitToNumber(originalString[i + 1]) << 4) +
                        hexDigitToNumber(originalString[i + 2]);
                    const unsigned int c2 =
                        (hexDigitToNumber(originalString[i + 3]) << 4) +
                        hexDigitToNumber(originalString[i + 4]);
                    const unsigned int c3 =
                        (hexDigitToNumber(originalString[i + 5]) << 4) +
                        hexDigitToNumber(originalString[i + 6]);
                    const unsigned int c4 =
                        (hexDigitToNumber(originalString[i + 7]) << 4) +
                        hexDigitToNumber(originalString[i + 8]);
                     unescapedString[j++] = (char)c1;
                     unescapedString[j++] = (char)c2;
                     unescapedString[j++] = (char)c3;
                     unescapedString[j++] = (char)c4;
                     i += 8;
                  }
                break;
               case 'a':
                  unescapedString[j++] = '\a';
                break;
               case 'b':
                  unescapedString[j++] = '\b';
                break;
               case 'e':
               case 'E':
                  unescapedString[j++] = '\e';
                break;
               case 'f':
                  unescapedString[j++] = '\f';
                break;
               case 't':
                  unescapedString[j++] = '\t';
                break;
               case 'v':
                  unescapedString[j++] = '\v';
                break;
               default:
                  unescapedString[j++] = '\\';
                  unescapedString[j++] = originalString[i];
                break;
            }
         }
         else {
            unescapedString[j++] = originalString[i];
         }
      }
      i++;
   }
   unescapedString[j] = 0x00;

   return unescapedString;
}


// ###### Convert UTF-8 string to wide string without ANSI sequences ########
wchar_t* convertToWideStringWithoutANSI(const char* originalString,
                                        const bool  removeANSISequences,
                                        size_t*     utf8Size,
                                        size_t*     wideLength)
{
   const size_t original_string_length = strlen(originalString);

   // ====== Copy string, but filter out ANSI colouring sequences ===========
   char* utf8String = malloc(original_string_length + 1);
   if(utf8String == NULL) {
      fputs("ERROR: malloc() failed!\n", stderr);
      exit(1);
   }

   bool   inANSISequence = false;
   size_t j = 0;
   for(size_t i = 0; i < original_string_length; i++) {
      if(!inANSISequence) {
         if((originalString[i] == '\e') || (!removeANSISequences)) {
            inANSISequence = true;
         }
         else {
            utf8String[j++] = originalString[i];
         }
      }
      else {
         if(isalpha(originalString[i])) {
            inANSISequence = false;
         }
      }
   }
   utf8String[j] = 0x00;

   // ====== Get string width ===============================================
   wchar_t* wide_string = (wchar_t*)malloc(sizeof(wchar_t) * (j + 1));
   if(wide_string == NULL) {
      fputs("ERROR: malloc() failed!\n", stderr);
      exit(1);
   }
   const size_t wide_string_length = mbstowcs(wide_string, utf8String, j);
   if(wide_string_length < 0) {
      fputs("ERROR: mbstowcs() failed!\n", stderr);
      exit(1);
   }

   if(utf8Size) {
      *utf8Size = j;
   }
   if(wideLength) {
      *wideLength = wide_string_length;
   }

   free(utf8String);
   return wide_string;
}


// ###### Obtain console printing width of UTF-8 string ######################
static int stringwidth(const char* originalString,
                       const bool  removeANSISequences)
{
   size_t   wide_string_length;
   wchar_t* wide_string = convertToWideStringWithoutANSI(
                             originalString, removeANSISequences,
                             NULL, &wide_string_length);
   const int width = wcswidth(wide_string, wide_string_length);
   free(wide_string);
   return width;
}


// ###### Obtain length of UTF-8 string in bytes ############################
static void stringSizeLengthWidth(const char* originalString,
                                  const bool  removeANSISequences,
                                  const bool  showSize,
                                  const bool  showLength,
                                  const bool  showWidth)
{
   size_t   wide_string_length;
   size_t   utf8_string_size;
   wchar_t* wide_string = convertToWideStringWithoutANSI(
                             originalString, removeANSISequences,
                             &utf8_string_size, &wide_string_length);
   const char* space = "";
   if(showSize) {
      printf("%zd", utf8_string_size);
      space = " ";
   }
   if(showLength) {
      printf("%s%zd", space, wide_string_length);
      space = " ";
   }
   if(showWidth) {
      printf("%s%d", space, wcswidth(wide_string, wide_string_length));
      // space = " ";
   }
   puts("");

   free(wide_string);
}


// ###### Get terminal information ##########################################
static void terminalInfo()
{
   struct winsize w;
   if(!ioctlTIOCGWINSZ(&w)) {
      // Use defaults!
      w.ws_col = 80;
      w.ws_row = 24;
   }
   printf("%u %u\n", (unsigned int)w.ws_col, (unsigned int)w.ws_row);
}


// ###### Indent with given number of spaces ################################
static void indent(const int indentWidth)
{
   if(indentWidth > 0) {
      char indentString[indentWidth + 1];
      for(int i = 0; i < indentWidth; i++) {
         indentString[i] = ' ';
      }
      indentString[indentWidth] = 0x00;
      fputs(indentString, stdout);
   }
}


// ###### Indent ############################################################
static void indented(const int   totalIndent,
                     const char* utf8String)
{
   if(totalIndent > 0) {
      indent( totalIndent - stringwidth(utf8String, true) );
   }
   fputs(utf8String, stdout);
   if(totalIndent < 0) {
      indent( (-totalIndent) - stringwidth(utf8String, true) );
   }
}


// ###### Center ############################################################
static void centered(const char* utf8String,
                     const int   consoleWidth,
                     const bool  padding)
{
   const int additionalSpacing = consoleWidth - stringwidth(utf8String, true);
   const int indentation       = additionalSpacing / 2;

   indent(indentation);
   fputs(utf8String, stdout);
   if(padding) {
      // NOTE: Uneven values of additionalSpacing needs to be handled here,
      //       i.e. compute the
      indent(additionalSpacing - indentation);
   }
}


// ###### Center ############################################################
static void separator(const char* separaterBorderLeft,
                      const char* separatorString,
                      const char* separaterBorderRight,
                      const int   consoleWidth)
{
   const int separaterLeftWidth   = stringwidth(separaterBorderLeft, true);
   const int separatorStringWidth = stringwidth(separatorString, true);
   const int separaterRightWidth  = stringwidth(separaterBorderRight, true);

   int i = separaterLeftWidth + separaterRightWidth;   // Border width
   fputs(separaterBorderLeft, stdout);
   while(i + separatorStringWidth <= consoleWidth) {
      fputs(separatorString, stdout);
      i += separatorStringWidth;
   }
   fputs(separaterBorderRight, stdout);
}


// ###### Multi-Line Center #################################################
static void doMultiLineIndentOrCenter(const char*       borderLeft,
                                      const char*       borderRight,
                                      int               consoleWidth,
                                      const int         indentWidth,
                                      const printmode_t mode)
{
   char*        lineArray[1024];
   unsigned int lineLength[1024];
   unsigned int lines = 0;

   // ====== Read lines from stdin ==========================================
   char*        s;
   char         buffer[8192];
   unsigned int maxLength = 0;
   while( (s = fgets((char*)&buffer, sizeof(buffer), stdin)) != NULL ) {
      buffer[strcspn(buffer, "\r\n")] = 0x00;   // Remove newline
      lineArray[lines] = strdup(s);
      if(lineArray[lines] == NULL) {
         exit(1);
      }
      lineLength[lines] = strlen(lineArray[lines]);
      if(lineLength[lines]  > maxLength) {
         maxLength = lineLength[lines] ;
      }
      lines++;
      if(lines >= sizeof(lineArray) / sizeof(lineArray[0])) {
         break;
      }
   }

   // ====== Obtain left and right border widths ============================
   size_t   wll;
   wchar_t* wsl = convertToWideStringWithoutANSI(borderLeft, true, NULL, &wll);
   if(wsl) {
      const int borderLeftWidth = wcswidth(wsl, wll);
      if(borderLeftWidth > 0) {
         consoleWidth -= borderLeftWidth;
      }
      free(wsl);
   }
   size_t   wlr;
   wchar_t* wsr = convertToWideStringWithoutANSI(borderRight, true, NULL, &wlr);
   if(wsr) {
      const int borderRightWidth = wcswidth(wsr, wlr);
      if(borderRightWidth > 0) {
         consoleWidth -= borderRightWidth;
      }
      free(wsr);
   }

   // ====== Print result ===================================================
   for(unsigned int i = 0; i < lines; i++) {
      fputs(borderLeft, stdout);
      if(mode == MultiLineIndent) {
         indented(indentWidth, lineArray[i]);
      }
      else {
         centered(lineArray[i], consoleWidth, true);
      }
      fputs(borderRight, stdout);
      if(i + 1 < lines) {
         puts("");
      }
      free(lineArray[i]);
      lineArray[i] = NULL;
   }
}



// ###### Main program ######################################################
int main (int argc, char** argv)
{
   // ====== Initialise locale support ======================================
   if(setlocale(LC_ALL, "") == NULL) {
      setlocale(LC_ALL, "C.UTF-8");   // "C" should exist on all systems!
   }

   // ====== Handle arguments ===============================================
   printmode_t mode                = Indent;
   int         indentWidth         = 0;
   char*       utf8String          = NULL;
   char*       borderLeft          = NULL;
   char*       borderRight         = NULL;
   bool        newline             = false;
   const int   defaultConsoleWidth = getConsoleWidth();
   int         consoleWidth        = defaultConsoleWidth;

   for(int i = 1; i <argc; i++) {
      if( (strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--indent") == 0) ) {
         if(i + 1 < argc) {
            indentWidth = atoi(argv[i + 1]);
         }
         else {
            fputs("ERROR: Invalid indentation setting!\n", stderr);
            return 1;
         }
         mode = Indent;
         i++;
      }
      else if( (strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--center") == 0) ) {
         mode = Center;
      }
      else if( (strcmp(argv[i], "-I") == 0) || (strcmp(argv[i], "--multiline-indent") == 0) ) {
         mode = MultiLineIndent;
         if(i + 3 < argc) {
            indentWidth = atoi(argv[i + 1]);
            borderLeft  = unescape(argv[i + 2]);
            borderRight = unescape(argv[i + 3]);
         }
         else {
            fputs("ERROR: Invalid arguments for multiline-ident!\n", stderr);
            return 1;
         }
      }
      else if( (strcmp(argv[i], "-C") == 0) || (strcmp(argv[i], "--multiline-center") == 0) ) {
         mode = MultiLineCenter;
         if(i + 2 < argc) {
            borderLeft  = unescape(argv[i + 1]);
            borderRight = unescape(argv[i + 2]);
         }
         else {
            fputs("ERROR: Invalid arguments for multiline-center!\n", stderr);
            return 1;
         }
      }
      else if( (strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--separator") == 0) ) {
         if(i + 3 < argc) {
            borderLeft  = unescape(argv[i + 1]);
            borderRight = unescape(argv[i + 3]);
            if(utf8String) {
               free(utf8String);
            }
            utf8String  = unescape(argv[i + 2]);
         }
         else {
            fputs("ERROR: Invalid separator setting!\n", stderr);
            return 1;
         }
         mode = Separator;
         i += 3;
      }
      else if( (strcmp(argv[i], "-n") == 0) || (strcmp(argv[i], "--newline") == 0) ) {
         newline = true;
      }
      else if( (strcmp(argv[i], "-t") == 0) || (strcmp(argv[i], "--terminal-info") == 0) ) {
         mode = TerminalInfo;
      }
      else if( (strcmp(argv[i], "-x") == 0) || (strcmp(argv[i], "--columns") == 0) ) {
         if(i + 1 < argc) {
            consoleWidth = atol(argv[i + 1]);
            if( consoleWidth <= 0) {
               consoleWidth = defaultConsoleWidth + consoleWidth;   // subtract!
            }
            if (consoleWidth > 4096) {
               fprintf(stderr, "ERROR: Invalid console width %u!\n", consoleWidth);
               return 1;
            }
         }
         i += 1;
      }
      else if( (strcmp(argv[i], "-w") == 0) || (strcmp(argv[i], "--width") == 0) ) {
         mode = Width;
      }
      else if( (strcmp(argv[i], "-l") == 0) || (strcmp(argv[i], "--length") == 0) ) {
         mode = Length;
      }
      else if( (strcmp(argv[i], "-b") == 0) || (strcmp(argv[i], "--size") == 0) ) {
         mode = Size;
      }
      else if( (strcmp(argv[i], "-a") == 0) || (strcmp(argv[i], "--size-length-width") == 0) ) {
         mode = SizeLengthWidth;
      }
      else if( (strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0) ) {
         fprintf(stderr, "Usage: %s [-n|--newline] [-i indentation string|--ident indentation string] [-c string|--center string] [-I left right|--multiline-indent indentation left right]  [-C left right|--multiline-center left right] [-s border_left separator border_right|--separator border_left separator border_right] [-c columns|--columns columns] [-s string|--size string] [-l string|--length string] [-w string|--width string] [-a string|--size-length-width string] [-t|--terminal-info] [-h|--help] [-v|--version]\n", argv[0]);
         return 1;
      }
      else if( (strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--version") == 0) ) {
         printf("printf-utf8 %s\n", SYSTEMTOOLS_VERSION);
         return 0;
      }
      else {
         if(utf8String) {
            free(utf8String);
         }
         utf8String = unescape(argv[i]);
         break;
      }
   }

   // ====== Handle command =================================================
   if( (utf8String) || (mode ==  TerminalInfo) ) {
      switch(mode) {
         case Indent:
            indented(indentWidth, utf8String);
          break;
         case Center:
            centered(utf8String, consoleWidth, false);
          break;
         case MultiLineIndent:
         case MultiLineCenter:
            doMultiLineIndentOrCenter(borderLeft, borderRight,
                                      consoleWidth, indentWidth, mode);
          break;
         case Separator:
            separator(borderLeft, utf8String, borderRight, consoleWidth);
          break;
         case Width:
            stringSizeLengthWidth(utf8String, true, false, false, true);
          break;
         case Length:
            stringSizeLengthWidth(utf8String, true, false, true, false);
          break;
         case Size:
            stringSizeLengthWidth(utf8String, true, true, false, false);
          break;
         case SizeLengthWidth:
            stringSizeLengthWidth(utf8String, true, true, true, true);
          break;
         case TerminalInfo:
            terminalInfo();
          break;
      }
   }
   if(newline) {
      fputs("\n", stdout);
   }

   if(utf8String) {
      free(utf8String);
   }
   if(borderLeft) {
      free(borderLeft);
   }
   if(borderRight) {
      free(borderRight);
   }

   return 0;
}
