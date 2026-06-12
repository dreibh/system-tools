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
// Get-System-Info
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
#include <utmpx.h>
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

#include "package-version.h"

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ < 202311L)
#ifndef nullptr
#define nullptr ((void*)0)
#endif
#endif


// Compatibility version of get-system-info, to allow for future changes:
// Currently, there is just version 0.
#define COMPATIBILITY_VERSION 0


struct interfaceaddress {
   const char*            ifname;
   const struct sockaddr* address;
   unsigned int           prefixlen;
   unsigned int           flags;
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


// ###### Print address #####################################################
static void printaddress(const struct sockaddr* address,
                         const unsigned int     prefixlen)
{
   if( (address->sa_family == AF_INET6) || (address->sa_family == AF_INET) ) {
      char resolvedHost[1025];
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
      printf("%s/%u", resolvedHost, prefixlen);
   }
#if defined(__linux__)
   else if(address->sa_family == AF_PACKET) {
      const struct sockaddr_ll* macAddress = (const struct sockaddr_ll*)address;
      for(unsigned int i = 0; i < macAddress->sll_halen; i++) {
         printf("%s%02x", (i > 0) ? ":" : "", macAddress->sll_addr[i]);
      }
   }
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
   else if(address->sa_family == AF_LINK) {
      const struct sockaddr_dl* macAddress = (const struct sockaddr_dl*)address;
      const uint8_t*            lladdr     = (const uint8_t*)LLADDR(macAddress);
      for(unsigned int i = 0; i < macAddress->sdl_alen; i++) {
         printf("%s%02x", (i > 0) ? ":" : "", lladdr[i]);
      }
   }
#else
#error Missing case!
#endif
}


// ###### Print interface flags #############################################
static void printflags(const unsigned int flags)
{
   printf("0x%x: <%s>", flags, (flags & IFF_UP) ? "UP" : "DOWN");
#if defined(IFF_LOWER_UP)
   if(flags & IFF_LOWER_UP) {
      fputs(" <LOWER_UP>", stdout);
   }
#endif
#if defined(IFF_RUNNING)
   if(flags & IFF_RUNNING) {
      fputs(" <RUNNING>", stdout);
   }
#endif
   if(flags & IFF_LOOPBACK) {
      fputs(" <LOOPBACK>", stdout);
   }
   if(flags & IFF_POINTOPOINT) {
      fputs(" <POINTOPOINT>", stdout);
   }
}


// ###### Print hostname information ########################################
static void showHostnameInformation(void)
{
   char hostname[256];
   if(gethostname(hostname, sizeof(hostname)) == 0) {
      hostname[sizeof(hostname) - 1] = 0x00;
   }
   else {
      strlcpy(hostname, "localhost", sizeof(hostname));
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
static void showKernelInformation(void)
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
#elif defined(__APPLE__)
   return clock_gettime(CLOCK_MONOTONIC, ts) == 0;
#else
#error Missing case!
#endif
   // return false;
}


// ###### Print uptime information ##########################################
static void showUptimeInformation(void)
{
   struct timespec ts;
   if(obtainUptime(&ts)) {
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


#if !(defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__))
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
         resultPtr = fgets(&result[resultSize], resultMaxSize - resultSize, fh);
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

#if defined(__linux__)
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
   }
   closedir(dir);

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
   if( (queryPipe("ps -aex -o pid= | wc -l", buffer, sizeof(buffer))) &&
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
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
   setutxent();
   struct utmpx* utx;
   while( (utx = getutxent()) != nullptr ) {
      if(utx->ut_type == USER_PROCESS) {
         count++;
      }
   }
   endutxent();

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
static void showLoadInformation(void)
{
   // ====== Cores and page size ============================================
   const unsigned int cores = sysconf(_SC_NPROCESSORS_ONLN);
   printf("system_cores=%u\n", cores);
   const unsigned int pageSize = sysconf(_SC_PAGESIZE);
   printf("system_pagesize=%u\n", pageSize);

   // ====== Number of running processes and number of users ================
   const unsigned int processCount = obtainProcessCount();
   printf("system_procs=%u\n", processCount);

   const unsigned int userCount = obtainUserCount();
   printf("system_users=%u\n", userCount);

   // ====== Load averages ==================================================
#if defined(__linux__)
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

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
   double loadavg[3];
   if(getloadavg(loadavg, 3) == 3) {
      const double fPercent = 100.0 / (double)cores;
      printf("system_load_avg1min=%1.6f\n",  loadavg[0]);
      printf("system_load_avg5min=%1.6f\n",  loadavg[1]);
      printf("system_load_avg15min=%1.6f\n", loadavg[2]);
      printf("system_load_avg1minpct=%1.4f\n",  loadavg[0] * fPercent);
      printf("system_load_avg5minpct=%1.4f\n",  loadavg[1] * fPercent);
      printf("system_load_avg15minpct=%1.4f\n", loadavg[2] * fPercent);
   }

#else
#error Missing case!
#endif
}


// ###### Print battery information #########################################
static void showBatteryInformation(void)
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
            printf("battery_%u_status=%u\n",   i, status);
            printf("battery_%u_capacity=%u\n", i, capacity);
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
               printf("battery_%u_status=%u\n",   i, status);
               printf("battery_%u_capacity=%u\n", i, capacity);
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
         printf("battery_0_status=%u\n",   status);
         printf("battery_0_capacity=%u\n", capacity);
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
            printf("battery_0_status=%u\n",   status);
            printf("battery_0_capacity=%u\n", capacity);
            batteryIDs[batteries++] = 0;
         }
      }
      close(apmFD);
   }

#else
#warning Missing case!
#endif

