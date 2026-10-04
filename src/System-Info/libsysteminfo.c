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
#include <stdarg.h>
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


#define SIE_KEY_SIZE 32

struct SystemInfoEntry
{
   struct RedBlackTreeNode  Node;
   char                     Key[SIE_KEY_SIZE];
   SystemInfoEntryValue     Value;
   SystemInfoEntryValueType ValueType;
   int                      DisplayHint;
};


// Compatibility version of libsysteminfo, to allow for future changes:
// Currently, there is just version 0.
#define COMPATIBILITY_VERSION 0


struct interfaceaddress {
   const char*            ifname;
   const struct sockaddr* address;
   unsigned int           prefixlen;
   unsigned int           flags;
};


// ###### SystemInfoEntry print function ####################################
static void systemInfoEntryPrint(const void* node, FILE* fd)
{
   const struct SystemInfoEntry* systemInfoEntry = (const struct SystemInfoEntry*)node;
   switch(systemInfoEntry->ValueType) {
      case SIET_STRING:
         printf("%s=\"%s\"\n", systemInfoEntry->Key, systemInfoEntry->Value.String);
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
         if(systemInfoEntry->DisplayHint >= 0) {
            char format[32];
            snprintf((char*)&format, sizeof(format), "%%s=%%1.%dlf\n", systemInfoEntry->DisplayHint);
            printf(format, systemInfoEntry->Key, systemInfoEntry->Value.Double);
         }
         else {
            printf("%s=%lf", systemInfoEntry->Key, systemInfoEntry->Value.Double);
         }
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
      systemInfoEntry->ValueType   = SIET_INVALID;
      systemInfoEntry->DisplayHint = 0;
      if(__builtin_expect(redBlackTreeInsert(&systemInfo->Tree, &systemInfoEntry->Node) == &systemInfoEntry->Node, 1)) {
         return systemInfoEntry;
      }
      fprintf(stderr, "INTERNAL ERROR: Tried to add duplicate key \"%s\"!\n", key);
      free(systemInfoEntry);
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


// ###### Generate a key according to format string and arguments ###########
char* systemInfoMakeKey(const char* format, ...)
{
   va_list     args;
   static char key[SIE_KEY_SIZE];

   va_start(args, format);
   vsnprintf(key, sizeof(key), format, args);
   va_end(args);

   return key;
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
   if(__builtin_expect(systemInfoEntry != nullptr, 1)) {
      systemInfoEntry->Value.Int32 = value;
      systemInfoEntry->ValueType    = SIET_INT32;
   }
}


// ###### Add key with 64-bit integer value #################################
void systemInfoAddInt64(struct SystemInfo* systemInfo,
                               const char* key, const int64_t value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoAddEntry(systemInfo, key);
   if(__builtin_expect(systemInfoEntry != nullptr, 1)) {
      systemInfoEntry->Value.Int64 = value;
      systemInfoEntry->ValueType    = SIET_INT64;
   }
}


// ###### Add key with 32-bit unsigned integer value ########################
void systemInfoAddUInt32(struct SystemInfo* systemInfo,
                                const char* key, const uint32_t value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoAddEntry(systemInfo, key);
   if(__builtin_expect(systemInfoEntry != nullptr, 1)) {
      systemInfoEntry->Value.UInt32 = value;
      systemInfoEntry->ValueType    = SIET_UINT32;
   }
}


// ###### Add key with 64-bit unsigned integer value ########################
void systemInfoAddUInt64(struct SystemInfo* systemInfo,
                                const char* key, const uint64_t value)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoAddEntry(systemInfo, key);
   if(__builtin_expect(systemInfoEntry != nullptr, 1)) {
      systemInfoEntry->Value.UInt64 = value;
      systemInfoEntry->ValueType    = SIET_UINT64;
   }
}


// ###### Add key with double value #########################################
void systemInfoAddDouble(struct SystemInfo* systemInfo,
                         const char* key, const double value,
                         const int   displayHint)
{
   struct SystemInfoEntry* systemInfoEntry = systemInfoAddEntry(systemInfo, key);
   if(__builtin_expect(systemInfoEntry != nullptr, 1)) {
      systemInfoEntry->Value.Double = value;
      systemInfoEntry->ValueType    = SIET_DOUBLE;
      systemInfoEntry->DisplayHint  = displayHint;
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


// ###### Comparison function for interfaceaddress type #####################
static int compareInterfaceAddresses(const void* a, const void* b)
{
   const struct interfaceaddress* ifa1 = (const struct interfaceaddress*)a;
   const struct interfaceaddress* ifa2 = (const struct interfaceaddress*)b;
   const int cmpIfName = strcmp(ifa1->ifname, ifa2->ifname);
   if(cmpIfName < 0) {
      return -1;
   }
   else if(cmpIfName == 0) {
      if(ifa1->address->sa_family < ifa2->address->sa_family) {
         return -1;
      }
      else if(ifa1->address->sa_family == ifa2->address->sa_family) {
         if(ifa1->address->sa_family == AF_INET6) {
            const struct sockaddr_in6* ip1 = (const struct sockaddr_in6*)ifa1->address;
            const struct sockaddr_in6* ip2 = (const struct sockaddr_in6*)ifa2->address;
            return memcmp( &ip1->sin6_addr, &ip2->sin6_addr, 16 );
         }
         else if(ifa1->address->sa_family == AF_INET) {
            const struct sockaddr_in* ip1 = (const struct sockaddr_in*)ifa1->address;
            const struct sockaddr_in* ip2 = (const struct sockaddr_in*)ifa2->address;
            return memcmp( &ip1->sin_addr, &ip2->sin_addr, 4 );
         }
#if defined(__linux__)
         else if(ifa1->address->sa_family == AF_PACKET) {
            const struct sockaddr_ll* mac1 = (const struct sockaddr_ll*)ifa1->address;
            const struct sockaddr_ll* mac2 = (const struct sockaddr_ll*)ifa2->address;
            const int lengthComparison = (int)mac1->sll_halen - (int)mac2->sll_halen;
            if(lengthComparison != 0) {
               return lengthComparison;
            }
            return memcmp(mac1->sll_addr, mac2->sll_addr, mac1->sll_halen);
         }
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
         else if(ifa1->address->sa_family == AF_LINK) {
            const struct sockaddr_dl* mac1 = (const struct sockaddr_dl*)ifa1->address;
            const struct sockaddr_dl* mac2 = (const struct sockaddr_dl*)ifa2->address;
            const int lengthComparison = (int)mac1->sdl_alen - (int)mac2->sdl_alen;
            if(lengthComparison != 0) {
               return lengthComparison;
            }
            return memcmp(LLADDR(mac1), LLADDR(mac2), mac1->sdl_alen);
         }
#endif
         return 0;   // Fallback for unknown family
      }
   }
   return 1;
}


// ###### Count set bits in byte array ######################################
unsigned int countSetBits(const uint8_t* array, const unsigned int size)
{
   unsigned int count = 0;
   for(unsigned int i = 0; i < size; i++) {
      unsigned char byte = array[i];
      while(byte != 0) {
         if(byte & 1) {
            count++;
         }
         byte = byte >> 1;
      }
   }
   return count;
}


// ###### Concatenate strings, insert " " if necessary ######################
char* strdup_concat(const char* s1, const char* s2)
{
   char* result;
   if(s1 == nullptr) {
      result = strdup(s2);
   }
   else {
      const size_t len1 = strlen(s1);
      const size_t len2 = strlen(s2);
      result = (char*)malloc(len1 + len2 + 2);
      if(result != nullptr) {
         memcpy(result, s1, len1);
         result[len1] = ' ';
         memcpy(result + len1 + 1, s2, len2 + 1);   // Includes null-terminator!
      }
   }
   return result;
}


// ###### Format address ####################################################
static char* formatAddress(const struct sockaddr* address,
                           const unsigned int     prefixlen)
{
   static char buffer[128];

   buffer[0] = 0x00;
   if( (address->sa_family == AF_INET6) || (address->sa_family == AF_INET) ) {
      char resolvedHost[64];
      int error = getnameinfo(address,
                              (address->sa_family == AF_INET6) ?
                                 sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in),
                              resolvedHost, sizeof(resolvedHost),
                              nullptr, 0,
                              NI_NUMERICHOST);
      if(error != 0) {
         fprintf(stderr, "ERROR: getnameinfo() failed: %s\n", gai_strerror(error));
         exit(1);
      }
      snprintf(buffer, sizeof(buffer), "%s/%u", resolvedHost, prefixlen);
   }
#if defined(__linux__)
   else if(address->sa_family == AF_PACKET) {
      const struct sockaddr_ll* macAddress = (const struct sockaddr_ll*)address;
      int offset = 0;
      for(unsigned int i = 0; i < macAddress->sll_halen; i++) {
         if((size_t)offset < sizeof(buffer)) {
            int ret = snprintf(buffer + offset, sizeof(buffer) - offset,
                               "%s%02x", (i > 0) ? ":" : "", macAddress->sll_addr[i]);
            if(ret > 0) offset += ret;
         }
      }
   }
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__APPLE__)
   else if(address->sa_family == AF_LINK) {
      const struct sockaddr_dl* macAddress = (const struct sockaddr_dl*)address;
      const uint8_t*            lladdr     = (const uint8_t*)LLADDR(macAddress);
      int offset = 0;
      for(unsigned int i = 0; i < macAddress->sdl_alen; i++) {
         if((size_t)offset < sizeof(buffer)) {
            int ret = snprintf(buffer + offset, sizeof(buffer) - offset,
                               "%s%02x", (i > 0) ? ":" : "", lladdr[i]);
            if(ret > 0) offset += ret;
         }
      }
   }
#elif defined(__gnu_hurd__)
   // FIXME: GNU Hurd does not return link-layer (MAC) addresses in getifaddrs().
#else
#error Missing case!
#endif

   return buffer;
}


// ###### Format interface flags ############################################
static char* formatFlags(const unsigned int flags)
{
   static char buffer[256];
   int offset = snprintf(buffer, sizeof(buffer), "0x%x: <%s>", flags, (flags & IFF_UP) ? "UP" : "DOWN");
#if defined(IFF_LOWER_UP)
   if((flags & IFF_LOWER_UP) && offset >= 0 && (size_t)offset < sizeof(buffer)) {
      offset += snprintf(buffer + offset, sizeof(buffer) - offset, " <LOWER_UP>");
   }
#endif
#if defined(IFF_RUNNING)
   if((flags & IFF_RUNNING) && offset >= 0 && (size_t)offset < sizeof(buffer)) {
      offset += snprintf(buffer + offset, sizeof(buffer) - offset, " <RUNNING>");
   }
#endif
   if((flags & IFF_LOOPBACK) && offset >= 0 && (size_t)offset < sizeof(buffer)) {
      offset += snprintf(buffer + offset, sizeof(buffer) - offset, " <LOOPBACK>");
   }
   if((flags & IFF_POINTOPOINT) && offset >= 0 && (size_t)offset < sizeof(buffer)) {
      offset += snprintf(buffer + offset, sizeof(buffer) - offset, " <POINTOPOINT>");
   }
   return buffer;
}


// ###### Print hostname information ########################################
static void obtainHostnameInformation(struct SystemInfo* systemInfo)
{
   char hostname[256];
   if(gethostname(hostname, sizeof(hostname)) == 0) {
      hostname[sizeof(hostname) - 1] = 0x00;
   }
   else {
#if defined(HAVE_STRLCPY)
      strlcpy(hostname, "localhost", sizeof(hostname));
#else
      strcpy(hostname, "localhost");
#endif
   }

   systemInfoAddString(systemInfo, "hostname_long", hostname);
   char* domainname = strchr(hostname, '.');
   if(domainname != nullptr) {
      domainname[0] = 0x00;
      domainname++;
   }
   systemInfoAddString(systemInfo, "domainname", (domainname != nullptr) ? domainname : "");
   systemInfoAddString(systemInfo, "hostname_short", hostname);
}


// ###### Print kernel information ##########################################
static void obtainKernelInformation(struct SystemInfo* systemInfo)
{
   struct utsname kernelInfo;
   if(uname(&kernelInfo) >= 0) {
      systemInfoAddString(systemInfo, "system_sysname",  kernelInfo.sysname);
      systemInfoAddString(systemInfo, "system_nodename", kernelInfo.nodename);
      systemInfoAddString(systemInfo, "system_release",  kernelInfo.release);
      systemInfoAddString(systemInfo, "system_version",  kernelInfo.version);
      systemInfoAddString(systemInfo, "system_machine",  kernelInfo.machine);
   }
}


// ###### Obtain the system uptime ##########################################
static bool obtainUptime(struct timespec* ts)
{
#if defined(__linux__)
   return clock_gettime(CLOCK_BOOTTIME, ts) == 0;
#elif defined(__FreeBSD__)
   return clock_gettime(CLOCK_UPTIME_PRECISE, ts) == 0;
#elif defined(__NetBSD__)
   return clock_gettime(CLOCK_MONOTONIC, ts) == 0;
#elif defined(__OpenBSD__)
   return clock_gettime(CLOCK_BOOTTIME, ts) == 0;
#elif defined(__sun__)
   return clock_gettime(CLOCK_MONOTONIC, ts) == 0;
#elif defined(__gnu_hurd__)
   return clock_gettime(CLOCK_MONOTONIC, ts) == 0;
#elif defined(__APPLE__)
   return clock_gettime(CLOCK_MONOTONIC, ts) == 0;
#else
#error Missing case!
#endif
   // return false;
}


// ###### Print uptime information ##########################################
static void obtainUptimeInformation(struct SystemInfo* systemInfo)
{
   struct timespec ts;
   if(obtainUptime(&ts)) {
      const uint64_t     totalSecs = (uint64_t)ts.tv_sec;
      const unsigned int days      = (unsigned int)(totalSecs / 86400);
      const unsigned int hours     = (unsigned int)((totalSecs / 3600) % 24);
      const unsigned int mins      = (unsigned int)((totalSecs / 60) % 60);
      const unsigned int secs      = (unsigned int)(totalSecs % 60);
      systemInfoAddDouble(systemInfo, "uptime_total",
                          (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0), 9);
      systemInfoAddUInt32(systemInfo, "uptime_days",  days);
      systemInfoAddUInt32(systemInfo, "uptime_hours", hours);
      systemInfoAddUInt32(systemInfo, "uptime_mins",  mins);
      systemInfoAddUInt32(systemInfo, "uptime_secs",  secs);
      systemInfoAddUInt32(systemInfo, "uptime_nsecs", ts.tv_nsec);
   }
}


#if !(defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__gnu_hurd__) || defined(__APPLE__))
// ###### Query information via shell #######################################
static bool queryPipe(const char* command, char* result, size_t resultMaxSize)
{
   FILE* fh = popen(command, "r");
   if(fh != nullptr) {
      size_t resultSize = 0;
      char*  resultPtr;
      do {
         resultPtr = fgets(&result[resultSize], resultMaxSize - resultSize, fh);
         if(resultPtr == nullptr) {
            break;
         }
         resultSize += strlen(resultPtr);
      }
      while(resultSize < resultMaxSize - 1);
      result[resultSize] = 0x00;
      pclose(fh);
      return (resultSize > 0);
   }
   return false;
}
#endif


#if defined(__linux__)
// ###### Query information from file #######################################
static bool queryFile(const char* file, char* result, size_t resultMaxSize)
{
   FILE* fh = fopen(file, "r");
   if(fh != nullptr) {
      size_t resultSize = 0;
      char*  resultPtr;
      do {
         resultPtr = fgets(&result[resultSize], (int)(resultMaxSize - resultSize), fh);
         if(resultPtr == nullptr) {
            break;
         }
         resultSize += strlen(resultPtr);
      }
      while(resultSize < resultMaxSize - 1);
      result[resultSize] = 0x00;
      fclose(fh);
      return (resultSize > 0);
   }
   return false;
}
#endif


// ###### Obtain the number of processes on the system ######################
static unsigned int obtainProcessCount(void)
{
   unsigned int count = 0;

#if defined(__linux__) || defined(__sun__) || defined(__gnu_hurd__)
   // ====== Linux: count processes in /proc ================================
   struct dirent* dirEntry;
   DIR*           dir = opendir("/proc");
   if(dir != nullptr) {
      while((dirEntry = readdir(dir)) != nullptr) {
         // The name of a process directory starts with a digit:
         if(isdigit(dirEntry->d_name[0])) {
            count++;
         }
      }
      closedir(dir);
   }

#elif defined(__FreeBSD__)
   // ====== FreeBSD: use sysctl to query the number of processes ===========
   // ------  Get memory size necessary to obtain the process list ----------
   const int          mibKernProcProc[3]  = { CTL_KERN, KERN_PROC, KERN_PROC_PROC };
   const unsigned int mibKernProcProcSize = sizeof(mibKernProcProc) / sizeof(mibKernProcProc[0]);
   size_t             parameterLength = 0;
   if(sysctl(mibKernProcProc, mibKernProcProcSize, nullptr, &parameterLength, nullptr, 0) == 0) {
      parameterLength = (parameterLength * 5) / 4;   // Add some extra space
      // The memory size is more than necessary for the process list, since
      // the list may change. To obtain the process count, it is necessary
      // to actually fetch the process list:
      void* processList = malloc(parameterLength);
      if(processList != nullptr) {
         // ------ Obtain the process list ----------------------------------
         if(sysctl(mibKernProcProc, mibKernProcProcSize, processList, &parameterLength, nullptr, 0) == 0) {
            // The current process count is the number of entries fetched:
            count = parameterLength / sizeof(struct kinfo_proc);
         }
         free(processList);
      }
   }

#elif defined(__NetBSD__)
   // ====== NetBSD: use sysctl to query the number of processes ============
   int                mibKernProcProc[6]  = { CTL_KERN, KERN_PROC2, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc2), 0 };
   const unsigned int mibKernProcProcSize = sizeof(mibKernProcProc) / sizeof(mibKernProcProc[0]);
   size_t             parameterLength = 0;
   if(sysctl(mibKernProcProc, mibKernProcProcSize, nullptr, &parameterLength, nullptr, 0) == 0) {
      parameterLength = (parameterLength * 5) / 4;   // Add some extra space
      void* processList = malloc(parameterLength);
      if(processList != nullptr) {
         mibKernProcProc[5] = (int)(parameterLength / sizeof(struct kinfo_proc2));
         // ------ Obtain the process list ----------------------------------
         if(sysctl(mibKernProcProc, mibKernProcProcSize, processList, &parameterLength, nullptr, 0) == 0) {
            // The current process count is the number of entries fetched:
            count = parameterLength / sizeof(struct kinfo_proc2);
         }
         free(processList);
      }
   }

#elif defined(__OpenBSD__)
   // ====== OpenBSD: use sysctl to query the number of processes ===========
   int                mibKernProcProc[6]  = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc), 0 };
   const unsigned int mibKernProcProcSize = sizeof(mibKernProcProc) / sizeof(mibKernProcProc[0]);
   size_t             parameterLength = 0;
   if(sysctl(mibKernProcProc, mibKernProcProcSize, nullptr, &parameterLength, nullptr, 0) == 0) {
      parameterLength = (parameterLength * 5) / 4;   // Add some extra space
      void* processList = malloc(parameterLength);
      if(processList != nullptr) {
         mibKernProcProc[5] = (int)(parameterLength / sizeof(struct kinfo_proc));
         // ------ Obtain the process list ----------------------------------
         if(sysctl(mibKernProcProc, mibKernProcProcSize, processList, &parameterLength, nullptr, 0) == 0) {
            // The current process count is the number of entries fetched:
            count = parameterLength / sizeof(struct kinfo_proc);
         }
         free(processList);
      }
   }

