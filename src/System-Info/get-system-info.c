// ==========================================================================
//         ____            _                     _____           _
//        / ___| _   _ ___| |_ ___ _ __ ___     |_   _|__   ___ | |___
//        \___ \| | | / __| __/ _ \ '_ ` _ \ _____| |/ _ \ / _ \| / __|
//         ___) | |_| \__ \ ||  __/ | | | | |_____| | (_) | (_) | \__ \
//        |____/ \__, |___/\__\___|_| |_| |_|     |_|\___/ \___/|_|___/
//               |___/
//                             --- System-Tools ---
//                  https://www.nntb.no/~dreibh/system-tools/
// ==========================================================================
//
// Get-System-Info
// Copyright (C) 2024-2025 by Thomas Dreibholz
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

#include <getopt.h>
#include <ifaddrs.h>
#include <libintl.h>
#include <locale.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>
#if defined(__linux)
#include <sys/sysinfo.h>
#include <netpacket/packet.h>
#elif defined(__FreeBSD__)
#include <net/if_dl.h>
#include <sys/sysctl.h>
#include <vm/vm_param.h>
#else
#error Unknown system! The system-specific code parts need an update!
#endif
#ifndef nullptr
#define nullptr NULL
#endif

// FIXME: For some reasons, IFF_LOWER_UP seems to be undefined here.
#if defined(__linux)
#define IFF_LOWER_UP (1 << 16)
#elif defined(__FreeBSD__)
#include <netlink/route/interface.h>
#endif

#include "package-version.h"


struct interfaceaddress {
   const char*            ifname;
   const struct sockaddr* address;
   unsigned int           prefixlen;
   u_int                  flags;
};


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
         const int cmpAddress =
            (ifa1->address->sa_family == AF_INET6) ?
               memcmp( (const void*)&((const struct sockaddr_in6*)ifa1->address)->sin6_addr,
                       (const void*)&((const struct sockaddr_in6*)ifa2->address)->sin6_addr,
                       16 ) :
               memcmp( (const void*)&((const struct sockaddr_in*)ifa1->address)->sin_addr,
                       (const void*)&((const struct sockaddr_in*)ifa2->address)->sin_addr,
                       4 );
         return cmpAddress;
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


// ###### Print address #####################################################
static void printaddress(const struct sockaddr* address,
                         const unsigned int     prefixlen)
{
   if( (address->sa_family == AF_INET6) || (address->sa_family == AF_INET) ) {
      char resolvedHost[NI_MAXHOST];
      int error = getnameinfo(address,
                              (address->sa_family == AF_INET6) ?
                                 sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in),
                              (char*)&resolvedHost, sizeof(resolvedHost),
                              nullptr, 0,
                              NI_NUMERICHOST);
      if(error != 0) {
         fprintf(stderr, "ERROR: getnameinfo() failed: %s\n", gai_strerror(error));
         exit(1);
      }
      printf("%s/%u", resolvedHost, prefixlen);
   }
#if defined(__linux)
   else if(address->sa_family == AF_PACKET) {
      const struct sockaddr_ll* macAddress = (const struct sockaddr_ll*)address;
      for(unsigned int i = 0; i < 6; i++) {
         printf("%s%02x", (i > 0) ? ":" : "", macAddress->sll_addr[i]);
      }
   }
#elif defined(__FreeBSD__)
   else if(address->sa_family == AF_LINK) {
      const uint8_t* macAddress = (unsigned char *)LLADDR((const struct sockaddr_dl*)address);
      for(unsigned int i = 0; i < 6; i++) {
         printf("%s%02x", (i > 0) ? ":" : "", macAddress[i]);
      }
   }
#endif
}


// ###### Print interface flags #############################################
static void printflags(const u_int flags)
{
   printf("0x%x: <%s>", flags, (flags & IFF_UP) ? "UP" : "DOWN");
   if(flags & IFF_LOWER_UP) {
      fputs(" <LOWER_UP>", stdout);
   }
   if(flags & IFF_LOOPBACK) {
      fputs(" <LOOPBACK>", stdout);
   }
   if(flags & IFF_POINTOPOINT) {
      fputs(" <POINTOPOINT>", stdout);
   }
}