   fputs("battery_list=\"", stdout);
   for(unsigned int i = 0; i < batteries; i++) {
      printf((i > 0) ? " %u" : "%u", batteryIDs[i]);
   }
   puts("\"");
}


// ###### Print memory information ##########################################
static void showMemoryInformation(void)
{
   unsigned long long memoryTotal     = 0;
   unsigned long long memoryAvailable = 0;
   unsigned long long memoryUsed      = 0;
   unsigned long long swapTotal       = 0;
   unsigned long long swapAvailable   = 0;
   unsigned long long swapUsed        = 0;

#if defined(__linux__)
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


// ###### Obtain disk usage statistics ######################################
static bool obtainDiskUsage(const char* mountPoint, const char* label)
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
   printf("disk_%s_total=%llu\n",     label, totalSpace);
   printf("disk_%s_used=%llu\n",      label, usedSpace);
   printf("disk_%s_available=%llu\n", label, availableSpace);
   printf("disk_%s_pct=%1.3f\n",      label, usagePercentage);
   return true;
}


// ###### Print disk information ############################################
static void showDiskInformation(void)
{
   obtainDiskUsage("/",     "root");
   obtainDiskUsage("/home", "home");
   obtainDiskUsage("/tmp",  "tmp");
   // obtainDiskUsage("/boot/efi", "efi");
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
            ifaArray[n].ifname    = ifa->ifa_name;
            ifaArray[n].address   = ifa->ifa_addr;
            ifaArray[n].flags     = ifa->ifa_flags;
            n++;
            break;

         // ====== MAC address ==============================================
#if defined(__linux__)
         case AF_PACKET:
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
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
         }
      }
   }
   qsort(ifaArray, n, sizeof(struct interfaceaddress),
         compareInterfaceAddresses);

   // ====== Print interfaces and their addresses ===========================
   unsigned int lastIfIndex = 0;
   int          lastFamily  = AF_UNSPEC;
   for(unsigned int i = 0; i < n; i++) {
      ifIndices[i] = if_nametoindex(ifaArray[i].ifname);
      if(ifIndices[i] != 0) {
         if( (lastIfIndex == 0) || (lastIfIndex != ifIndices[i]) ) {
            if(lastIfIndex != 0) {
               puts("\"");
            }
            printf("netif_%u_name=\"%s\"\n", ifIndices[i], ifaArray[i].ifname);
            printf("netif_%u_flags=\"", ifIndices[i]);
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
                  printf("netif_%u_ipv6=\"", ifIndices[i]);
               break;
               case AF_INET:
                  printf("netif_%u_ipv4=\"", ifIndices[i]);
               break;
#if defined(__linux__)
               case AF_PACKET:
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
               case AF_LINK:
#else
#error Missing case!
#endif
                  printf("netif_%u_mac=\"", ifIndices[i]);
               break;
               default:
               break;
            }
         }
         else {
            fputs(" ", stdout);
         }

         printaddress(ifaArray[i].address, ifaArray[i].prefixlen);

         lastIfIndex = ifIndices[i];
         lastFamily  = ifaArray[i].address->sa_family;
      }
   }
   if(lastIfIndex != 0) {
      puts("\"");
   }

   // ====== Print interfaces list ==========================================
   lastIfIndex = 0;
   printf("netif_list=\"");
   for(unsigned int i = 0; i < n; i++) {
      if(ifIndices[i] != 0) {
         if( (lastIfIndex == 0) || (lastIfIndex != ifIndices[i]) ) {
            if(lastIfIndex != 0) {
               fputs(" ", stdout);
            }
            printf("%u", ifIndices[i]);
         }
         lastIfIndex = ifIndices[i];
      }
   }
   puts("\"");

   freeifaddrs(ifaddr);
}