#elif defined(__APPLE__)
   // ====== Apple: use libproc to query the number of processes ============
   // proc_listpids() with nullptr buffer returns the required buffer size:
   const int bufferSize = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
   if(bufferSize > 0) {
      count = (unsigned int)bufferSize / sizeof(pid_t);
   }

#else
#warning Using fallback solution for obtaining the process count!
   // ====== Fallback =======================================================
   char         buffer[64];
   unsigned int value;
   if( (queryPipe("ps -ae -o pid= | wc -l", buffer, sizeof(buffer))) &&
       (sscanf(buffer, "%u", &value) == 1) ) {
      count = value;
   }
#endif

    return count;
}


// ###### Obtain the number of users on the system ##########################
static unsigned int obtainUserCount(void)
{
   // Count the number of user sessions, the same as "who | wc -l":
   unsigned int count = 0;

   // ====== Use getutxent() to obtain and count the number of users ========
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__sun__) || defined(__gnu_hurd__) || defined(__APPLE__)
#if defined(ENABLE_SYSTEMD)
   char** sessions = nullptr;
   int totalSessions = sd_get_sessions(&sessions);
   if(totalSessions < 0) {
      return 0;
   }

   // Query the session class (e.g., "user", "greeter", "background"), and
   // only count interactive user logins:
   for(int i = 0; i < totalSessions; i++) {
      char* className = nullptr;
      if(sd_session_get_class(sessions[i], &className) >= 0) {
         // Equivalent to USER_PROCESS: only count interactive user logins
         if(className && strcmp(className, "user") == 0) {
            count++;
         }
         free(className);
      }
      free(sessions[i]);
   }
   free(sessions);
