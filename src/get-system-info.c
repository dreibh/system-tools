#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>


struct interfaceaddress {
   const char*            ifname;
   const struct sockaddr* address;
   unsigned char          prefixlen;
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


// ###### Print address #####################################################
void printaddress(const struct sockaddr* address)
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
   fputs(resolvedHost, stdout);
}



// ###### Main program ######################################################
int main(void)
{
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
        if( (ifa->ifa_addr->sa_family == AF_INET) ||
            (ifa->ifa_addr->sa_family == AF_INET6) ) {
            if(n >= (sizeof(ifaArray) / sizeof(ifaArray[0]))) {
                fprintf(stderr, "WARNING: Truncated list of interface addresses!\n");
                break;
            }
            ifaArray[n].ifname  = ifa->ifa_name;
            ifaArray[n].address = ifa->ifa_addr;
            n++;
        }
      }
   }
   qsort(&ifaArray, n, sizeof(struct interfaceaddress),
         compareInterfaceAddresses);

   // ====== Print interfaces and their addresses ===========================
   for(unsigned int i = 0; i < n; i++) {
      printf("%s:\t", ifaArray[i].ifname);
      printaddress(ifaArray[i].address);
      puts("");
   }

   freeifaddrs(ifaddr);
   return 0;
}
