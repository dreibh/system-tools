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
static int getConsoleWidth()
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
         if((originalString[i] == 0x1b) || (!removeANSISequences)) {
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
   if(showSize) {
      printf(" %zd", utf8_string_size);
   }
   if(showLength) {
      printf(" %zd", wide_string_length);
   }
   if(showWidth) {
      const int width = wcswidth(wide_string, wide_string_length);
      printf(" %d", wcswidth(wide_string, wide_string_length));
   }
   puts("");
}


// ###### Get terminal information ##########################################
static int terminalInfo()
{
   struct winsize w;
   ioctlTIOCGWINSZ(&w);
   printf("%u %u\n", (unsigned int)w.ws_col, (unsigned int)w.ws_row);
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
      indent( totalIndent - stringwidth(utf8String, true) );
   }
   fputs(utf8String, stdout);
   if(totalIndent < 0) {
      indent( (-totalIndent) - stringwidth(utf8String, true) );
   }
}


// ###### Center ############################################################
static void centered(const char* utf8String)
{
   indent( (getConsoleWidth() - stringwidth(utf8String, true)) / 2 );
   fputs(utf8String, stdout);
}


// ###### Center ############################################################
static void separator(const char* separaterLeftBorder,
                      const char* separatorString,
                      const char* separaterRightBorder)
{
   const int separaterLeftWidth   = stringwidth(separaterLeftBorder, true);
   const int separatorStringWidth = stringwidth(separatorString, true);
   const int separaterRightWidth  = stringwidth(separaterRightBorder, true);
   const int consoleWidth         = getConsoleWidth();

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
      Indent          = 1,
      Center          = 2,
      Separator       = 3,
      Width           = 4,
      TerminalInfo    = 5,
      Length          = 6,
      Size            = 7,
      SizeLengthWidth = 8
   };
   enum mode_t mode        = Indent;
   int         indentWidth = 0;
   char*       utf8String  = NULL;
   char*       borderLeft  = NULL;
   char*       borderRight = NULL;
   bool        newline     = false;
   for(int i = 1; i <argc; i++) {
      if( (strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--center") == 0) ) {
         mode = Center;
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
            borderLeft  = unescape(argv[i + 1]);
            utf8String  = unescape(argv[i + 2]);
            borderRight = unescape(argv[i + 3]);
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
         fprintf(stderr, "Usage: %s [-n|--newline] [-i indentation string|--ident indentation string] [-c string|--center string] [-s border_left separator border_right|--separator border_left separator border_right] [-s string|--size string] [-l string|--length string] [-w string|--width string] [-a string|--size-length-width string] [-t|--terminal-info]\n", argv[0]);
         return 1;
      }
      else {
         utf8String = unescape(argv[i]);
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
