#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include	<sys/types.h>
#include <sys/utsname.h>
#ifdef __linux
#include <sys/sysinfo.h>
#elif __FreeBSD__
#include	<sys/sysctl.h>
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
   printf("F=0x%x: <%s>", flags, (flags & IFF_UP) ? "UP" : "DOWN");
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
      printf("hostname.long=\"%s.%s\"\n", hostname, domainname);
   }
   else {
       printf("hostname.long=\"%s\"\n", hostname);
   }
   printf("hostname.short=\"%s\"\n", hostname);
}


// ###### Print kernel information ##########################################
static void showKernelInformation()
{
   struct utsname kernelInfo;
   if(uname(&kernelInfo) == 0) {
      printf("system.sysname=\"%s\"\n",  kernelInfo.sysname);
      printf("system.nodename=\"%s\"\n", kernelInfo.nodename);
      printf("system.release=\"%s\"\n",  kernelInfo.release);
      printf("system.version=\"%s\"\n",  kernelInfo.version);
      printf("system.machine=\"%s\"\n",  kernelInfo.machine);
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
      printf("uptime.total=%1.9f\n",
             (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0));
      printf("uptime.days=%u\n",   days);
      printf("uptime.hours=%u\n",  hours);
      printf("uptime.mins=%u\n",   mins);
      printf("uptime.secs=%u\n",   secs);
      printf("uptime.nsecs=%lu\n", ts.tv_nsec);
   }
}


// ###### Print memory information ##########################################
static void showMemoryInformation()
{
   unsigned long memoryTotal     = 0;
   unsigned long memoryAvailable = 0;
   unsigned long memoryUsed      = 0;
   unsigned long swapTotal       = 0;
   unsigned long swapAvailable   = 0;
   unsigned long swapUsed        = 0;

#ifdef __linux
   struct sysinfo systemInfo;
   if(sysinfo(&systemInfo) == 0) {
      printf("system.procs: %u\n",       systemInfo.procs);

      memoryTotal     = systemInfo.totalram;
      memoryAvailable = systemInfo.freeram;
      memoryUsed      = systemInfo.totalram - systemInfo.freeram;

      swapTotal       = systemInfo.totalswap;
      swapAvailable   = systemInfo.freeswap;
      swapUsed        = systemInfo.totalswap - systemInfo.freeswap;
   }

#elif __FreeBSD__
   // ====== Query system information via sysctl ============================
   // Documentation: https://man.freebsd.org/cgi/man.cgi?query=sysctl&sektion=3

   // ------ Query hw.pagesize ----------------------------------------------
   unsigned int pageSize;
   size_t len = sizeof(pageSize);
   if(sysctlbyname("hw.pagesize", &pageSize, &len, NULL, 0) != 0) {
      perror("sysctl(hw.pagesize)");
      return 1;
   }

   // ------ Query hw.physmem -----------------------------------------------
   unsigned long physMem;
   len = sizeof(physMem);
   if(sysctlbyname("hw.physmem", &physMem, &len, NULL, 0) != 0) {
      perror("sysctl(hw.physmem)");
      return 1;
   }

   // ------ Query vm.stats.vm.v_inactive_count -----------------------------
   unsigned int vInactiveCount;
   len = sizeof(vInactiveCount);
   if(sysctlbyname("vm.stats.vm.v_inactive_count", &vInactiveCount, &len, NULL, 0) != 0) {
      perror("sysctl(vm.stats.vm.v_inactive_count)");
      return 1;
   }

   // ------ Query vm.stats.vm.v_cache_count -----------------------------
   unsigned int vCacheCount;
   len = sizeof(vCacheCount);
   if(sysctlbyname("vm.stats.vm.v_cache_count", &vCacheCount, &len, NULL, 0) != 0) {
      perror("sysctl(vm.stats.vm.v_cache_count)");
      return 1;
   }

   // ------ Query vm.stats.vm.v_free_count -----------------------------
   unsigned int vFreeCount;
   len = sizeof(vFreeCount);
   if(sysctlbyname("vm.stats.vm.v_free_count", &vFreeCount, &len, NULL, 0) != 0) {
      perror("sysctl(vm.stats.vm.v_free_count)");
      return 1;
   }

   // ------ Calculations ---------------------------------------------------
   const unsigned long vmstatInactive  = vInactiveCount * pageSize;
   const unsigned long vmstatCache     = vCacheCount * pageSize;
   const unsigned long vmstatFree      = vFreeCount *  pageSize;

   memoryTotal     = physMem;
   memoryAvailable = vmstatInactive + vmstatCache + vmstatFree;
   memoryUsed      = memoryTotal - memoryAvailable;

/*
      pageSize=$(sysctl -n hw.pagesize)
      memPhysical=$(sysctl -n hw.physmem)
      # vmstatAll=$(( $(sysctl -n vm.stats.vm.v_page_count)*pageSize ))
      # vmstatWired=$(( $(sysctl -n vm.stats.vm.v_wire_count)*pageSize ))
      # vmstatActive=$(( $(sysctl -n vm.stats.vm.v_active_count)*pageSize ))
      vmstatInactive=$(( $(sysctl -n vm.stats.vm.v_inactive_count)*pageSize ))
      vmstatCache=$(( $(sysctl -n vm.stats.vm.v_cache_count)*pageSize ))
      vmstatFree=$(( $(sysctl -n vm.stats.vm.v_free_count)*pageSize ))

      memoryTotal=$(( memPhysical/1048576 ))
      memoryAvailable=$(( (vmstatInactive+vmstatCache+vmstatFree)/1048576 ))
      memoryUsed=$(( memoryTotal-memoryAvailable ))

      swap=$(swapctl -sk)
      swapTotal=$(echo "${swap}" | awk '{ n=int($2/1024+0.5); print n; }')
      swapUsed=$(echo "${swap}" | awk '{ n=int($3/1024+0.5); print n; }')
      swapAvailable=$(( swapTotal-swapUsed ))
*/
#endif

   printf("system.ram.total=%lu\n", memoryTotal);
   printf("system.ram.used=%lu\n",  memoryUsed);
   printf("system.ram.free=%lu\n",  memoryAvailable);
   if(memoryTotal > 0) {
      printf("system.ram.usedpct=%1.1f\n", 100.0 * memoryUsed / memoryTotal);
      printf("system.ram.freepct=%1.1f\n", 100.0 * memoryAvailable / memoryTotal);
   }

   printf("system.swap.total=%lu\n", swapTotal);
   printf("system.swap.used=%lu\n",  swapUsed);
   printf("system.swap.free=%lu\n",  swapAvailable);
   if(swapTotal > 0) {
      printf("system.swap.usedpct=%1.1f\n", 100.0 * swapUsed / swapTotal);
      printf("system.swap.freepct=%1.1f\n", 100.0 * swapAvailable / swapTotal);
   }
}