#else
   setutxent();
   struct utmpx* utx;
   while( (utx = getutxent()) != nullptr ) {
      if(utx->ut_type == USER_PROCESS) {
         count++;
      }
   }
   endutxent();
#endif
#elif defined(__OpenBSD__)
   // ====== OpenBSD: read /var/run/utmp to obtain the number of users ======
   FILE* fh = fopen("/var/run/utmp", "r");
   if(fh != nullptr) {
      struct utmp ut;
      while(fread(&ut, sizeof(struct utmp), 1, fh) == 1) {
         if(ut.ut_name[0] != '\0') {
            count++;
         }
      }
      fclose(fh);
   }

#else
   // ====== Fallback =======================================================
#warning Using fallback solution for obtaining the user count!
   char         buffer[64];
   unsigned int value;
   if( (queryPipe("who | wc -l", buffer, sizeof(buffer))) &&
       (sscanf(buffer, "%u", &value) == 1) ) {
      count = value;
   }
#endif

   return count;
}


// ###### Print load information ############################################
static void obtainLoadInformation(struct SystemInfo* systemInfo)
{
   // ====== Cores and page size ============================================
   const unsigned int cores = (unsigned int)sysconf(_SC_NPROCESSORS_ONLN);
   systemInfoAddUInt32(systemInfo, "system_cores", cores);
   const unsigned int pageSize = (unsigned int)sysconf(_SC_PAGESIZE);
   systemInfoAddUInt32(systemInfo, "system_pagesize", pageSize);

   // ====== Number of running processes and number of users ================
   const unsigned int processCount = obtainProcessCount();
   systemInfoAddUInt32(systemInfo, "system_procs", processCount);

   const unsigned int userCount = obtainUserCount();
   systemInfoAddUInt32(systemInfo, "system_users", userCount);

   // ====== Load averages ==================================================
#if defined(__linux__)
   struct sysinfo sysInfo;
   if(sysinfo(&sysInfo) == 0) {
      const double fFraction = 1.0 / (1 << SI_LOAD_SHIFT);
      const double fPercent  = 100.0 * fFraction / cores;   // Percent of CPU
      systemInfoAddDouble(systemInfo, "system_load_avg1min",     (double)sysInfo.loads[0] * fFraction, 6);
      systemInfoAddDouble(systemInfo, "system_load_avg5min",     (double)sysInfo.loads[1] * fFraction, 6);
      systemInfoAddDouble(systemInfo, "system_load_avg15min",    (double)sysInfo.loads[2] * fFraction, 6);
      systemInfoAddDouble(systemInfo, "system_load_avg1minpct",  (double)sysInfo.loads[0] * fPercent, 4);
      systemInfoAddDouble(systemInfo, "system_load_avg5minpct",  (double)sysInfo.loads[1] * fPercent, 4);
      systemInfoAddDouble(systemInfo, "system_load_avg15minpct", (double)sysInfo.loads[2] * fPercent, 4);
   }

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__APPLE__) || defined(__gnu_hurd__)
   double loadavg[3];
   if(getloadavg(loadavg, 3) == 3) {
      const double fPercent = 100.0 / (double)cores;
      systemInfoAddDouble(systemInfo, "system_load_avg1min",     loadavg[0], 6);
      systemInfoAddDouble(systemInfo, "system_load_avg5min",     loadavg[1], 6);
      systemInfoAddDouble(systemInfo, "system_load_avg15min",    loadavg[2], 6);
      systemInfoAddDouble(systemInfo, "system_load_avg1minpct",  loadavg[0] * fPercent, 4);
      systemInfoAddDouble(systemInfo, "system_load_avg5minpct",  loadavg[1] * fPercent, 4);
      systemInfoAddDouble(systemInfo, "system_load_avg15minpct", loadavg[2] * fPercent, 4);
   }

