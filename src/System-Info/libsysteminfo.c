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

#define _GNU_SOURCE
#define __EXTENSIONS__
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <ifaddrs.h>
#include <locale.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/utsname.h>
#if defined(__linux__)
#include <dirent.h>
#include <linux/if.h>
#include <netpacket/packet.h>
#include <sys/sysinfo.h>
#if defined(ENABLE_SYSTEMD)
#include <systemd/sd-login.h>
#else
#include <utmpx.h>
#endif
#elif defined(__FreeBSD__)
#include <dev/acpica/acpiio.h>
#include <net/if_dl.h>
#include <netlink/route/interface.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <utmpx.h>
#include <vm/vm_param.h>
#elif defined(__NetBSD__)
#include <net/if_dl.h>
#include <sys/envsys.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <utmpx.h>
#include <uvm/uvm_extern.h>
#include <sys/swap.h>
#elif defined(__OpenBSD__)
#include <machine/apmvar.h>
#include <net/if_dl.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <uvm/uvm_extern.h>
#include <sys/swap.h>
#include <utmp.h>
#elif defined(__sun__)
#include <dirent.h>
#include <kstat.h>
#include <sys/loadavg.h>
#include <sys/swap.h>
#include <utmpx.h>
#elif defined(__gnu_hurd__)
#include <dirent.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <utmpx.h>
#elif defined(__APPLE__)
#include <libproc.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/mach_time.h>
#include <net/if_dl.h>
#include <sys/sysctl.h>
#include <utmpx.h>
#else
#error Unknown system! The system-specific code parts need an update!
#endif
#ifdef ENABLE_NLS
#include <libintl.h>
#else
#define bindtextdomain(domain, dirname) { }
#define textdomain(domain) { }
#define gettext(string) string
#define ngettext(singular, plural, n) ((n) == 1 ? (singular) : (plural))
#endif

#include "libsysteminfo.h"
#include "package-version.h"
#include "redblacktree.h"

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
   char                     Key[32];
   SystemInfoEntryValue     Value;
   SystemInfoEntryValueType ValueType;
};


// ###### SystemInfoEntry print function ####################################
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


// ###### SystemInfoEntry comparison function ###############################
static int systemInfoEntryComparison(const void* node1, const void* node2)
{
   const struct SystemInfoEntry* systemInfoEntry1 = (const struct SystemInfoEntry*)node1;
   const struct SystemInfoEntry* systemInfoEntry2 = (const struct SystemInfoEntry*)node2;
   return(strcmp(systemInfoEntry1->Key, systemInfoEntry2->Key));
}


// ###### Add SystemInfoEntry to tree #######################################
static struct SystemInfoEntry* systemInfoAddEntry(struct SystemInfo* systemInfo,
                                                  const char*        key)
{
   struct SystemInfoEntry* systemInfoEntry =
      (struct SystemInfoEntry*)malloc(sizeof(struct SystemInfoEntry));
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      redBlackTreeNodeNew(&systemInfoEntry->Node);
      strlcpy(systemInfoEntry->Key, key, sizeof(systemInfoEntry->Key));
      systemInfoEntry->ValueType = SIET_INVALID;
      if(__builtin_expect(redBlackTreeInsert(&systemInfo->Tree, &systemInfoEntry->Node) == &systemInfoEntry->Node, 1)) {
         return systemInfoEntry;
      }
      fprintf(stderr, "INTERNAL ERROR: Tried to add duplicate key \"%s\"!\n", key);
   }
   systemInfo->Error = 1;
   return nullptr;
}


// ###### Find SystemInfoEntry in tree ######################################
static struct SystemInfoEntry* systemInfoFindEntry(struct SystemInfo* systemInfo,
                                                   const char*        key)
{
   struct SystemInfoEntry comparisonNode;
   strlcpy(comparisonNode.Key, key, sizeof(comparisonNode.Key));
   struct SystemInfoEntry* found =
      (struct SystemInfoEntry*)redBlackTreeFind(&systemInfo->Tree, &comparisonNode.Node);
   return found;
}