// ###### Print network information #########################################
static void showNetworkInformation()
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
            ifaArray[n].ifname    = ifa->ifa_name;
            ifaArray[n].address   = ifa->ifa_addr;
            ifaArray[n].flags     = ifa->ifa_flags;
            if(ifa->ifa_addr->sa_family == AF_INET6) {
               const uint8_t* netmask =
                  (const uint8_t*)&((const struct sockaddr_in6*)ifa->ifa_netmask)->sin6_addr.s6_addr;
               ifaArray[n].prefixlen = countSetBits(netmask, 16);
            }
            else {
               const uint8_t* netmask =
                  (const uint8_t*)&((const struct sockaddr_in*)ifa->ifa_netmask)->sin_addr;
               ifaArray[n].prefixlen = countSetBits(netmask, 4);
            }
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
            puts("");
         }
         printf("netif.%s.flags=\"", ifaArray[i].ifname);
         printflags(ifaArray[i].flags);
      }

      if( (lastIfName == NULL) ||
          ((strcmp(lastIfName, ifaArray[i].ifname) != 0)) ||
          (lastFamily != ifaArray[i].address->sa_family) ) {
          printf("\nnetif.%s.ipv%u=\"",
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

   freeifaddrs(ifaddr);
}



// ###### Main program ######################################################
int main(void)
{
   showHostnameInformation();
   showUptimeInformation();
   showKernelInformation();
   showMemoryInformation();
   showNetworkInformation();


   // ====== Query system information =======================================

   return 0;
}
