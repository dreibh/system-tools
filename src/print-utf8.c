#define _XOPEN_SOURCE 1
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


// ###### Obtain console size ###############################################
static void ioctlTIOCGWINSZ(struct winsize* w)
{
   int fd = open("/dev/tty", O_RDWR);
   if(fd >= 0) {
      if(ioctl(fd, TIOCGWINSZ, w) != 0) {
         perror("ioctl(TIOCGWINSZ)");
         exit(1);
      }
      close(fd);
   }
   else {
      perror("open() failed for /dev/tty");
      exit(1);
   }
}


// ###### Obtain console width ##############################################
static int consolewidth()
{
   struct winsize w;
   ioctlTIOCGWINSZ(&w);
   return w.ws_col;
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
static void unescape(const char*  originalString,
                     char*        unescapedString,
                     const size_t maxLength)
{
   size_t original_string_length = strlen(originalString);
   if(original_string_length > maxLength - 1) {
      original_string_length = maxLength - 1;
   }
   size_t i = 0;
   size_t j = 0;
   while(i < original_string_length) {
      if(originalString[i] != '\\') {
         unescapedString[j++] = originalString[i];
      }
      else {
         i++;
         if(i < original_string_length) {
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
                  unescapedString[j++] = originalString[i];
                break;
            }
         }
      }
      i++;
   }
   unescapedString[j] = 0x00;
}


// ###### Obtain console printing widthq of UTF-8 string #####################
static int stringwidth(const char* originalString)
{

   // ====== Copy string, but filter out ANSI colouring sequences ===========
   char   utf8String[8192];
   size_t original_string_length = strlen(originalString);
   if(original_string_length > sizeof(utf8String) - 1) {
      original_string_length = sizeof(utf8String) - 1;
   }
   bool   inANSISequence = false;
   size_t j = 0;
   for(size_t i = 0; i < original_string_length; i++) {
      if(!inANSISequence) {
         if(originalString[i] == 0x1b) {
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
   wchar_t wide_string[sizeof(utf8String)];
   const size_t wide_string_length = mbstowcs((wchar_t*)&wide_string, utf8String,
                                              sizeof(wide_string) / sizeof(wide_string[0]));
   if(wide_string_length < 0) {
      perror("mbstowcs");
      exit(1);
   }
   return wcswidth(wide_string, wide_string_length);
}


// ###### Center ############################################################
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


// ###### Center ############################################################
static void indented(const int   totalIndent,
                     const char* utf8String)
{
   if(totalIndent > 0) {
      indent( totalIndent - stringwidth(utf8String) );
   }
   fputs(utf8String, stdout);
   if(totalIndent < 0) {
      indent( (-totalIndent) - stringwidth(utf8String) );
   }
}


// ###### Center ############################################################
static void centered(const char* utf8String)
{
   indent( (consolewidth() - stringwidth(utf8String)) / 2 );
   fputs(utf8String, stdout);
}


// ###### Center ############################################################
static void separator(const char* separaterLeftBorder,
                      const char* separatorString,
                      const char* separaterRightBorder)
{
   const int separaterLeftWidth   = stringwidth(separaterLeftBorder);
   const int separatorStringWidth = stringwidth(separatorString);
   const int separaterRightWidth  = stringwidth(separaterRightBorder);
   const int consoleWidth         = consolewidth();

   int i = separaterLeftWidth + separaterRightWidth;   // Border width
   fputs(separaterLeftBorder, stdout);
   while(i + separatorStringWidth <= consoleWidth) {
      fputs(separatorString, stdout);
      i += separatorStringWidth;
   }
   fputs(separaterRightBorder, stdout);
}




// ###### Main program ######################################################
int main (int argc, char** argv)
{
   // ====== Handle arguments ===============================================
   enum mode_t {
      Indent       = 1,
      Center       = 2,
      Separator    = 3,
      Width        = 4,
      TerminalInfo = 5
   };
   enum mode_t  mode        = Indent;
   int          indentWidth = 0;
   const char   utf8String[8192];
   const char   borderLeft[8192];
   const char   borderRight[8192];
   bool         newline     = false;
   for(int i = 1; i <argc; i++) {
      if( (strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--center") == 0) ) {
         mode = Center;
      }
      else if( (strcmp(argv[i], "-w") == 0) || (strcmp(argv[i], "--width") == 0) ) {
         mode = Width;
      }
      else if( (strcmp(argv[i], "-t") == 0) || (strcmp(argv[i], "--terminal-info") == 0) ) {
         mode = TerminalInfo;
      }
      else if( (strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--indent") == 0) ) {
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
      else if( (strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--separator") == 0) ) {
         if(i + 3 < argc) {
            unescape(argv[i + 1], (char*)&borderLeft,  sizeof(borderLeft));
            unescape(argv[i + 2], (char*)&utf8String,  sizeof(utf8String));
            unescape(argv[i + 3], (char*)&borderRight, sizeof(borderRight));
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
      else if( (strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0) ) {
         fprintf(stderr, "Usage: %s [-n|--newline] [-i indentation string|--ident indentation string] [-c string|--center string] [-s border_left separator border_right|--separator border_left separator border_right] [-w string|--width string] [-t|--terminal-info]\n", argv[0]);
         return 1;
      }
      else {
         unescape(argv[i], (char*)&utf8String, sizeof(utf8String));
         break;
      }
   }

   // ====== Handle command =================================================
   // Set locale to handle UTF-8 multibyte characters
   setlocale(LC_ALL, "");

   switch(mode) {
      case Indent:
         indented(indentWidth, utf8String);
       break;
      case Center:
         centered(utf8String);
       break;
      case Separator:
         separator(borderLeft, utf8String, borderRight);
       break;
      case Width:
         printf("%d\n", stringwidth(utf8String));
       break;
      case TerminalInfo:
         struct winsize w;
         ioctlTIOCGWINSZ(&w);
         printf("%u %u\n", (unsigned int)w.ws_col, (unsigned int)w.ws_row);
       break;
   }
   if(newline) {
      fputs("\n", stdout);
   }

   return 0;
}