// ###### Remove SystemInfoEntry from tree ##################################
static void systemInfoRemoveEntry(struct SystemInfo*      systemInfo,
                                  struct SystemInfoEntry* systemInfoEntry)
{
   redBlackTreeRemove(&systemInfo->Tree, &systemInfoEntry->Node);
   if(systemInfoEntry->ValueType == SIET_STRING) {
      free(systemInfoEntry->Value.String);
   }
   free(systemInfoEntry);
}



// ###### Add key with string value #########################################
void systemInfoAddString(struct SystemInfo* systemInfo, const char* key, const char* value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoAddEntry(systemInfo, key);
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      systemInfoEntry->Value.String = strdup(value);
      if(__builtin_expect(systemInfoEntry->Value.String != nullptr, 1)) {
         systemInfoEntry->ValueType = SIET_STRING;
      }
      else {
         systemInfo->Error = 1;
      }
   }
}


// ###### Add key with 32-bit integer value #################################
void systemInfoAddInt32(struct SystemInfo* systemInfo,
                               const char* key, const int32_t value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoAddEntry(systemInfo, key);
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      systemInfoEntry->Value.Int32 = value;
      systemInfoEntry->ValueType    = SIET_INT32;
   }
}


// ###### Add key with 64-bit integer value #################################
void systemInfoAddInt64(struct SystemInfo* systemInfo,
                               const char* key, const int64_t value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoAddEntry(systemInfo, key);
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      systemInfoEntry->Value.Int64 = value;
      systemInfoEntry->ValueType    = SIET_INT64;
   }
}


// ###### Add key with 32-bit unsigned integer value ########################
void systemInfoAddUInt32(struct SystemInfo* systemInfo,
                                const char* key, const uint32_t value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoAddEntry(systemInfo, key);
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      systemInfoEntry->Value.UInt32 = value;
      systemInfoEntry->ValueType    = SIET_UINT32;
   }
}


// ###### Add key with 64-bit unsigned integer value ########################
void systemInfoAddUInt64(struct SystemInfo* systemInfo,
                                const char* key, const uint64_t value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoAddEntry(systemInfo, key);
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      systemInfoEntry->Value.UInt64 = value;
      systemInfoEntry->ValueType    = SIET_UINT64;
   }
}


// ###### Add key with double value #########################################
void systemInfoAddDouble(struct SystemInfo* systemInfo, const char* key, const double value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoAddEntry(systemInfo, key);
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      systemInfoEntry->Value.Double = value;
      systemInfoEntry->ValueType    = SIET_DOUBLE;
   }
}


// ###### Check whether key exists (returns value type or SIET_INVALID) #####
SystemInfoEntryValueType systemInfoExists(struct SystemInfo* systemInfo, const char* key)
{
   const struct SystemInfoEntry* systemInfoEntry = systemInfoFindEntry(systemInfo, key);
   if(systemInfoEntry) {
      return systemInfoEntry->ValueType;
   }
   return SIET_INVALID;
}


// ###### Get 32-bit integer value, or default if not found #################
int32_t systemInfoGetInt32(struct SystemInfo* systemInfo,
                           const char* key, const int32_t defaultValue)
{
   const struct SystemInfoEntry* systemInfoEntry = systemInfoFindEntry(systemInfo, key);
   if( (systemInfoEntry != nullptr) && (systemInfoEntry->ValueType == SIET_INT32) ) {
      return systemInfoEntry->Value.Int32;
   }
   return defaultValue;
}


// ###### Get 64-bit integer value, or default if not found #################
int64_t systemInfoGetInt64(struct SystemInfo* systemInfo,
                           const char* key, const int64_t defaultValue)
{
   const struct SystemInfoEntry* systemInfoEntry = systemInfoFindEntry(systemInfo, key);
   if( (systemInfoEntry != nullptr) && (systemInfoEntry->ValueType == SIET_INT64) ) {
      return systemInfoEntry->Value.Int64;
   }
   return defaultValue;
}