// ###### Print hostname information ########################################
static void showHostnameInformation()
{
   char hostname[256];
   if(gethostname((char*)&hostname, sizeof(hostname)) != 0) {
      strcpy((char*)&hostname, "localhost");
   }

   char* domainname = strchr(hostname, '.');
   if(domainname != nullptr) {
      domainname[0] = 0x00;
      domainname++;
      printf("hostname_long=\"%s.%s\"\n", hostname, domainname);
   }
   else {
       printf("hostname_long=\"%s\"\n", hostname);
   }
   printf("hostname_short=\"%s\"\n", hostname);
}


// ###### Print kernel information ##########################################
static void showKernelInformation()
{
   struct utsname kernelInfo;
   if(uname(&kernelInfo) == 0) {
      printf("system_sysname=\"%s\"\n",  kernelInfo.sysname);
      printf("system_nodename=\"%s\"\n", kernelInfo.nodename);
      printf("system_release=\"%s\"\n",  kernelInfo.release);
      printf("system_version=\"%s\"\n",  kernelInfo.version);
      printf("system_machine=\"%s\"\n",  kernelInfo.machine);
   }
}


// ###### Print uptime information ##########################################
static void showUptimeInformation()
{
   struct timespec ts;
   if(clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
      const unsigned int days  = ts.tv_sec / 86400;
      const unsigned int hours = (ts.tv_sec / 3600) - (days * 24);
      const unsigned int mins  = (ts.tv_sec / 60) - (days * 1440) - (hours * 60);
      const unsigned int secs  = ts.tv_sec - (days * 86400) - (hours * 3600) - (mins * 60);
      printf("uptime_total=%1.9f\n",
             (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0));
      printf("uptime_days=%u\n",   days);
      printf("uptime_hours=%u\n",  hours);
      printf("uptime_mins=%u\n",   mins);
      printf("uptime_secs=%u\n",   secs);
      printf("uptime_nsecs=%lu\n", ts.tv_nsec);
   }
}


// ###### Query information via shell #######################################
static bool queryPipe(const char* command, char* result, size_t resultMaxSize)
{
   FILE* fh = popen(command, "r");
   if(fh != nullptr) {
      size_t resultSize = 0;
      char*  r;
      do {
         r = fgets((char*)&result[resultSize], resultMaxSize - 1 - resultSize, fh);
         if(r == nullptr) {
            break;
         }
         resultSize += strlen(r);
      }
      while(resultSize < resultMaxSize- 1);
      result[resultSize] = 0x00;
      pclose(fh);
      return true;
   }
   return false;
}


// ###### Query information from file #######################################
static bool queryFile(const char* file, char* result, size_t resultMaxSize)
{
   FILE* fh = fopen(file, "r");
   if(fh != nullptr) {
      size_t resultSize = 0;
      char*  r;
      do {
         r = fgets((char*)&result[resultSize], resultMaxSize - resultSize, fh);
         if(r == nullptr) {
            break;
         }
         resultSize += strlen(r);
      }
      while(resultSize < resultMaxSize);
      fclose(fh);
      return true;
   }
   return false;
}


