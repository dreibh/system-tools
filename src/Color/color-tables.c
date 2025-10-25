#include <stdlib.h>
#include <strings.h>

#include "color-tables.h"


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
static const ColorTableEntry* colorLookup(const ColorTableEntry* table,
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