// ###### Version ###########################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void version(void)
{
   printf("get-system-info %s\n", SYSTEMTOOLS_VERSION);
   exit(0);
}


// ###### Usage #############################################################
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
[[ noreturn ]]
#endif
static void usage(const char* program, const int exitCode)
{
   fprintf(stderr, "Usage: %s [-h|--help] [-v|--version] [-c|--compatibility version]\n", program);
   exit(exitCode);
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   unsigned int compatibilityVersion = COMPATIBILITY_VERSION;

   // ====== Initialise locale support ======================================
   if(setlocale(LC_ALL, "") == nullptr) {
      setlocale(LC_ALL, "C.UTF-8");   // "C" should exist on all systems!
   }

   // ====== Handle arguments ===============================================
   static const struct option long_options[] = {
      { "help",          no_argument,       0, 'h' },
      { "version",       no_argument,       0, 'v' },
      { "compatibility", required_argument, 0, 'c' },
      {  nullptr,        0,                 0, 0   }
   };

   int          option;
   int          longIndex;
   while( (option = getopt_long(argc, argv, "hvc:", long_options, &longIndex)) != -1 ) {
      switch(option) {
         case 'c':
            compatibilityVersion = atoll(optarg);
            if(compatibilityVersion > COMPATIBILITY_VERSION) {
               fprintf(stderr, "ERROR: Requested compatibility version %u > available version %u!\n",
                       compatibilityVersion, COMPATIBILITY_VERSION);
               return 1;
            }
          break;
         case 'v':
            version();
          break;
         case 'h':
         case '?':
            // Exit with 0 on h/help, exit with 1 on '?' (unknown option):
            usage(argv[0], (option == 'h') ? 0 : 1);
          break;
         case '-':
          break;
         default:
            // This should not happen: wrong getopt parameters, or missing case?
            fprintf(stderr, "INTERNAL ERROR: Unhandled option c=%c code=%x!\n",
                    (isprint(option) ? (char)option : ' '), option);
            return 1;
          break;
      }
   }
   if(optind < argc) {
      usage(argv[0], 1);
   }

   // ====== Show system information in machine-readable form ===============
   printf("compatibility=%u\n", compatibilityVersion);
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
