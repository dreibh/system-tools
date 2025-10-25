#ifndef COLOR_TABLES_H
#define COLOR_TABLES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


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


const ColorTableEntry* colorLookupANSI(const char* ansiColorName);
const ColorTableEntry* colorLookupX11(const char* x11ColorName);
const ColorTableEntry* colorLookupX11HTML(const char* htmlColorName);


#ifdef __cplusplus
}
#endif

#endif
