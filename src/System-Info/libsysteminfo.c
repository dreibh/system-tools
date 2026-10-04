// ==========================================================================
//         ____            _                     _____           _
//        / ___| _   _ ___| |_ ___ _ __ ___     |_   _|__   ___ | |___
//        \___ \| | | / __| __/ _ \ '_ ` _ \ _____| |/ _ \ / _ \| / __|
//         ___) | |_| \__ \ ||  __/ | | | | |_____| | (_) | (_) | \__ \.
//        |____/ \__, |___/\__\___|_| |_| |_|     |_|\___/ \___/|_|___/
//               |___/
//                             --- System-Tools ---
//                  https://www.nntb.no/~dreibh/system-tools/
// ==========================================================================
//
// System-Info Library
// Copyright (C) 2024-2026 by Thomas Dreibholz
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Contact: thomas.dreibholz@gmail.com


#include <stdint.h>
#include <string.h>
#include <redblacktree.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ < 202311L)
#ifndef nullptr
#define nullptr ((void*)0)
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif


struct SystemInfo
{
   struct RedBlackTree Tree;
   unsigned int        Error;
};

typedef enum {
   SIET_INVALID = 0,
   SIET_STRING  = 1,
   SIET_UINT32  = 2,
   SIET_UINT64  = 3,
   SIET_INT32   = 4,
   SIET_INT64   = 5,
   SIET_DOUBLE  = 6
} SystemInfoEntryValueType;

typedef union {
   char*    String;
   uint32_t UInt32;
   uint64_t UInt64;
   int32_t  Int32;
   int64_t  Int64;
   double   Double;
} SystemInfoEntryValue;

struct SystemInfoEntry
{
   struct RedBlackTreeNode  Node;
   char*                    Key;
   SystemInfoEntryValue     Value;
   SystemInfoEntryValueType ValueType;
};



struct SystemInfo* systemInfoObtain(unsigned int flags);
void systemInfoRelease(struct SystemInfo* systemInfo);
void systemInfoPrint(const struct SystemInfo* systemInfo, FILE* fd);




static void systemInfoEntryPrint(const void* node, FILE* fd)
{
   const struct SystemInfoEntry* systemInfoEntry = (const struct SystemInfoEntry*)node;
   switch(systemInfoEntry->ValueType) {
      case SIET_STRING:
         printf("%s=%s\n", systemInfoEntry->Key, systemInfoEntry->Value.String);
       break;
      case SIET_INT32:
         printf("%s=%d\n", systemInfoEntry->Key, (unsigned int)systemInfoEntry->Value.Int32);
       break;
      case SIET_INT64:
         printf("%s=%lld\n", systemInfoEntry->Key, (unsigned long long)systemInfoEntry->Value.Int64);
       break;
      case SIET_UINT32:
         printf("%s=%u\n", systemInfoEntry->Key, (unsigned int)systemInfoEntry->Value.UInt32);
       break;
      case SIET_UINT64:
         printf("%s=%llu\n", systemInfoEntry->Key, (unsigned long long)systemInfoEntry->Value.UInt64);
       break;
      case SIET_DOUBLE:
         printf("%s=%lf\n", systemInfoEntry->Key, systemInfoEntry->Value.Double);
       break;
      default:
         printf("%s=(INVALID!)\n", systemInfoEntry->Key);
       break;
   }
}

static int systemInfoEntryComparison(const void* node1, const void* node2)
{
   const struct SystemInfoEntry* systemInfoEntry1 = (const struct SystemInfoEntry*)node1;
   const struct SystemInfoEntry* systemInfoEntry2 = (const struct SystemInfoEntry*)node2;
   return(strcmp(systemInfoEntry1->Key, systemInfoEntry2->Key));
}


static struct SystemInfoEntry* systemInfoMakeEntry(struct SystemInfo* systemInfo,
                                                   const char*        key)
{
   struct SystemInfoEntry* systemInfoEntry =
      (struct SystemInfoEntry*)malloc(sizeof(struct SystemInfoEntry));
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      redBlackTreeNodeNew(&systemInfoEntry->Node);
      systemInfoEntry->Key          = strdup(key);
      systemInfoEntry->ValueType    = SIET_INVALID;
      if(__builtin_expect(systemInfoEntry->Key != nullptr, 1)) {
         if(__builtin_expect(redBlackTreeInsert(&systemInfo->Tree, &systemInfoEntry->Node) == &systemInfoEntry->Node, 1)) {
            return systemInfoEntry;
         }
         fprintf(stderr, "INTERNAL ERROR: Tried to add duplicate key \"%s\"!\n", key);
      }
      free(systemInfoEntry);
   }
   systemInfo->Error = 1;
   return nullptr;
}