// ###### Print load information ############################################
static void showLoadInformation()
{
   // ====== Cores and page size ============================================
   const unsigned int cores = sysconf(_SC_NPROCESSORS_ONLN);
   printf("system_cores=%u\n", cores);
   const unsigned int pageSize = sysconf(_SC_PAGESIZE);
   printf("system_pagesize=%u\n", pageSize);

   // ====== Number of running processes ====================================
   char         buffer[64];
   unsigned int value;
   if( (queryPipe("ps -aex -o pid= | wc -l", (char*)&buffer, sizeof(buffer))) &&
       (sscanf(buffer, "%u", &value) == 1) ) {
      printf("system_procs=%u\n", value);
   }
   else {
      printf("system_procs=0");
   }

   // ====== Number of users ================================================
   if( (queryPipe("who | cut -d' ' -f1 | sort -ud | wc -l", (char*)&buffer, sizeof(buffer))) &&
       (sscanf(buffer, "%u", &value) == 1) ) {
      printf("system_users=%u\n", value);
   }
   else {
      printf("system_users=0");
   }

   // ====== Load averages ==================================================
#if defined(__linux)
   struct sysinfo systemInfo;
   if(sysinfo(&systemInfo) == 0) {
      const double fFraction = 1.0 / (1 << SI_LOAD_SHIFT);
      const double fPercent  = 100.0 * fFraction / cores;   // Percent of CPU
      printf("system_load_avg1min=%1.6f\n",     (double)systemInfo.loads[0] * fFraction);
      printf("system_load_avg5min=%1.6f\n",     (double)systemInfo.loads[1] * fFraction);
      printf("system_load_avg15min=%1.6f\n",    (double)systemInfo.loads[2] * fFraction);
      printf("system_load_avg1minpct=%1.4f\n",  (double)systemInfo.loads[0] * fPercent);
      printf("system_load_avg5minpct=%1.4f\n",  (double)systemInfo.loads[1] * fPercent);
      printf("system_load_avg15minpct=%1.4f\n", (double)systemInfo.loads[2] * fPercent);
   }

#elif defined(__FreeBSD__)
   double loadavg[3];
   if(getloadavg(loadavg, 3) == 3) {
      printf("system_load_avg1min=%1.6f\n",  loadavg[0]);
      printf("system_load_avg5min=%1.6f\n",  loadavg[1]);
      printf("system_load_avg15min=%1.6f\n", loadavg[2]);
      printf("system_load_avg1minpct=%1.6f\n",  100.0 * loadavg[0]);
      printf("system_load_avg5minpct=%1.6f\n",  100.0 * loadavg[1]);
      printf("system_load_avg15minpct=%1.6f\n", 100.0 * loadavg[2]);
   }
#endif
}


// ###### Print battery information #########################################
static void showBatteryInformation()
{
   const unsigned int maxBatteries = 2;
   unsigned int       batteries    = 0;
   unsigned int       batteryIDs[maxBatteries];

#if defined(__linux)
   for(unsigned int i = 0; i < maxBatteries; i++) {
      char capacityFileName[64];
      char capacityBuffer[64];
      unsigned int capacity;
      snprintf((char*)&capacityFileName, sizeof(capacityFileName), "/sys/class/power_supply/BAT%u/capacity", i);
      if( (queryFile(capacityFileName, (char*)&capacityBuffer, sizeof(capacityBuffer))) &&
          (sscanf(capacityBuffer, "%u", &capacity) == 1) ) {
         char  statusFileName[64];
         char  statusBuffer[64];
         char* statusEnd;
         snprintf((char*)&statusFileName, sizeof(statusFileName), "/sys/class/power_supply/BAT%u/status", i);
         if( (queryFile(statusFileName, (char*)&statusBuffer, sizeof(statusBuffer))) &&
             (statusEnd = index(statusBuffer, '\n')) ) {
            *statusEnd = 0x00;
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
            printf("battery_%u_status=%u\n",   i, status);
            printf("battery_%u_capacity=%u\n", i, capacity);
            batteryIDs[batteries++] = i;
         }
      }
   }

#elif defined(__FreeBSD__)
   char batteryInfoQuery[64];
   char batteryInfo[4096];
   for(unsigned int i = 0; i < maxBatteries; i++) {
      snprintf((char*)&batteryInfoQuery, sizeof(batteryInfoQuery),
               "acpiconf -i%u 2>/dev/null", i);
      if( queryPipe(batteryInfoQuery, (char*)&batteryInfo, sizeof(batteryInfo)) ) {
         const char* r = (const char*)&batteryInfo;
         do {
            printf("x");
            r = index(r, '\n');
         } while(r++ != nullptr);
         batteryIDs[batteries++] = i;
      }
   }
#endif

   fputs("battery_list=\"", stdout);
   for(unsigned int i = 0; i < batteries; i++) {
      printf((i > 0) ? " %u" : "%u", batteryIDs[i]);
   }
   puts("\"");
}