#else
#error Missing case!
#endif
}


#if defined(__sun__)
static bool getKstatUint64(const kstat_named_t* kn, uint64_t* val)
{
   switch(kn->data_type) {
      case KSTAT_DATA_UINT32:
         *val = (uint64_t)kn->value.ui32;
         return true;
      case KSTAT_DATA_UINT64:
         *val = kn->value.ui64;
         return true;
      case KSTAT_DATA_INT32:
         *val = (kn->value.i32 >= 0) ? (uint64_t)kn->value.i32 : 0;
         return true;
      case KSTAT_DATA_INT64:
         *val = (kn->value.i64 >= 0) ? (uint64_t)kn->value.i64 : 0;
         return true;
      default:
         return false;
   }
}
#endif


// ###### Print battery information #########################################
static void obtainBatteryInformation(struct SystemInfo* systemInfo)
{
   const unsigned int maxBatteries = 2;
   unsigned int       batteries    = 0;
   unsigned int       batteryIDs[maxBatteries];

   // ====== Linux: Obtain battery status via /sys file system ==============
#if defined(__linux__)
   for(unsigned int i = 0; i < maxBatteries; i++) {
      // ------ Obtain status of battery unit -------------------------------
      char capacityFileName[64];
      char capacityBuffer[64];
      unsigned int capacity;
      snprintf(capacityFileName, sizeof(capacityFileName), "/sys/class/power_supply/BAT%u/capacity", i);
      if( (queryFile(capacityFileName, capacityBuffer, sizeof(capacityBuffer))) &&
          (sscanf(capacityBuffer, "%u", &capacity) == 1) ) {
         char statusFileName[64];
         char statusBuffer[64];
         snprintf(statusFileName, sizeof(statusFileName), "/sys/class/power_supply/BAT%u/status", i);
         if(queryFile(statusFileName, statusBuffer, sizeof(statusBuffer))) {
            statusBuffer[strcspn(statusBuffer, "\r\n")] = 0x00;

            // ------ Extract status as status code -------------------------
            int status = 0;   // Unknown
            if(strcmp(statusBuffer, "Not charging") == 0) {
               status = 1;    // Not charging
            }
            else if(strcmp(statusBuffer, "Charging") == 0) {
               status = 2;    // Charging
            }
            else if(strcmp(statusBuffer, "Full") == 0) {
               status = 3;    // Full
            }
            else if(strcmp(statusBuffer, "Discharging") == 0) {
               status = 4;    // Discharging
            }

            // ------ Print battery status and capacity ---------------------
            systemInfoAddUInt32(systemInfo, systemInfoMakeKey("battery_%u_status",   i), status);
            systemInfoAddUInt32(systemInfo, systemInfoMakeKey("battery_%u_capacity", i), capacity);
            batteryIDs[batteries++] = i;
         }
      }
   }

   // ====== FreeBSD: Obtain battery status via ACPI device ioctls ==========
#elif defined(__FreeBSD__)
   int acpiFD = open("/dev/acpi", O_RDONLY);
   if(acpiFD >= 0) {
      unsigned int batteryUnits = 0;
      if(ioctl(acpiFD, ACPIIO_BATT_GET_UNITS, &batteryUnits) == 0) {
         for(unsigned int i = 0; i < batteryUnits; i++) {

            // ------ Obtain status of battery unit -------------------------
            union acpi_battery_ioctl_arg batteryInfo;
            memset(&batteryInfo, 0, sizeof(batteryInfo));
            batteryInfo.unit = (int)i;
            if(ioctl(acpiFD, ACPIIO_BATT_GET_BATTINFO, &batteryInfo) == 0) {

               // ------ Extract status as status code ----------------------
               unsigned int status = 0;   // Unknown
               if(batteryInfo.battinfo.state != ACPI_BATT_STAT_NOT_PRESENT) {
                  if(batteryInfo.battinfo.state == 0) {
                     status = 3;   // Full
                  }
                  else {
                     if(batteryInfo.battinfo.state & ACPI_BATT_STAT_CHARGING) {
                        status = 2;   // Charging
                     }
                     else if(batteryInfo.battinfo.state & ACPI_BATT_STAT_DISCHARG) {
                        status = 4;   // Discharging
                     }
                     else {
                        status = 1;   // Not charging
                     }
                  }
               }
               const unsigned int capacity = (unsigned int)batteryInfo.battinfo.cap;

               // ------ Print battery status and capacity ------------------
               systemInfoAddUInt32(systemInfo, systemInfoMakeKey("battery_%u_status",   i), status);
               systemInfoAddUInt32(systemInfo, systemInfoMakeKey("battery_%u_capacity", i), capacity);
               batteryIDs[batteries++] = i;
               if(batteries == maxBatteries) {
                  break;
               }
            }
         }
      }
      close(acpiFD);
   }

   // ====== NetBSD: Obtain battery status via envsys =======================
#elif defined(__NetBSD__)
   int sysmonFD = open("/dev/sysmon", O_RDONLY);
   if(sysmonFD >= 0) {
      bool                foundBattery  = false;
      bool                isPresent     = false;
      bool                isCharging    = false;
      bool                isDischarging = false;
      unsigned int        capacity      = 0;
      int                 status        = 0;   // Unknown
      envsys_basic_info_t info;
      envsys_tre_data_t   data;

      // ------ Iterate over all sensor slots -------------------------------
      for(unsigned int i = 0; i < 256; i++) {
         memset(&info, 0, sizeof(info));
         info.sensor = i;
         if(ioctl(sysmonFD, ENVSYS_GTREINFO, &info) == -1) {
            continue;
         }
         if(!(info.validflags & ENVSYS_FVALID)) {
            continue;
         }

         memset(&data, 0, sizeof(data));
         data.sensor = i;
         if(ioctl(sysmonFD, ENVSYS_GTREDATA, &data) == -1) {
            continue;
         }
         if(!(data.validflags & ENVSYS_FCURVALID)) {
            continue;
         }

         // ------ Obtain information about battery -------------------------
         if(strstr(info.desc, "acpibat0") != nullptr) {
            foundBattery = true;
            if(strstr(info.desc, "present") != nullptr) {
               if(data.cur.data_us > 0) {
                  isPresent = true;
               }
            }
            else if( (strstr(info.desc, "charge") != nullptr) &&
                     (strstr(info.desc, "rate") == nullptr) ) {
               if(data.max.data_us > 0) {
                  capacity  = (unsigned int)(((unsigned long long)data.cur.data_us * 100) / data.max.data_us);
                  isPresent = true;
               }
            }
            else if(strstr(info.desc, "charging") != nullptr) {
               if(data.cur.data_us > 0) {
                  isCharging = true;
               }
            }
            else if(strstr(info.desc, "discharge rate") != nullptr) {
               if(data.cur.data_us > 0) {
                  isDischarging = true;
               }
            }
         }
      }
      close(sysmonFD);

      if(foundBattery && isPresent) {
         if(capacity > 100) {
            capacity = 100;
         }

         // ------ Extract status as status code ----------------------------
         if(capacity == 100) {
            status = 3;   // Full
         }
         else if(isCharging) {
            status = 2;   // Charging
         }
         else if(isDischarging) {
            status = 4;   // Discharging
         }
         else {
            status = 1;   // Not charging
         }

         // ------ Print battery status and capacity ------------------------
         systemInfoAddUInt32(systemInfo, "battery_0_status",   status);
         systemInfoAddUInt32(systemInfo, "battery_0_capacity", capacity);
         batteryIDs[batteries++] = 0;
      }
   }

   // ====== OpenBSD: Obtain battery status via APM =========================
#elif defined(__OpenBSD__)
   int apmFD = open("/dev/apm", O_RDONLY);
   if(apmFD >= 0) {
      struct apm_power_info powerInfo;
      memset(&powerInfo, 0, sizeof(powerInfo));
      if(ioctl(apmFD, APM_IOC_GETPOWER, &powerInfo) == 0) {
         if( (powerInfo.battery_life <= 100) &&
             (powerInfo.battery_state != APM_BATT_UNKNOWN) &&
             (powerInfo.battery_state != APM_BATTERY_ABSENT) ) {
            const unsigned int capacity = powerInfo.battery_life;
            int status = 0;   // Unknown
            if(powerInfo.battery_state == APM_BATT_CHARGING) {
               status = 2;    // Charging
            }
            else if(capacity == 100) {
               status = 3;    // Full
            }
            else if(powerInfo.ac_state == APM_AC_ON) {
               status = 1;    // Not charging
            }
            else if(powerInfo.ac_state == APM_AC_OFF) {
               status = 4;    // Discharging
            }

            // ------ Print battery status and capacity ---------------------
            systemInfoAddUInt32(systemInfo, "battery_0_status",   status);
            systemInfoAddUInt32(systemInfo, "battery_0_capacity", capacity);
            batteryIDs[batteries++] = 0;
         }
      }
      close(apmFD);
   }

   // ====== Solaris: Obtain battery status via kstat =======================
#elif defined(__sun__)
   kstat_ctl_t* kc = kstat_open();
   if(kc != nullptr) {
      struct SolarisBatteryInfo {
         uint64_t lastCap;
         uint64_t remCap;
         uint64_t state;
         bool     foundLastCap;
         bool     foundRemCap;
         bool     foundState;
      } batteryInfo[maxBatteries];
      memset(batteryInfo, 0, sizeof(batteryInfo));

      for(kstat_t* ksp = kc->kc_chain; ksp != nullptr; ksp = ksp->ks_next) {
         if(strcmp(ksp->ks_module, "acpi_drv") == 0) {
            unsigned int batID = 0;
            bool         isBIF = false;
            bool         isBST = false;

            if(sscanf(ksp->ks_name, "battery BIF%u", &batID) == 1) {
               isBIF = true;
            }
            else if(strcmp(ksp->ks_name, "battery BIF") == 0) {
               batID = 0;
               isBIF = true;
            }
            else if(sscanf(ksp->ks_name, "battery BST%u", &batID) == 1) {
               isBST = true;
            }
            else if(strcmp(ksp->ks_name, "battery BST") == 0) {
               batID = 0;
               isBST = true;
            }

            if((isBIF || isBST) && (batID < maxBatteries)) {
               if(kstat_read(kc, ksp, nullptr) != -1) {
                  const kstat_named_t* knArray = (kstat_named_t*)ksp->ks_data;
                  for(size_t i = 0; i < ksp->ks_ndata; i++) {
                     const char* name = knArray[i].name;
                     if(isBIF && (strcmp(name, "bif_last_cap") == 0)) {
                        batteryInfo[batID].foundLastCap = getKstatUint64(&knArray[i], &batteryInfo[batID].lastCap);
                     }
                     else if(isBST && (strcmp(name, "bst_rem_cap") == 0)) {
                        batteryInfo[batID].foundRemCap = getKstatUint64(&knArray[i], &batteryInfo[batID].remCap);
                     }
                     else if(isBST && (strcmp(name, "bst_state") == 0)) {
                        batteryInfo[batID].foundState = getKstatUint64(&knArray[i], &batteryInfo[batID].state);
                     }
                  }
               }
            }
         }
      }
      kstat_close(kc);

      for(unsigned int i = 0; i < maxBatteries; i++) {
         if(batteryInfo[i].foundLastCap && batteryInfo[i].foundRemCap &&
            batteryInfo[i].foundState && (batteryInfo[i].lastCap > 0)) {

            unsigned int capacity = (unsigned int)((batteryInfo[i].remCap * 100) / batteryInfo[i].lastCap);
            if(capacity > 100) {
               capacity = 100;
            }

            // ------ Extract status as status code ------------------------
            int status = 0;   // Unknown
            if(batteryInfo[i].state & 0x02) {
               status = 2;    // Charging
            }
            else if(batteryInfo[i].state & 0x01) {
               status = 4;    // Discharging
            }
            else if(capacity == 100) {
               status = 3;    // Full
            }
            else {
               status = 1;    // Not charging
            }

            // ------ Print battery status and capacity --------------------
            systemInfoAddUInt32(systemInfo, systemInfoMakeKey("battery_%u_status",   i), status);
            systemInfoAddUInt32(systemInfo, systemInfoMakeKey("battery_%u_capacity", i), capacity);
            batteryIDs[batteries++] = i;
         }
      }
   }

   // ====== GNU Hurd =======================================================
#elif defined(__gnu_hurd__)
   // GNU Hurd does not currently provide battery interface support.

   // ====== Apple: Obtain battery status via TBD ===========================
#elif defined(__APPLE__)

#warning FIXME! Battery status for Apple

#else
#warning Missing case!
#endif

   int  offset = 0;
   char buffer[8 * maxBatteries];
   for (unsigned int i = 0; i < batteries; i++) {
      if (offset < 0 || (size_t)offset >= sizeof(buffer)) {
         break;
      }
      offset += snprintf(buffer + offset, sizeof(buffer) - offset, (i > 0) ? " %u" : "%u", batteryIDs[i]);
   }
   systemInfoAddString(systemInfo, "battery_list", buffer);
}