static void systemInfoDisposeEntry(struct SystemInfoEntry* systemInfoEntry)
{
   if(systemInfoEntry->ValueType == SIET_STRING) {
      free(systemInfoEntry->Value.String);
   }
   free(systemInfoEntry->Key);
   free(systemInfoEntry);
}



static void systemInfoAddString(struct SystemInfo* systemInfo, const char* key, const char* value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoMakeEntry(systemInfo, key);
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      systemInfoEntry->Value.String = strdup(value);
      if(systemInfoEntry->Value.String) {
         systemInfoEntry->ValueType = SIET_STRING;
      }
      else {
         systemInfo->Error = 1;
      }
   }
}


static void systemInfoAddInt32(struct SystemInfo* systemInfo, const char* key, const int32_t value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoMakeEntry(systemInfo, key);
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      systemInfoEntry->Value.Int32 = value;
      systemInfoEntry->ValueType    = SIET_INT32;
   }
}


static void systemInfoAddInt64(struct SystemInfo* systemInfo, const char* key, const int64_t value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoMakeEntry(systemInfo, key);
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      systemInfoEntry->Value.Int64 = value;
      systemInfoEntry->ValueType    = SIET_INT64;
   }
}


static void systemInfoAddUInt32(struct SystemInfo* systemInfo, const char* key, const uint32_t value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoMakeEntry(systemInfo, key);
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      systemInfoEntry->Value.UInt32 = value;
      systemInfoEntry->ValueType    = SIET_UINT32;
   }
}


static void systemInfoAddUInt64(struct SystemInfo* systemInfo, const char* key, const uint64_t value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoMakeEntry(systemInfo, key);
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      systemInfoEntry->Value.UInt64 = value;
      systemInfoEntry->ValueType    = SIET_UINT64;
   }
}


static void systemInfoAddDouble(struct SystemInfo* systemInfo, const char* key, const double value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoMakeEntry(systemInfo, key);
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      systemInfoEntry->Value.Double = value;
      systemInfoEntry->ValueType    = SIET_DOUBLE;
   }
}



struct SystemInfo* systemInfoObtain(unsigned int flags)
{
   struct SystemInfo* systemInfo = malloc(sizeof(struct SystemInfo));
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      redBlackTreeNew(&systemInfo->Tree, systemInfoEntryPrint, systemInfoEntryComparison);
      systemInfo->Error = 0;

      systemInfoAddString(systemInfo, "name", "Test");
      systemInfoAddUInt32(systemInfo, "uint32", 32);
      systemInfoAddUInt64(systemInfo, "uint64", 64);
      systemInfoAddInt32(systemInfo,  "int32", -32);
      systemInfoAddInt64(systemInfo,  "int64", -64);
      systemInfoAddString(systemInfo, "block", "xxx");
      systemInfoAddDouble(systemInfo, "pi", 3.1415);

      if(systemInfo->Error) {
         systemInfoRelease(systemInfo);
         systemInfo = nullptr;
      }
   }
   return systemInfo;
}


void systemInfoRelease(struct SystemInfo* systemInfo)
{
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      struct SystemInfoEntry* systemInfoEntry =
         (struct SystemInfoEntry*)redBlackTreeGetFirst(&systemInfo->Tree);
      while(systemInfoEntry) {
         redBlackTreeRemove(&systemInfo->Tree, &systemInfoEntry->Node);
         systemInfoDisposeEntry(systemInfoEntry);
         systemInfoEntry = (struct SystemInfoEntry*)redBlackTreeGetFirst(&systemInfo->Tree);
      }
      redBlackTreeDelete(&systemInfo->Tree);
      free(systemInfo);
   }
}


void systemInfoPrint(const struct SystemInfo* systemInfo, FILE* fd)
{
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      redBlackTreePrint(&systemInfo->Tree, fd);
   }
}


#ifdef __cplusplus
}
#endif


int main(int argc, char** argv)
{
   struct SystemInfo* systemInfo = systemInfoObtain(0);
   if(systemInfo) {
      systemInfoPrint(systemInfo, stdout);
      systemInfoRelease(systemInfo);
   }
}
