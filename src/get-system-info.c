#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>
#ifdef __linux
#include <sys/sysinfo.h>
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
int compareInterfaceAddresses(const void* a, const void* b)
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
void printaddress(const struct sockaddr* address,
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
void printflags(const u_int flags)
{
   if(flags & IFF_UP) {
      fputs("UP", stdout);
   }
   else {
      fputs("DOWN", stdout);
   }
   if(flags & IFF_LOWER_UP) {
      fputs(" LOWER_UP", stdout);
   }
   if(flags & IFF_LOOPBACK) {
      fputs(" LOOPBACK", stdout);
   }
   if(flags & IFF_POINTOPOINT) {
      fputs(" POINTOPOINT", stdout);
   }
   printf(" (F=0x%x)", flags);
}



// ###### Main program ######################################################
int main(void)
{
   // ====== Query hostname information =====================================
   char hostname[256];
   if(gethostname((char*)&hostname, sizeof(hostname)) == 0) {
      printf("host.name: %s\n", hostname);
   }
   else {
      hostname[0] = 0x00;
   }
   char domainname[256];
   if(getdomainname((char*)&domainname, sizeof(domainname)) == 0) {
      printf("host.domain: %s\n", domainname);
      if(strcmp(domainname, "(none)") == 0) {
          domainname[0] = 0x00;
      }
   }
   else {
      domainname[0] = 0x00;
   }
   if(domainname[0] != 0x00) {
      printf("host.fqdn: %s.%s\n", hostname, domainname);
   }
   else {
       printf("host.fqdn: %s\n", hostname);
   }


   // ====== Query uname information ========================================
   struct utsname kernelInfo;
   if(uname(&kernelInfo) == 0) {
      printf("uname.sysname: %s\n", kernelInfo.sysname);
      printf("uname.nodename: %s\n", kernelInfo.nodename);
      printf("uname.release: %s\n", kernelInfo.release);
      printf("uname.version: %s\n", kernelInfo.version);
      printf("uname.machine: %s\n", kernelInfo.machine);
   }


   // ====== Query system information =======================================
#ifdef __linux
   struct sysinfo systemInfo;
   if(sysinfo(&systemInfo) == 0) {
       const unsigned int days  = systemInfo.uptime / 86400;
       const unsigned int hours = (systemInfo.uptime / 3600) - (days * 24);
       const unsigned int mins  = (systemInfo.uptime / 60) - (days * 1440) - (hours * 60);
       const unsigned int secs  = systemInfo.uptime - (days * 86400) - (hours * 3600) - (mins * 60);
       printf("system.uptime: %lu (%u days %u hours %u mins %u secs)\n",
              systemInfo.uptime, days, hours, mins, secs);
       printf("system.procs: %u\n",       systemInfo.procs);
       printf("system.ram.total: %lu\n",  systemInfo.totalram);
       printf("system.ram.free: %lu\n",   systemInfo.freeram);
       printf("system.swap.total: %lu\n", systemInfo.totalswap);
       printf("system.swap.free: %lu\n",  systemInfo.freeswap);
   }
#elif __FreeBSD__
   struct timeval tv;
   getmicrouptime(&tv);
   const unsigned int days  = tv.tv_sec / 86400;
   const unsigned int hours = (tv.tv_sec / 3600) - (days * 24);
   const unsigned int mins  = (tv.tv_sec / 60) - (days * 1440) - (hours * 60);
   const unsigned int secs  = tv.tv_sec - (days * 86400) - (hours * 3600) - (mins * 60);
   printf("system.uptime: %lu (%u days %u hours %u mins %u secs)\n",
         tv.tv_sec, days, hours, mins, secs);

   print

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

   // ====== Query available interfaces and their addresses =================
   struct ifaddrs* ifaddr;
   if(getifaddrs(&ifaddr) == -1) {
      perror("getifaddrs()");
      return 1;
   }

   // ====== Build list of interfaces and their addresses ==================
   struct interfaceaddress ifaArray[16384];
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
         printf("netif.%s.flags:\t", ifaArray[i].ifname);
         printflags(ifaArray[i].flags);
      }

      if( (lastIfName == NULL) ||
          ((strcmp(lastIfName, ifaArray[i].ifname) != 0)) ||
          (lastFamily != ifaArray[i].address->sa_family) ) {
          printf("\nnetif.%s.ipv%u:\t",
                 ifaArray[i].ifname,
                 (ifaArray[i].address->sa_family == AF_INET6) ? 6 : 4);
      }
      else {
         fputs("\t", stdout);
      }

      printaddress(ifaArray[i].address, ifaArray[i].prefixlen);

      lastIfName = ifaArray[i].ifname;
      lastFamily = ifaArray[i].address->sa_family;
   }
   puts("");

   freeifaddrs(ifaddr);
   return 0;
}
