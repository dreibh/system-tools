#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <sys/ioctl.h>


int main (int argc, char** argv)
{
   // Set locale to handle UTF-8 multibyte characters
   setlocale(LC_ALL, "");

   // ====== Convert UTF-8 string to wide string ============================
   const char* utf8_string = "Test öäüÆØÅ 托马斯博士 💻";


   wchar_t      wide_string[8192];
   const size_t wide_string_length = mbstowcs((wchar_t*)&wide_string, utf8_string,
                                              sizeof(wide_string) / sizeof(wide_string[0]));
   if(wide_string_length < 0) {
      perror("mbstowcs");
      return 0;
   }

   printf("wcslen=%lu\n", wide_string_length);
   printf("wcswidth=%d\n", wcswidth(wide_string, wide_string_length));

   struct winsize w;
   ioctl(0, TIOCGWINSZ, &w);
   printf ("lines %d\n",   w.ws_row);
   printf ("columns %d\n", w.ws_col);

   return 0;
}