// ###### Print memory information ##########################################
static void showMemoryInformation()
{
   unsigned long long memoryTotal     = 0;
   unsigned long long memoryAvailable = 0;
   unsigned long long memoryUsed      = 0;
   unsigned long long swapTotal       = 0;
   unsigned long long swapAvailable   = 0;
   unsigned long long swapUsed        = 0;

#if defined(__linux)
   // ====== Query memory information via /proc =============================
   FILE* fh = fopen("/proc/meminfo", "r");
   if(fh != nullptr) {
      char line[256];
      while(fgets(line, sizeof(line) ,fh)) {
         if(strncmp(line, "Mem", 3) == 0) {
            if(sscanf(line, "MemTotal: %llu", &memoryTotal) == 1) {
               continue;
            }
            else if(sscanf(line, "MemAvailable: %llu", &memoryAvailable) == 1) {
               continue;
            }
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

#elif defined(__FreeBSD__)
   // ====== Query system information via sysctl ============================
   // Documentation: https://man.freebsd.org/cgi/man.cgi?query=sysctl&sektion=3

   // ------ Query hw.pagesize ----------------------------------------------
   unsigned int pageSize;
   size_t len = sizeof(pageSize);
   if(sysctlbyname("hw.pagesize", &pageSize, &len, nullptr, 0) != 0) {
      perror("sysctl(hw.pagesize)");
      return;
   }

   // ------ Query hw.physmem -----------------------------------------------
   unsigned long physMem;
   len = sizeof(physMem);
   if(sysctlbyname("hw.physmem", &physMem, &len, nullptr, 0) != 0) {
      perror("sysctl(hw.physmem)");
      return;
   }

   // ------ Query vm.stats.vm.v_inactive_count -----------------------------
   unsigned int vInactiveCount;
   len = sizeof(vInactiveCount);
   if(sysctlbyname("vm.stats.vm.v_inactive_count", &vInactiveCount, &len, nullptr, 0) != 0) {
      perror("sysctl(vm.stats.vm.v_inactive_count)");
      return;
   }

   // ------ Query vm.stats.vm.v_cache_count -----------------------------
   unsigned int vCacheCount;
   len = sizeof(vCacheCount);
   if(sysctlbyname("vm.stats.vm.v_cache_count", &vCacheCount, &len, nullptr, 0) != 0) {
      perror("sysctl(vm.stats.vm.v_cache_count)");
      return;
   }

   // ------ Query vm.stats.vm.v_free_count -----------------------------
   unsigned int vFreeCount;
   len = sizeof(vFreeCount);
   if(sysctlbyname("vm.stats.vm.v_free_count", &vFreeCount, &len, nullptr, 0) != 0) {
      perror("sysctl(vm.stats.vm.v_free_count)");
      return;
   }

   // ------ Calculations ---------------------------------------------------
   const unsigned long long vmstatInactive = (unsigned long long)vInactiveCount * pageSize;
   const unsigned long long vmstatCache    = (unsigned long long)vCacheCount * pageSize;
   const unsigned long long vmstatFree     = (unsigned long long)vFreeCount *  pageSize;

   memoryTotal     = physMem;
   memoryAvailable = vmstatInactive + vmstatCache + vmstatFree;
   memoryUsed      = memoryTotal - memoryAvailable;

   // ------ Get information about swap -------------------------------------
   // Based on: https://cgit.freebsd.org/src/tree/sbin/swapon/swapon.c
   int mib[16];
   size_t mibsize = 16;
   if(sysctlnametomib("vm.swap_info", mib, &mibsize) != -0) {
      perror("sysctlnametomib(vm.swap_info)");
      return;
   }
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
#endif

   printf("system_mem_total=%llu\n", memoryTotal);
   printf("system_mem_used=%llu\n",  memoryUsed);
   printf("system_mem_free=%llu\n",  memoryAvailable);
   if(memoryTotal > 0) {
      printf("system_mem_usedpct=%1.1f\n", 100.0 * memoryUsed / memoryTotal);
      printf("system_mem_freepct=%1.1f\n", 100.0 * memoryAvailable / memoryTotal);
   }

   if(swapTotal > 0) {
      printf("system_swap_total=%llu\n", swapTotal);
      printf("system_swap_used=%llu\n",  swapUsed);
      printf("system_swap_free=%llu\n",  swapAvailable);
      printf("system_swap_usedpct=%1.1f\n", 100.0 * swapUsed / swapTotal);
      printf("system_swap_freepct=%1.1f\n", 100.0 * swapAvailable / swapTotal);
   }
   else {
      puts("system_swap_total=0");
   }
}


// ###### Print disk information ############################################
static void showDiskInformation()
{
   char         buffer[64];
   unsigned int value;

   if( (queryPipe("env LANGUAGE=en df -hT / | grep -vE '^Filesystem|shm' | awk '{ print $6 }' | tr -d '%'",
                  (char*)&buffer, sizeof(buffer))) &&
       (sscanf(buffer, "%u", &value) == 1) ) {
      printf("disk_root_pct=%1.1f\n", (double)value);
   }
   if( (queryPipe("env LANGUAGE=en df -hT /home | grep -vE '^Filesystem|shm' | awk '{ print $6 }' | tr -d '%'",
                  (char*)&buffer, sizeof(buffer))) &&
       (sscanf(buffer, "%u", &value) == 1) ) {
      printf("disk_home_pct=%1.1f\n", (double)value);
   }
   if( (queryPipe("env LANGUAGE=en df -hT /tmp | grep -vE '^Filesystem|shm' | awk '{ print $6 }' | tr -d '%'",
                  (char*)&buffer, sizeof(buffer))) &&
       (sscanf(buffer, "%u", &value) == 1) ) {
      printf("disk_tmp_pct=%1.1f\n", (double)value);
   }
   fputs("disk_list=\"root home tmp\"\n", stdout);
}


// ###### Print network information #########################################
static void showNetworkInformation(const bool filterLocalScope)
{
   // ====== Query available interfaces and their addresses =================
   struct ifaddrs* ifaddr;
   if(getifaddrs(&ifaddr) == -1) {
      perror("getifaddrs()");
      return;
   }

   // ====== Build list of interfaces and their addresses ==================
   struct interfaceaddress ifaArray[4096];
   unsigned int n = 0;
   for(struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
      if(ifa->ifa_addr != nullptr) {
         if( (ifa->ifa_addr->sa_family == AF_INET6) ||
             (ifa->ifa_addr->sa_family == AF_INET) ) {
            if(n >= (sizeof(ifaArray) / sizeof(ifaArray[0]))) {
               fprintf(stderr, "WARNING: Truncated list of interface addresses!\n");
               break;
            }
            if(ifa->ifa_addr->sa_family == AF_INET6) {

               if( filterLocalScope &&
                   ( (IN6_IS_ADDR_LOOPBACK(&((const struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr)) ||
                     (IN6_IS_ADDR_LINKLOCAL(&((const struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr)) ) ) {
                  continue;
               }
               const uint8_t* netmask =
                  (const uint8_t*)&((const struct sockaddr_in6*)ifa->ifa_netmask)->sin6_addr.s6_addr;
               ifaArray[n].prefixlen = countSetBits(netmask, 16);
            }
            else {
               if( filterLocalScope &&
                   (ntohl( ((const struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr) == INADDR_LOOPBACK) ) {
                  continue;
               }
               const uint8_t* netmask =
                  (const uint8_t*)&((const struct sockaddr_in*)ifa->ifa_netmask)->sin_addr;
               ifaArray[n].prefixlen = countSetBits(netmask, 4);
            }
            ifaArray[n].ifname    = ifa->ifa_name;
            ifaArray[n].address   = ifa->ifa_addr;
            ifaArray[n].flags     = ifa->ifa_flags;
            n++;
         }
#if defined(__linux)
         else if(ifa->ifa_addr->sa_family == AF_PACKET) {
#elif defined(__FreeBSD__)
         else if(ifa->ifa_addr->sa_family == AF_LINK) {
#endif
            ifaArray[n].ifname  = ifa->ifa_name;
            ifaArray[n].address = ifa->ifa_addr;
            ifaArray[n].flags   = ifa->ifa_flags;
            n++;
         }
      }
   }
   qsort(&ifaArray, n, sizeof(struct interfaceaddress),
         compareInterfaceAddresses);

   // ====== Print interfaces and their addresses ===========================
   unsigned int lastIfIndex = 0;
   int          lastFamily  = AF_UNSPEC;
   for(unsigned int i = 0; i < n; i++) {
      const unsigned int ifIndex = if_nametoindex(ifaArray[i].ifname);
      if( (lastIfIndex == 0) || (lastIfIndex != ifIndex) ) {
         if(lastIfIndex != 0) {
            puts("\"");
         }
         printf("netif_%u_name=\"%s\"\n", ifIndex, ifaArray[i].ifname);
         printf("netif_%u_flags=\"", ifIndex);
         printflags(ifaArray[i].flags);
         puts("\"");
         lastFamily = AF_UNSPEC;
      }

      if(lastFamily != ifaArray[i].address->sa_family) {
         if(lastFamily != AF_UNSPEC) {
            puts("\"");
         }
         switch(ifaArray[i].address->sa_family) {
            case AF_INET6:
               printf("netif_%u_ipv6=\"", ifIndex);
             break;
            case AF_INET:
               printf("netif_%u_ipv4=\"", ifIndex);
             break;
#if defined(__linux)
            case AF_PACKET:
#elif defined(__FreeBSD__)
            case AF_LINK:
#endif
               printf("netif_%u_mac=\"", ifIndex);
             break;
            default:
             break;
         }
      }
      else {
         fputs(" ", stdout);
      }

      printaddress(ifaArray[i].address, ifaArray[i].prefixlen);

      lastIfIndex = ifIndex;
      lastFamily  = ifaArray[i].address->sa_family;
   }
   puts("\"");

   // ====== Print interfaces list ==========================================
   lastIfIndex = 0;
   printf("netif_list=\"");
   for(unsigned int i = 0; i < n; i++) {
      const unsigned int ifIndex = if_nametoindex(ifaArray[i].ifname);
      if( (lastIfIndex == 0) || (lastIfIndex != ifIndex) ) {
         if(lastIfIndex != 0) {
            fputs(" ", stdout);
         }
         printf("%u", if_nametoindex(ifaArray[i].ifname));
      }
      lastIfIndex = ifIndex;
   }
   puts("\"");

   freeifaddrs(ifaddr);
}


// ###### Version ###########################################################
[[ noreturn ]] static void version()
{
   printf("get-system-info %s\n", SYSTEMTOOLS_VERSION);
   exit(0);
}


// ###### Usage #############################################################
[[ noreturn ]] static void usage(const char* program, const int exitCode)
{
   fprintf(stderr, "Usage: %s [-h|--help] [-v|--version]\n", program);
   exit(exitCode);
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   // ====== Initialise locale support ======================================
   if(setlocale(LC_ALL, "") == nullptr) {
      setlocale(LC_ALL, "C.UTF-8");   // "C" should exist on all systems!
   }

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
            fprintf(stderr, "ERROR: Invalid argument %s!", argv[optind - 1]);
            return 1;
          break;
      }
   }
   if(optind < argc) {
      usage(argv[0], 1);
   }

   // ====== Show system information in machine-readable form ===============
   showHostnameInformation();
   showUptimeInformation();
   showKernelInformation();
   showLoadInformation();
   showBatteryInformation();
   showMemoryInformation();
   showDiskInformation();
   showNetworkInformation(true);
   return 0;
}
