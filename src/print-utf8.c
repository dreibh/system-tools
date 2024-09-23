#define _XOPEN_SOURCE 1
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <sys/ioctl.h>


// ###### Obtain console width ##############################################
static int consolewidth()
{
   struct winsize w;
   if(ioctl(0, TIOCGWINSZ, &w) != 0) {
      perror("ioctl(TIOCGWINSZ)");
      exit(1);
   }
   return w.ws_col;
}


// ###### Obtain console printing width of UTF-8 string #####################
static int stringwidth(const char* utf8String)
{
   wchar_t      wide_string[8192];
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
      Indent    = 1,
      Center    = 2,
      Separator = 3
   };
   enum mode_t  mode        = Indent;
   int          indentWidth = 0;
   const char*  utf8String  = "";
   const char*  borderLeft  = NULL;
   const char*  borderRight = NULL;
   bool         newline     = false;
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
            borderLeft  = argv[i + 1];
            utf8String  = argv[i + 2];
            borderRight = argv[i + 3];
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
         fprintf(stderr, "Usage: %s [-n|--newline] [-i indentation string|--ident indentation string] [-c string|--center string] [-s border_left separator border_right|--separator border_left separator border_right]\n", argv[0]);
         return 1;
      }
      else {
         utf8String = argv[i];
         break;
      }
   }


   // Set locale to handle UTF-8 multibyte characters
   setlocale(LC_ALL, "");

   /*
   // ====== Convert UTF-8 string to wide string ============================
   const char* utf8_string = "Test öäüÆØÅ 托马斯博士 💻";

   // ====== Get console width ==============================================
   struct winsize w;
   if(ioctl(0, TIOCGWINSZ, &w) != 0) {
      perror("ioctl(TIOCGWINSZ)");
      return 1;
   }
   // printf("lines:   %d\n", w.ws_row);
   // printf("columns: %d\n", w.ws_col);

   // ------ Center ---------------------------------------------------------
   separator("<😀", "=", "😀>");
   fputs("\n", stdout);

   center(utf8_string);
   fputs("\n", stdout);

   separator("<😀", "=", "😀>");
   fputs("\n", stdout);

   indented(-30, utf8_string);
   fputs("Test\n", stdout);
   indented(30, utf8_string);
   fputs("Test\n", stdout);

   separator("<😀", "=", "😀>");
   fputs("\n", stdout);
   */

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
   }
   if(newline) {
      fputs("\n", stdout);
   }

   return 0;
}
