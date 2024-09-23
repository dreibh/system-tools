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
#ifdef __linux
#include <sys/sysinfo.h>
#elif __FreeBSD__
#include <sys/sysctl.h>
#include <vm/vm_param.h>
#endif

// FIXME: For some reasons, IFF_LOWER_UP seems to be undefined here.
#ifdef __linux
#define IFF_LOWER_UP (1 << 16)
#elif __FreeBSD__
#include <netlink/route/interface.h>
#endif


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
   char resolvedHost[NI_MAXHOST];
   int error = getnameinfo(address,
                           (address->sa_family == AF_INET6) ?
                              sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in),
                           (char*)&resolvedHost, sizeof(resolvedHost),
                           NULL, 0,
                           NI_NUMERICHOST);
   if(error != 0) {
      fprintf(stderr, "ERROR: getnameinfo() failed: %s\n", gai_strerror(error));
      exit(1);
   }
   fprintf(stdout, "%s/%u", resolvedHost, prefixlen);
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
   if(domainname != NULL) {
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
static bool query(const char* command, char* result, size_t resultMaxSize)
{
   FILE* pipeFH = popen(command, "r");
   if(pipeFH != NULL) {
      size_t resultSize = 0;
      char*  r;
      do {
         r = fgets((char*)&result[resultSize], resultMaxSize - resultSize, pipeFH);
         if(r == NULL) {
            break;
         }
         resultSize += strlen(r);
      }
      while(resultSize < resultMaxSize);
      pclose(pipeFH);
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
   if( (query("ps -aex -o pid= | wc -l", (char*)&buffer, sizeof(buffer))) &&
       (sscanf(buffer, "%u", &value) == 1) ) {
      printf("system_procs=%u\n", value);
   }
   else {
      printf("system_procs=0");
   }

   // ====== Number of users ================================================
   if( (query("who | cut -d' ' -f1 | sort -ud | wc -l", (char*)&buffer, sizeof(buffer))) &&
       (sscanf(buffer, "%u", &value) == 1) ) {
      printf("system_users=%u\n", value);
   }
   else {
      printf("system_users=0");
   }

   // ====== Load averages ==================================================
#ifdef __linux
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

#elif __FreeBSD__
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


// ###### Print memory information ##########################################
static void showMemoryInformation()
{
   unsigned long long memoryTotal     = 0;
   unsigned long long memoryAvailable = 0;
   unsigned long long memoryUsed      = 0;
   unsigned long long swapTotal       = 0;
   unsigned long long swapAvailable   = 0;
   unsigned long long swapUsed        = 0;

#ifdef __linux
   // ====== Query memory information via /proc =============================
   FILE* fh = fopen("/proc/meminfo", "r");
   if(fh != NULL) {
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

#elif __FreeBSD__
   // ====== Query system information via sysctl ============================
   // Documentation: https://man.freebsd.org/cgi/man.cgi?query=sysctl&sektion=3

   // ------ Query hw.pagesize ----------------------------------------------
   unsigned int pageSize;
   size_t len = sizeof(pageSize);
   if(sysctlbyname("hw.pagesize", &pageSize, &len, NULL, 0) != 0) {
      perror("sysctl(hw.pagesize)");
      return;
   }

   // ------ Query hw.physmem -----------------------------------------------
   unsigned long physMem;
   len = sizeof(physMem);
   if(sysctlbyname("hw.physmem", &physMem, &len, NULL, 0) != 0) {
      perror("sysctl(hw.physmem)");
      return;
   }

   // ------ Query vm.stats.vm.v_inactive_count -----------------------------
   unsigned int vInactiveCount;
   len = sizeof(vInactiveCount);
   if(sysctlbyname("vm.stats.vm.v_inactive_count", &vInactiveCount, &len, NULL, 0) != 0) {
      perror("sysctl(vm.stats.vm.v_inactive_count)");
      return;
   }

   // ------ Query vm.stats.vm.v_cache_count -----------------------------
   unsigned int vCacheCount;
   len = sizeof(vCacheCount);
   if(sysctlbyname("vm.stats.vm.v_cache_count", &vCacheCount, &len, NULL, 0) != 0) {
      perror("sysctl(vm.stats.vm.v_cache_count)");
      return;
   }

   // ------ Query vm.stats.vm.v_free_count -----------------------------
   unsigned int vFreeCount;
   len = sizeof(vFreeCount);
   if(sysctlbyname("vm.stats.vm.v_free_count", &vFreeCount, &len, NULL, 0) != 0) {
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
      if(sysctl(mib, mibsize + 1, &xsw, &size, NULL, 0) == -1) {
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
      puts("system_swap_total=NA");
   }
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
   for(struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
      if(ifa->ifa_addr != NULL) {
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
      }
   }
   qsort(&ifaArray, n, sizeof(struct interfaceaddress),
         compareInterfaceAddresses);

   // ====== Print interfaces and their addresses ===========================
   const char* lastIfName = NULL;
   int         lastFamily = AF_UNSPEC;
   for(unsigned int i = 0; i < n; i++) {
      if( (lastIfName == NULL) ||
          ((strcmp(lastIfName, ifaArray[i].ifname) != 0)) ) {
         if(lastIfName) {
            puts("\"");
         }
         printf("netif_%s_flags=\"", ifaArray[i].ifname);
         printflags(ifaArray[i].flags);
         puts("\"");
         lastFamily = AF_UNSPEC;
      }

      if(lastFamily != ifaArray[i].address->sa_family) {
         if(lastFamily != AF_UNSPEC) {
            puts("\"");
         }
         printf("netif_%s_ipv%u=\"",
                ifaArray[i].ifname,
                (ifaArray[i].address->sa_family == AF_INET6) ? 6 : 4);
      }
      else {
         fputs(" ", stdout);
      }

      printaddress(ifaArray[i].address, ifaArray[i].prefixlen);

      lastIfName = ifaArray[i].ifname;
      lastFamily = ifaArray[i].address->sa_family;
   }
   puts("\"");

   // ====== Print interfaces list ==========================================
   lastIfName = NULL;
   printf("netif_all=\"");
   for(unsigned int i = 0; i < n; i++) {
      if( (lastIfName == NULL) ||
          ((strcmp(lastIfName, ifaArray[i].ifname) != 0)) ) {
         if(lastIfName != NULL) {
            fputs(" ", stdout);
         }
         fputs(ifaArray[i].ifname, stdout);
      }
      lastIfName = ifaArray[i].ifname;
   }
   puts("\"");

   freeifaddrs(ifaddr);
}



// ###### Main program ######################################################
int main(void)
{
   // ====== Initialise i18n support ========================================
   setlocale(LC_ALL, "");

   // ====== Show system information in machine-readable form ===============
   showHostnameInformation();
   showUptimeInformation();
   showKernelInformation();
   showLoadInformation();
   showMemoryInformation();
   showNetworkInformation(true);
   return 0;
}