// ###### Print memory information ##########################################
static void obtainMemoryInformation(struct SystemInfo* systemInfo)
{
   unsigned long long memoryTotal     = 0;
   unsigned long long memoryAvailable = 0;
   unsigned long long memoryUsed      = 0;
   unsigned long long swapTotal       = 0;
   unsigned long long swapAvailable   = 0;
   unsigned long long swapUsed        = 0;

#if defined(__linux__) || defined(__gnu_hurd__)
   // ====== Query memory information via /proc =============================
   FILE* fh = fopen("/proc/meminfo", "r");
   if(fh != nullptr) {
      char line[256];
      while(fgets(line, sizeof(line) ,fh)) {
         if(strncmp(line, "Mem", 3) == 0) {
            if(sscanf(line, "MemTotal: %llu", &memoryTotal) == 1) {
               continue;
            }
#if defined(__linux__)
            else if(sscanf(line, "MemAvailable: %llu", &memoryAvailable) == 1) {
               continue;
            }
#elif defined(__gnu_hurd__)
            else if(sscanf(line, "MemFree: %llu", &memoryAvailable) == 1) {
               // GNU Hurd has no MemAvailable entry in /proc/meminfo:
               continue;
            }
#endif
         }
         if(strncmp(line, "Swap", 4) == 0) {
            if(sscanf(line, "SwapTotal: %llu", &swapTotal) == 1) {
               continue;
            }
            else if(sscanf(line, "SwapFree: %llu", &swapAvailable) == 1) {
               continue;
            }
         }
      }
      fclose(fh);
   }
   memoryTotal     *= 1024;
   memoryAvailable *= 1024;
   memoryUsed = memoryTotal - memoryAvailable;
   swapTotal       *= 1024;
   swapAvailable   *= 1024;
   swapUsed = swapTotal - swapAvailable;

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
   // ====== Query system information via sysctl ============================
   // Documentation: https://man.freebsd.org/cgi/man.cgi?query=sysctl&sektion=3

   size_t parameterLength;

   // ------ Query hw.pagesize ----------------------------------------------
   const int          mibHwPageSize[2]  = { CTL_HW, HW_PAGESIZE };
   const unsigned int mibHwPageSizeSize = sizeof(mibHwPageSize) / sizeof(mibHwPageSize[0]);
   unsigned int       pageSize;
   parameterLength = sizeof(pageSize);
   if(sysctl(mibHwPageSize, mibHwPageSizeSize, &pageSize, &parameterLength, nullptr, 0) != 0) {
      perror("sysctl(hw.pagesize)");
      return;
   }

   // ------ Query hw.physmem -----------------------------------------------
#if defined(__FreeBSD__)
   size_t             physMem;
   const int          mibHwPhysMem[2]  = { CTL_HW, HW_PHYSMEM };
#else
   uint64_t           physMem;
   const int          mibHwPhysMem[2]  = { CTL_HW, HW_PHYSMEM64 };
#endif
   const unsigned int mibHwPhysMemSize = sizeof(mibHwPhysMem) / sizeof(mibHwPhysMem[0]);
   parameterLength = sizeof(physMem);
   if(sysctl(mibHwPhysMem, mibHwPhysMemSize, &physMem, &parameterLength, nullptr, 0) != 0) {
      perror("sysctl(hw.physmem)");
      return;
   }

   // ------ Virtual mmemory ------------------------------------------------
   unsigned int vInactiveCount;
   unsigned int vFreeCount;
#if defined(__FreeBSD__)
   // ------ Query vm.stats.vm.v_inactive_count -----------------------------
   parameterLength = sizeof(vInactiveCount);
   if(sysctlbyname("vm.stats.vm.v_inactive_count", &vInactiveCount, &parameterLength, nullptr, 0) != 0) {
      perror("sysctl(vm.stats.vm.v_inactive_count)");
      return;
   }

   // ------ Query vm.stats.vm.v_free_count -----------------------------
   parameterLength = sizeof(vFreeCount);
   if(sysctlbyname("vm.stats.vm.v_free_count", &vFreeCount, &parameterLength, nullptr, 0) != 0) {
      perror("sysctl(vm.stats.vm.v_free_count)");
      return;
   }

#elif defined(__NetBSD__)
   struct uvmexp_sysctl uvm;
   const int            mibUvmExp[2]  = { CTL_VM, VM_UVMEXP2 };
   const unsigned int   mibUvmExpSize = sizeof(mibUvmExp) / sizeof(mibUvmExp[0]);
   parameterLength = sizeof(uvm);
   if(sysctl(mibUvmExp, mibUvmExpSize, &uvm, &parameterLength, nullptr, 0) != 0) {
      perror("sysctl({CTL_VM,VM_UVMEXP2})");
      return;
   }
   vInactiveCount = (unsigned int)uvm.inactive;
   vFreeCount     = (unsigned int)uvm.free;

#elif defined(__OpenBSD__)
   struct uvmexp        uvm;
   const int            mibUvmExp[2]  = { CTL_VM, VM_UVMEXP };
   const unsigned int   mibUvmExpSize = sizeof(mibUvmExp) / sizeof(mibUvmExp[0]);
   parameterLength = sizeof(uvm);
   if(sysctl(mibUvmExp, mibUvmExpSize, &uvm, &parameterLength, nullptr, 0) != 0) {
      perror("sysctl({CTL_VM,VM_UVMEXP})");
      return;
   }
   vInactiveCount = (unsigned int)uvm.inactive;
   vFreeCount     = (unsigned int)uvm.free;
#endif

   // ------ Calculations ---------------------------------------------------
   const unsigned long long vmstatInactive = (unsigned long long)vInactiveCount * pageSize;
   const unsigned long long vmstatFree     = (unsigned long long)vFreeCount *  pageSize;

   memoryTotal     = physMem;
   memoryAvailable = vmstatInactive + vmstatFree;
   if(memoryTotal >= memoryAvailable) {
      memoryUsed = memoryTotal - memoryAvailable;
   }
   else {
      // The sysctl calls are asynchronous. Protect against underflows:
      memoryAvailable = memoryTotal;
      memoryUsed      = 0;
   }

#elif defined(__sun__)
   // ====== Query physical memory via sysconf ==============================
   const long pageSize = sysconf(_SC_PAGESIZE);
   if (pageSize > 0) {
      const long physPages   = sysconf(_SC_PHYS_PAGES);
      const long avphysPages = sysconf(_SC_AVPHYS_PAGES);

      if(physPages >= 0) {
         memoryTotal = (unsigned long long)physPages * (unsigned long)pageSize;
      }
      if(avphysPages >= 0) {
         memoryAvailable = (unsigned long long)avphysPages * (unsigned long)pageSize;
      }
      if(memoryTotal >= memoryAvailable) {
         memoryUsed = memoryTotal - memoryAvailable;
      }
   }
#endif

   // ------ Get information about swap -------------------------------------
#if defined(__FreeBSD__)
   // Based on: https://cgit.freebsd.org/src/tree/sbin/swapon/swapon.c
   int    mib[16];
   size_t mibsize = 16;
   if(sysctlnametomib("vm.swap_info", mib, &mibsize) == 0) {
      for(int n = 0; ; n++) {
         mib[mibsize] = n;
         struct xswdev xsw;
         size_t        size  = sizeof(xsw);
         if(sysctl(mib, mibsize + 1, &xsw, &size, nullptr, 0) == -1) {
            break;
         }
         if(xsw.xsw_version != XSWDEV_VERSION) {
            break;
         }
         const unsigned long long total      = (unsigned long long)xsw.xsw_nblks * pageSize;
         const unsigned long long used       = (unsigned long long)xsw.xsw_used * pageSize;
         const unsigned long long available  = total - used;
         swapTotal      += total;
         swapAvailable  += available;
         swapUsed       += used;
      }
   }

#elif defined(__NetBSD__) || defined(__OpenBSD__)
   const int numberOfSwapDevices = swapctl(SWAP_NSWAP, nullptr, 0);
   if(numberOfSwapDevices > 0) {
      struct swapent* swapDeviceArray = malloc((unsigned int)numberOfSwapDevices * sizeof(struct swapent));
      if(swapDeviceArray) {
         int swapRecords = swapctl(SWAP_STATS, swapDeviceArray, numberOfSwapDevices);
         if(swapRecords > 0) {
            for (unsigned int i = 0; i < (unsigned int)swapRecords; i++) {
               if(swapDeviceArray[i].se_flags & SWF_INUSE) {
                  const unsigned long long totalBytes =
                     (unsigned long long)swapDeviceArray[i].se_nblks * DEV_BSIZE;
                  const unsigned long long usedBytes =
                     (unsigned long long)swapDeviceArray[i].se_inuse * DEV_BSIZE;
                  swapTotal     += totalBytes;
                  swapUsed      += usedBytes;
                  swapAvailable += (totalBytes - usedBytes);
               }
            }
         }
         free(swapDeviceArray);
      }
   }

#elif defined(__sun__)
   // ====== Query physical swap space via swapctl(SC_LIST) =================
   if(pageSize > 0) {
      const int numSwap = swapctl(SC_GETNSWP, nullptr);
      if(numSwap > 0) {
         struct swaptable* swapTable = (struct swaptable*)malloc(sizeof(int) + (size_t)numSwap * sizeof(struct swapent));
         if(swapTable != nullptr) {
            char pathBuffer[numSwap][MAXPATHLEN];
            swapTable->swt_n = numSwap;
            for(int i = 0; i < numSwap; i++) {
               swapTable->swt_ent[i].ste_path = pathBuffer[i];
            }
            const int returned = swapctl(SC_LIST, swapTable);
            if(returned > 0) {
               for(int i = 0; i < returned; i++) {
                  if(!(swapTable->swt_ent[i].ste_flags & ST_INDEL)) {
                     swapTotal     += (unsigned long long)swapTable->swt_ent[i].ste_pages * (unsigned long)pageSize;
                     swapAvailable += (unsigned long long)swapTable->swt_ent[i].ste_free  * (unsigned long)pageSize;
                  }
               }
               if(swapTotal >= swapAvailable) {
                  swapUsed = swapTotal - swapAvailable;
               }
            }
            free(swapTable);
         }
      }
   }

#elif defined(__APPLE__)
   // ====== Query system information via sysctl ============================

   // ------ Query hw.memsize -----------------------------------------------
   int64_t memSize;
   size_t len = sizeof(memSize);
   if(sysctlbyname("hw.memsize", &memSize, &len, nullptr, 0) == 0) {
      memoryTotal = (unsigned long long)memSize;
   }

   // ------ Query memory statistics ----------------------------------------
   mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
   vm_statistics64_data_t memoryStatistics;
   vm_size_t              pageSize;
   host_page_size(mach_host_self(), &pageSize);
   if(host_statistics64(mach_host_self(), HOST_VM_INFO64,
                        (host_info64_t)&memoryStatistics, &count) == KERN_SUCCESS) {
      const unsigned long long free =
         (unsigned long long)memoryStatistics.free_count * pageSize;
      const unsigned long long inactive =
         (unsigned long long)memoryStatistics.inactive_count * pageSize;
      const unsigned long long speculative =
         (unsigned long long)memoryStatistics.speculative_count * pageSize;

      memoryAvailable = free + inactive + speculative;
      memoryUsed      = memoryTotal - memoryAvailable;
   }

   // ------ Query swap usage -----------------------------------------------
   struct xsw_usage swapUsageStatistics;
   size_t swapUsageStatisticsSize = sizeof(swapUsageStatistics);
   if(sysctlbyname("vm.swapusage", &swapUsageStatistics, &swapUsageStatisticsSize, nullptr, 0) == 0) {
      swapTotal     = swapUsageStatistics.xsu_total;
      swapUsed      = swapUsageStatistics.xsu_used;
      swapAvailable = swapUsageStatistics.xsu_avail;
   }
#endif

   systemInfoAddUInt64(systemInfo, "system_mem_total", memoryTotal);
   systemInfoAddUInt64(systemInfo, "system_mem_used",  memoryUsed);
   systemInfoAddUInt64(systemInfo, "system_mem_free",  memoryAvailable);
   if(memoryTotal > 0) {
      systemInfoAddDouble(systemInfo, "system_mem_usedpct", 100.0 * memoryUsed / memoryTotal, 1);
      systemInfoAddDouble(systemInfo, "system_mem_freepct", 100.0 * memoryAvailable / memoryTotal, 1);
   }

   if(swapTotal > 0) {
      systemInfoAddUInt64(systemInfo, "system_swap_total", swapTotal);
      systemInfoAddUInt64(systemInfo, "system_swap_used",  swapUsed);
      systemInfoAddUInt64(systemInfo, "system_swap_free",  swapAvailable);
      systemInfoAddDouble(systemInfo, "system_swap_usedpct", 100.0 * swapUsed / swapTotal, 1);
      systemInfoAddDouble(systemInfo, "system_swap_freepct", 100.0 * swapAvailable / swapTotal, 1);
   }
   else {
      systemInfoAddUInt64(systemInfo, "system_swap_total", 0);
   }
}


