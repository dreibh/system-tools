#include <getopt.h>
#include <libintl.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#ifndef nullptr
#define nullptr NULL
#endif

#include "package-version.h"


typedef enum
{
   CTET_NONE  = 0,
   CTET_INDEX = 1,
   CTET_RGB   = 2
} ColorTableEntryType;

typedef union ColorTableEntryValue
{
   uint8_t    Index;
   struct {
      uint8_t Red;
      uint8_t Green;
      uint8_t Blue;
   } RGB;
} ColorTableEntryValue;

typedef struct
{
   const char*                Name;
   uint8_t                    Type;
   union ColorTableEntryValue Value;
} ColorTableEntry;


static const ColorTableEntry ColorTableX11[] = {
#include "colors-x11.h"
};
static const ColorTableEntry ColorTableHTML[] = {
#include "colors-html.h"
};


// ###### Compare two color table entries ###################################
static int comareColorTableEntries(const void* leftPtr, const void* rightPtr)
{
   const ColorTableEntry* leftValue  = (const ColorTableEntry*)leftPtr;
   const ColorTableEntry* rightValue = (const ColorTableEntry*)rightPtr;
   return strcasecmp(leftValue->Name, rightValue->Name);
}


// ###### Find entry in color table #########################################
const ColorTableEntry* colorLookup(const ColorTableEntry* table,
                                   const unsigned int     entries,
                                   const char*            colorName)
{
   const ColorTableEntry  key   = { colorName };
   const ColorTableEntry* found =
      (const ColorTableEntry*)bsearch(&key, table, entries,
                                      sizeof(ColorTableEntry),
                                      &comareColorTableEntries);
   return found;
}


// ###### Find entry in X11 color table #####################################
const ColorTableEntry* colorLookupX11(const char* x11ColorName)
{
   return colorLookup((const ColorTableEntry*)&ColorTableX11,
                      sizeof(ColorTableX11) / sizeof(ColorTableX11[0]),
                      x11ColorName);
}


// ###### Find entry in HTML color table #####################################
const ColorTableEntry* colorLookupX11HTML(const char* htmlColorName)
{
   return colorLookup((const ColorTableEntry*)&ColorTableHTML,
                      sizeof(ColorTableHTML) / sizeof(ColorTableHTML[0]),
                      htmlColorName);
}


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