// ###### Get 32-bit unsigned integer value, or default if not found ########
uint32_t systemInfoGetUInt32(struct SystemInfo* systemInfo,
                             const char* key, const uint32_t defaultValue)
{
   const struct SystemInfoEntry* systemInfoEntry = systemInfoFindEntry(systemInfo, key);
   if( (systemInfoEntry != nullptr) && (systemInfoEntry->ValueType == SIET_UINT32) ) {
      return systemInfoEntry->Value.UInt32;
   }
   return defaultValue;
}


// ###### Get 64-bit unsigned integer value, or default if not found ########
uint64_t systemInfoGetUInt64(struct SystemInfo* systemInfo,
                             const char* key, const uint64_t defaultValue)
{
   const struct SystemInfoEntry* systemInfoEntry = systemInfoFindEntry(systemInfo, key);
   if( (systemInfoEntry != nullptr) && (systemInfoEntry->ValueType == SIET_UINT64) ) {
      return systemInfoEntry->Value.UInt64;
   }
   return defaultValue;
}


// ###### Get double value, or default if not found #########################
double systemInfoGetDouble(struct SystemInfo* systemInfo,
                           const char* key, const double defaultValue)
{
   const struct SystemInfoEntry* systemInfoEntry = systemInfoFindEntry(systemInfo, key);
   if( (systemInfoEntry != nullptr) && (systemInfoEntry->ValueType == SIET_DOUBLE) ) {
      return systemInfoEntry->Value.Double;
   }
   return defaultValue;
}


// ###### Obtain SystemInfo #################################################
struct SystemInfo* systemInfoObtain(unsigned int flags)
{
   struct SystemInfo* systemInfo =
      (struct SystemInfo*)malloc(sizeof(struct SystemInfo));
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      redBlackTreeNew(&systemInfo->Tree, systemInfoEntryPrint, systemInfoEntryComparison);
      systemInfo->Error = 0;

      systemInfoAddString(systemInfo, "name", "Test");
      systemInfoAddUInt32(systemInfo, "uint32", 32);
      systemInfoAddUInt64(systemInfo, "uint64", 64);
      systemInfoAddInt32(systemInfo,  "int32", -32);
      systemInfoAddInt64(systemInfo,  "int64", -64);
      systemInfoAddString(systemInfo, "block", "xxx");
      systemInfoAddDouble(systemInfo, "pi", 3.14159265358979323846);

      printf("INT32=%d\n", systemInfoGetInt32(systemInfo, "int32", 0xffffffff));
      printf("INT64=%lld\n", (unsigned long long)systemInfoGetInt64(systemInfo, "int64", 0xffffffff));
      printf("UINT32=%u\n", systemInfoGetUInt32(systemInfo, "uint32", 0xffffffff));
      printf("UINT64=%llu\n", (unsigned long long)systemInfoGetUInt64(systemInfo, "uint64", 0xffffffff));
      printf("PI=%1.9lf\n", systemInfoGetDouble(systemInfo, "pi", 0.0));
      printf("NA=%1.9lf\n", systemInfoGetDouble(systemInfo, "na", 0.0));

      if(systemInfo->Error) {
         systemInfoRelease(systemInfo);
         systemInfo = nullptr;
      }
   }
   return systemInfo;
}


// ###### Release SystemInfo ################################################
void systemInfoRelease(struct SystemInfo* systemInfo)
{
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      struct SystemInfoEntry* systemInfoEntry =
         (struct SystemInfoEntry*)redBlackTreeGetFirst(&systemInfo->Tree);
      while(systemInfoEntry) {
         systemInfoRemoveEntry(systemInfo, systemInfoEntry);
         systemInfoEntry = (struct SystemInfoEntry*)redBlackTreeGetFirst(&systemInfo->Tree);
      }
      redBlackTreeDelete(&systemInfo->Tree);
      free(systemInfo);
   }
}


// ###### Print SystemInfo ##################################################
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