// ###### Obtain disk usage statistics ######################################
static bool obtainDiskUsage(struct SystemInfo* systemInfo,
                            const char* mountPoint, const char* label)
{
   // ====== Obtain file system status of mount point =======================
   struct statvfs status;
   if(statvfs(mountPoint, &status) != 0) {
      return false;
   }

   // ====== Compute the size statistics ====================================
   // Computations according to "df" command's output:
   const unsigned long long totalSpace =
      (unsigned long long)status.f_blocks * (unsigned long long)status.f_frsize;
   const unsigned long long availableSpace =   /* available for user */
      (unsigned long long)status.f_bavail * (unsigned long long)status.f_frsize;
   const unsigned long long freeSpace =   /* actual free space */
      (unsigned long long)status.f_bfree * (unsigned long long)status.f_frsize;
   const unsigned long long usedSpace = totalSpace - freeSpace;
   const double usagePercentage = (totalSpace > 0) ?
      (100.0 * (double)(totalSpace - availableSpace) / (double)totalSpace) : 0.0;

   // ====== Print the results ==============================================
   systemInfoAddUInt64(systemInfo, systemInfoMakeKey("disk_%s_total", label), totalSpace);
   systemInfoAddUInt64(systemInfo, systemInfoMakeKey("disk_%s_used",  label), usedSpace);
   systemInfoAddUInt64(systemInfo, systemInfoMakeKey("disk_%s_available", label), availableSpace);
   systemInfoAddDouble(systemInfo, systemInfoMakeKey("disk_%s_pct", label), usagePercentage, 3);
   return true;
}


