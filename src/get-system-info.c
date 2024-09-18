#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>


union sockaddr_union {
   struct sockaddr     sa;
   struct sockaddr_in  in;
   struct sockaddr_in6 in6;
};

struct interfacesaddress {
   const char*                 ifname;
   const union sockaddr_union* address;
};


// ###### Print address #####################################################
void printaddress(const union sockaddr_union* address)
{
   char resolvedHost[NI_MAXHOST];
   char resolvedService[NI_MAXSERV];
   int error = getnameinfo(&address->sa, sizeof(address->sa),
                           (char*)&resolvedHost, sizeof(resolvedHost),
                           (char*)&resolvedService, sizeof(resolvedService),
                           NI_NUMERICHOST|NI_NUMERICSERV);
   if(error != 0) {
      fprintf(stderr, "ERROR: getnameinfo() failed: %s\n", gai_strerror(error));
      exit(1);
   }
   fputs(resolvedHost, stdout);
}


// ###### Main program ######################################################
int main(void)
{
    // ====== Create socket for querying the interface information ==========
    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sd < 0) {
       perror("socket()");
       return 1;
    }

    // ====== Query available interfaces ====================================
    char          buffer[8192];
    struct ifconf interfaceConfig;
    interfaceConfig.ifc_len = sizeof(buffer);
    interfaceConfig.ifc_buf = buffer;
    if(ioctl(sd, SIOCGIFCONF, &interfaceConfig) < 0) {
       perror("ioctl(SIOCGIFCONF)");
       return 1;
    }

    // ====== Build list of interfaces and their addresses ==================
    const struct ifreq* ifr            = interfaceConfig.ifc_req;
    const unsigned int  interfaceCount =
       interfaceConfig.ifc_len / sizeof(struct ifreq);
    for(int i = 0; i < interfaceCount; i++) {
        const struct ifreq* item = &ifr[i];

        const union sockaddr_union* address =
           (const union sockaddr_union*)&item->ifr_addr;

        printf("#%d\t%s:\t", i, item->ifr_name);
        printaddress(address);
        puts("");
    }

   close(sd);
   return 0;
}
