#define _XOPEN_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
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
   return wide_string_length;
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
static void left(const unsigned int totalIndent,
                 const char*        utf8String)
{
   fputs(utf8String, stdout);
   indent( totalIndent - stringwidth(utf8String) );
}


// ###### Center ############################################################
static void center(const char* utf8String)
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
   const int consoleWidth         = consolewidth() - 1;

   int i = separaterLeftWidth + separaterRightWidth;   // Border width
   fputs(separaterLeftBorder, stdout);
   while(i + separatorStringWidth < consoleWidth) {
      fputs(separatorString, stdout);
      i += separatorStringWidth;
   }
   fputs(separaterRightBorder, stdout);
}




// ###### Main program ######################################################
int main (int argc, char** argv)
{
   // Set locale to handle UTF-8 multibyte characters
   setlocale(LC_ALL, "");

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

   left(25, utf8_string);
   fputs("Test\n", stdout);

   separator("<😀", "=", "😀>");
   fputs("\n", stdout);

   return 0;
}