// ###### Print disk information ############################################
static void obtainDiskInformation(struct SystemInfo* systemInfo)
{
   obtainDiskUsage(systemInfo, "/",     "root");
   obtainDiskUsage(systemInfo, "/home", "home");
   obtainDiskUsage(systemInfo, "/tmp",  "tmp");
   // obtainDiskUsage(systemInfo, "/boot/efi", "efi");
   systemInfoAddString(systemInfo, "disk_list", "root home tmp");
}


// ###### Print network information #########################################
static void obtainNetworkInformation(struct SystemInfo* systemInfo,
                                     const bool         filterLocalScope)
{
   // ====== Query available interfaces and their addresses =================
   struct ifaddrs* ifaddr;
   if(getifaddrs(&ifaddr) == -1) {
      perror("getifaddrs()");
      return;
   }

   // ====== Build list of interfaces and their addresses ===================
   struct interfaceaddress ifaArray[1024];
   unsigned int            ifIndices[1024];
   unsigned int            n = 0;
   for(struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
      if(ifa->ifa_addr != nullptr) {
         if(n >= (sizeof(ifaArray) / sizeof(ifaArray[0]))) {
            fprintf(stderr, "WARNING: Truncated list of interface addresses!\n");
            break;
         }

         // ====== IP address ===============================================
         switch(ifa->ifa_addr->sa_family) {
            case AF_INET:
            case AF_INET6:
               if(ifa->ifa_addr->sa_family == AF_INET6) {

                  if( filterLocalScope &&
                      ( (IN6_IS_ADDR_LOOPBACK(&((const struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr)) ||
                        (IN6_IS_ADDR_LINKLOCAL(&((const struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr)) ) ) {
                     continue;
                  }
                  if(__builtin_expect(ifa->ifa_netmask != nullptr, 1)) {
                     const uint8_t* netmask =
                        (const uint8_t*)&((const struct sockaddr_in6*)ifa->ifa_netmask)->sin6_addr.s6_addr;
                     ifaArray[n].prefixlen = countSetBits(netmask, 16);
                  }
                  else {
                     ifaArray[n].prefixlen = 128;
                  }
               }
               else {
                  if( filterLocalScope &&
                      (ntohl( ((const struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr) == INADDR_LOOPBACK) ) {
                     continue;
                  }
                  if(__builtin_expect(ifa->ifa_netmask != nullptr, 1)) {
                     const uint8_t* netmask =
                        (const uint8_t*)&((const struct sockaddr_in*)ifa->ifa_netmask)->sin_addr;
                     ifaArray[n].prefixlen = countSetBits(netmask, 4);
                  }
                  else {
                     ifaArray[n].prefixlen = 32;
                  }
               }
            ifaArray[n].ifname  = ifa->ifa_name;
            ifaArray[n].address = ifa->ifa_addr;
            ifaArray[n].flags   = ifa->ifa_flags;
            n++;
            break;

         // ====== MAC address ==============================================
#if defined(__gnu_hurd__)
         // GNU Hurd does not return link-layer addresses in getifaddrs().
#else
#if defined(__linux__)
         case AF_PACKET:
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__APPLE__)
         case AF_LINK:
#else
#error Missing case!
#endif
            ifaArray[n].prefixlen = 0;   // Just to be sure ...
            ifaArray[n].ifname    = ifa->ifa_name;
            ifaArray[n].address   = ifa->ifa_addr;
            ifaArray[n].flags     = ifa->ifa_flags;
            n++;
            break;
#endif
         }
      }
   }
   qsort(ifaArray, n, sizeof(struct interfaceaddress),
         compareInterfaceAddresses);

   // ====== Print interfaces and their addresses ===========================
   unsigned int lastIfIndex = 0;
   int          lastFamily  = AF_UNSPEC;
   char*        addressList = nullptr;
   char         addressKey[SIE_KEY_SIZE];
   for(unsigned int i = 0; i < n; i++) {
      ifIndices[i] = if_nametoindex(ifaArray[i].ifname);
      if(ifIndices[i] != 0) {
         if( (lastIfIndex == 0) || (lastIfIndex != ifIndices[i]) ) {
            systemInfoAddString(systemInfo, systemInfoMakeKey("netif_%u_name", ifIndices[i]), ifaArray[i].ifname);
            systemInfoAddString(systemInfo, systemInfoMakeKey("netif_%u_flags", ifIndices[i]), formatFlags(ifaArray[i].flags));
            lastFamily = AF_UNSPEC;
            if(addressList) {
               systemInfoAddString(systemInfo, addressKey, addressList);
               free(addressList);
               addressList = nullptr;
            }

#if defined(__gnu_hurd__)
            // GNU Hurd has to query link-layer addresses via ioctl:
            int sd = socket(AF_INET, SOCK_DGRAM, 0);
            if(sd >= 0) {
               struct ifreq ifr;
               memset(&ifr, 0, sizeof(ifr));
               strncpy(ifr.ifr_name, ifaArray[i].ifname, IFNAMSIZ - 1);
               if(ioctl(sd, SIOCGIFHWADDR, &ifr) == 0) {
                  if(ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
                     printf("netif_%u_mac=\"", ifIndices[i]);
                     const uint8_t*     macAddress       = (const uint8_t*)ifr.ifr_hwaddr.sa_data;
                     const unsigned int macAddressLength = 6;
                     for(unsigned int i = 0; i < macAddressLength; i++) {
                        printf("%s%02x", (i > 0) ? ":" : "", macAddress[i]);
                     }
                     puts("\"");
                  }
               }
               close(sd);
            }
#endif
         }

         if(lastFamily != ifaArray[i].address->sa_family) {
            if(lastFamily != AF_UNSPEC) {
               if(addressList) {
                  systemInfoAddString(systemInfo, addressKey, addressList);
                  free(addressList);
                  addressList = nullptr;
               }
            }
            switch(ifaArray[i].address->sa_family) {
               case AF_INET6:
                  strlcpy(addressKey, systemInfoMakeKey("netif_%u_ipv6", ifIndices[i]), sizeof(addressKey));
                  break;
               case AF_INET:
                  strlcpy(addressKey, systemInfoMakeKey("netif_%u_ipv4", ifIndices[i]), sizeof(addressKey));
                  break;
#if defined(__gnu_hurd__)
               // GNU Hurd does not return link-layer addresses in getifaddrs().
#else
#if defined(__linux__)
               case AF_PACKET:
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__APPLE__)
               case AF_LINK:
#else
#error Missing case!
#endif
#endif
                  strlcpy(addressKey, systemInfoMakeKey("netif_%u_mac", ifIndices[i]), sizeof(addressKey));
                  break;
               default:
                  break;
            }
         }
         const char* address = formatAddress(ifaArray[i].address, ifaArray[i].prefixlen);
         addressList = strdup_concat(addressList, address);

         lastIfIndex = ifIndices[i];
         lastFamily  = ifaArray[i].address->sa_family;
      }
   }
   if(addressList) {
      systemInfoAddString(systemInfo, addressKey, addressList);
      free(addressList);
      addressList = nullptr;
   }

   // ====== Print interfaces list ==========================================
   lastIfIndex = 0;

   int  offset = 0;
   char buffer[8 * n];
   for(unsigned int i = 0; i < n; i++) {
      if(ifIndices[i] != 0) {
         if( (lastIfIndex == 0) || (lastIfIndex != ifIndices[i]) ) {
            if (offset < 0 || (size_t)offset >= sizeof(buffer)) {
               break;
            }
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, ((i > 0) || (lastIfIndex != 0)) ? " %u" : "%u", ifIndices[i]);
         }
         lastIfIndex = ifIndices[i];
      }
   }
   systemInfoAddString(systemInfo, "netif_list", buffer);

   freeifaddrs(ifaddr);
}


// ###### Obtain SystemInfo #################################################
struct SystemInfo* systemInfoObtain(const unsigned int compatibilityVersion,
                                    const unsigned int flags)
{
   struct SystemInfo* systemInfo =
      (struct SystemInfo*)malloc(sizeof(struct SystemInfo));
   if(__builtin_expect(systemInfo != nullptr, 1)) {
      redBlackTreeNew(&systemInfo->Tree, systemInfoEntryPrint, systemInfoEntryComparison);
      systemInfo->Error = 0;

      systemInfoAddUInt32(systemInfo, "compatibility", compatibilityVersion);
      obtainHostnameInformation(systemInfo);
      obtainUptimeInformation(systemInfo);
      obtainKernelInformation(systemInfo);
      obtainLoadInformation(systemInfo);
      obtainBatteryInformation(systemInfo);
      obtainMemoryInformation(systemInfo);
      obtainDiskInformation(systemInfo);
      obtainNetworkInformation(systemInfo, true);


      // systemInfoAddString(systemInfo, "name", "Test");
      // systemInfoAddUInt32(systemInfo, "uint32", 32);
      // systemInfoAddUInt64(systemInfo, "uint64", 64);
      // systemInfoAddInt32(systemInfo,  "int32", -32);
      // systemInfoAddInt64(systemInfo,  "int64", -64);
      // systemInfoAddString(systemInfo, "block", "xxx");
      // systemInfoAddDouble(systemInfo, "pi", 3.14159265358979323846, -1);
      //
      // printf("INT32=%d\n", systemInfoGetInt32(systemInfo, "int32", 0xffffffff));
      // printf("INT64=%lld\n", (unsigned long long)systemInfoGetInt64(systemInfo, "int64", 0xffffffff));
      // printf("UINT32=%u\n", systemInfoGetUInt32(systemInfo, "uint32", 0xffffffff));
      // printf("UINT64=%llu\n", (unsigned long long)systemInfoGetUInt64(systemInfo, "uint64", 0xffffffff));
      // printf("PI=%1.9lf\n", systemInfoGetDouble(systemInfo, "pi", 0.0));
      // printf("NA=%1.9lf\n", systemInfoGetDouble(systemInfo, "na", 0.0));

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
   // ====== Initialise locale support ======================================
   if(setlocale(LC_ALL, "") == nullptr) {
      setlocale(LC_ALL, "C.UTF-8");   // "C" should exist on all systems!
   }

   struct SystemInfo* systemInfo = systemInfoObtain(COMPATIBILITY_VERSION, 0xffffffff);
   if(systemInfo) {
      systemInfoPrint(systemInfo, stdout);
      systemInfoRelease(systemInfo);
   }
}
