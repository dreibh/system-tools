#include <getopt.h>
#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#ifndef nullptr
#define nullptr NULL
#endif

#include "color-tables.h"

#include "package-version.h"


// ###### Version ###########################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void version()
{
   printf("print-color %s\n", SYSTEMTOOLS_VERSION);
   exit(0);
}


// ###### Usage #############################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void usage(const char* program, const int exitCode)
{
   fprintf(stderr, "%s %s ..."
           " [-h|--help]"
           " [-v|--version]\n",
           gettext("Usage:"), program);
   exit(exitCode);
}



// ###### Main program ######################################################
int main (int argc, char** argv)
{
   const char* foreground = nullptr;

   // ====== Initialise i18n support ========================================
   if(setlocale(LC_ALL, "") == nullptr) {
      setlocale(LC_ALL, "C.UTF-8");   // "C" should exist on all systems!
   }
   bindtextdomain("random-sleep", nullptr);
   textdomain("random-sleep");

   // ====== Handle arguments ===============================================
   const static struct option long_options[] = {
      { "help",    no_argument, 0, 'h' },
      { "version", no_argument, 0, 'v' },
      {  nullptr,  0,           0, 0   }
   };

   int option;
   int longIndex;
   while( (option = getopt_long(argc, argv, "hv", long_options, &longIndex)) != -1 ) {
      switch(option) {
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
   if(optind + 1 == argc) {
      foreground = argv[1];
   }
   else if(optind != argc) {
      usage(argv[0], 1);
   }

   // ====== Perform configured operations ==================================
   if(foreground) {
      printf("Lookup: %s\n", foreground);
      const ColorTableEntry* entry = colorLookupX11(foreground);
      if(entry) {
         printf("Name: %s \e[38;2;%d;%d;%dm****** This is a test of %s ******\e[0m\n",
               entry->Name,
               entry->Value.RGB.Red, entry->Value.RGB.Green, entry->Value.RGB.Blue,
               entry->Name);
         printf("ANSI Sequence: \\x1b[38;2;%d;%d;%dm\n",
                entry->Value.RGB.Red, entry->Value.RGB.Green, entry->Value.RGB.Blue);
      }
      else {
         puts("not found");
      }
   }

   return 0;
}
