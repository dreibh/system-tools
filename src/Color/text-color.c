#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <strings.h>
#include <unistd.h>
// #include <string.h>

struct ColorTableEntry
{
   const char* Name;
   uint8_t     Red;
   uint8_t     Green;
   uint8_t     Blue;
};


static const struct ColorTableEntry ColorTableX11[] = {
#include "colors-x11.h"
};
static const struct ColorTableEntry ColorTableHTML[] = {
#include "colors-html.h"
};


// ###### Compare two color table entries ###################################
static int comareColorTableEntries(const void* leftPtr, const void* rightPtr)
{
   const struct ColorTableEntry* leftValue  = (const struct ColorTableEntry*)leftPtr;
   const struct ColorTableEntry* rightValue = (const struct ColorTableEntry*)rightPtr;
   return strcasecmp(leftValue->Name, rightValue->Name);
}


// ###### Find entry in color table #########################################
const struct ColorTableEntry* colorLookup(const struct ColorTableEntry* table,
                                          const unsigned int            entries,
                                          const char*                   colorName)
{
   const struct ColorTableEntry  key   = { colorName, 0, 0, 0 };
   const struct ColorTableEntry* found =
      bsearch(&key, table, entries, sizeof(struct ColorTableEntry),
              &comareColorTableEntries);
   return found;
}


// ###### Find entry in X11 color table #####################################
const struct ColorTableEntry* colorLookupX11(const char* x11ColorName)
{
   return colorLookup((const struct ColorTableEntry*)&ColorTableX11,
                      sizeof(ColorTableX11) / sizeof(ColorTableX11[0]),
                      x11ColorName);
}


// ###### Find entry in HTML color table #####################################
const struct ColorTableEntry* colorLookupX11HTML(const char* htmlColorName)
{
   return colorLookup((const struct ColorTableEntry*)&ColorTableHTML,
                      sizeof(ColorTableHTML) / sizeof(ColorTableHTML[0]),
                      htmlColorName);
}



// ###### Main program ######################################################
int main (int argc, char** argv)
{
   const char* name = "goldenrod";

   const struct ColorTableEntry* entry = colorLookupX11(name);
   printf("entry=%p\n", entry);
   if(entry) {
      printf("entry->Name=%s\n", entry->Name);
   }

   return 0;
}
