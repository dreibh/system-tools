#include <stdlib.h>
#include <strings.h>

#include "color-tables.h"


static const ColorTableEntry ColorTableANSI[] = {
#include "colors-ansi.h"
};
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
static const ColorTableEntry* colorLookupFromTable(const ColorTableEntry* table,
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


// ###### Find entry in ANSI color table #####################################
const ColorTableEntry* colorLookupANSI(const char* ansiColorName)
{
   return colorLookupFromTable((const ColorTableEntry*)&ColorTableANSI,
                               sizeof(ColorTableANSI) / sizeof(ColorTableANSI[0]),
                               ansiColorName);
}


// ###### Find entry in X11 color table #####################################
const ColorTableEntry* colorLookupX11(const char* x11ColorName)
{
   return colorLookupFromTable((const ColorTableEntry*)&ColorTableX11,
                               sizeof(ColorTableX11) / sizeof(ColorTableX11[0]),
                               x11ColorName);
}


// ###### Find entry in HTML color table #####################################
const ColorTableEntry* colorLookupHTML(const char* htmlColorName)
{
   return colorLookupFromTable((const ColorTableEntry*)&ColorTableHTML,
                               sizeof(ColorTableHTML) / sizeof(ColorTableHTML[0]),
                               htmlColorName);
}
