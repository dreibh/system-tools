#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <strings.h>
#include <unistd.h>
// #include <string.h>

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
                                          const unsigned int            entries,
                                          const char*                   colorName)
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



// ###### Main program ######################################################
int main (int argc, char** argv)
{
   const char* name = "goldenrod";

   const ColorTableEntry* entry = colorLookupX11(name);
   if(entry) {
      printf("Name: %s \e[38;2;%d;%d;%dm****** This is a test of %s ******\e[0m\n",
             entry->Name,
             entry->Value.RGB.Red, entry->Value.RGB.Green, entry->Value.RGB.Blue,
             entry->Name);
   }
   else {
      puts("not found");
   }

   return 0;
}
