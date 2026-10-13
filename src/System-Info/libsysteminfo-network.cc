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

#include <libsysteminfo.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <format>
#include <vector>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#if defined(__linux__)
#include <linux/if.h>
#include <netpacket/packet.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__APPLE__)
#include <net/if_dl.h>
#elif defined(__gnu_hurd__)
#include <net/if_arp.h>
#include <sys/ioctl.h>
#endif
#else
#include <windows.h>
#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#if !defined(IFF_POINTOPOINT)
#define IFF_POINTOPOINT 0x10
#endif
#if !defined(IFF_RUNNING)
#define IFF_RUNNING 0x40
#endif
#endif


// ###### Count set bits in byte array ######################################
static unsigned int countSetBits(const uint8_t*     array,
                                 const unsigned int size)
{
   unsigned int count = 0;
   for(unsigned int i = 0; i < size; ++i) {
      uint8_t byte = array[i];
      while(byte != 0) {
         count += (byte & 1U);
         byte >>= 1U;
      }
   }
   return count;
}


// ###### Format interface flags ############################################
static std::string formatFlags(const unsigned int flags)
{
   std::string result = std::format("0x{:x}: <{}>", flags, (flags & IFF_UP) ? "UP" : "DOWN");
#if defined(IFF_LOWER_UP)
   if(flags & IFF_LOWER_UP) {
      result += " <LOWER_UP>";
   }
#endif
#if defined(IFF_RUNNING)
   if(flags & IFF_RUNNING) {
      result += " <RUNNING>";
   }
#endif
   if(flags & IFF_LOOPBACK) {
      result += " <LOOPBACK>";
   }
#if defined(IFF_POINTOPOINT)
   if(flags & IFF_POINTOPOINT) {
      result += " <POINTOPOINT>";
   }
#endif
   return result;
}


// ###### Format address ####################################################
static std::string formatAddress(const struct sockaddr* address,
                                 const unsigned int     prefixlen)
{
   if((address->sa_family == AF_INET6) || (address->sa_family == AF_INET)) {
      char      host[NI_MAXHOST];
      socklen_t length = (address->sa_family == AF_INET6) ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
      if(getnameinfo(address, length, host, sizeof(host), nullptr, 0, NI_NUMERICHOST) == 0) {
         return std::format("{}/{}", host, prefixlen);
      }
   }
#if defined(__linux__)
   else if(address->sa_family == AF_PACKET) {
      const auto* mac = reinterpret_cast<const sockaddr_ll*>(address);
      std::string result;
      for(unsigned int i = 0; i < mac->sll_halen; ++i) {
         result += std::format("{}{:02x}", (i > 0) ? ":" : "", mac->sll_addr[i]);
      }
      return result;
   }
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__APPLE__)
   else if(address->sa_family == AF_LINK) {
      const auto* mac    = reinterpret_cast<const sockaddr_dl*>(address);
      const auto* lladdr = reinterpret_cast<const uint8_t*>(LLADDR(mac));
      std::string result;
      for(unsigned int i = 0; i < mac->sdl_alen; ++i) {
         result += std::format("{}{:02x}", (i > 0) ? ":" : "", lladdr[i]);
      }
      return result;
   }
#elif defined(__gnu_hurd__)
   else if(address->sa_family == ARPHRD_ETHER) {
      const auto* lladdr = reinterpret_cast<const uint8_t*>(&address->sa_data);
      std::string result;
      for(unsigned int i = 0; i < 6; ++i) {
         result += std::format("{}{:02x}", (i > 0) ? ":" : "", lladdr[i]);
      }
      return result;
   }
#endif
   return "";
}


#if !defined(_WIN32)
struct InterfaceAddress {
   std::string     IfName;
   const sockaddr* Address { nullptr };
   unsigned int    PrefixLen { 0 };
   unsigned int    Flags { 0 };
};


// ###### Comparison function for interfaceaddress type #####################
static bool compareInterfaceAddresses(const InterfaceAddress& a,
                                      const InterfaceAddress& b)
{
   if(a.IfName != b.IfName) {
      return a.IfName < b.IfName;
   }
   if(a.Address->sa_family != b.Address->sa_family) {
      return a.Address->sa_family < b.Address->sa_family;
   }

   if(a.Address->sa_family == AF_INET6) {
      const auto* ip1 = reinterpret_cast<const sockaddr_in6*>(a.Address);
      const auto* ip2 = reinterpret_cast<const sockaddr_in6*>(b.Address);
      return std::memcmp(&ip1->sin6_addr, &ip2->sin6_addr, 16) < 0;
   }
   if(a.Address->sa_family == AF_INET) {
      const auto* ip1 = reinterpret_cast<const sockaddr_in*>(a.Address);
      const auto* ip2 = reinterpret_cast<const sockaddr_in*>(b.Address);
      return std::memcmp(&ip1->sin_addr, &ip2->sin_addr, 4) < 0;
   }
#if defined(__linux__)
   if(a.Address->sa_family == AF_PACKET) {
      const auto* mac1 = reinterpret_cast<const sockaddr_ll*>(a.Address);
      const auto* mac2 = reinterpret_cast<const sockaddr_ll*>(b.Address);
      if(mac1->sll_halen != mac2->sll_halen) {
         return mac1->sll_halen < mac2->sll_halen;
      }
      return std::memcmp(mac1->sll_addr, mac2->sll_addr, mac1->sll_halen) < 0;
   }
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
   if(a.Address->sa_family == AF_LINK) {
      const auto* mac1 = reinterpret_cast<const sockaddr_dl*>(a.Address);
      const auto* mac2 = reinterpret_cast<const sockaddr_dl*>(b.Address);
      if(mac1->sdl_alen != mac2->sdl_alen) {
         return mac1->sdl_alen < mac2->sdl_alen;
      }
      return std::memcmp(LLADDR(mac1), LLADDR(mac2), mac1->sdl_alen) < 0;
   }
#endif
   return false;
}


// ##### Obtain network information on Unix systems #########################
void SystemInfo::obtainNetworkInformation(const bool filterLocalScope)
{
   ifaddrs* ifaddr = nullptr;
   if(getifaddrs(&ifaddr) == -1) {
      return;
   }

   std::vector<InterfaceAddress> ifaList;
   for(auto* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
      if(!ifa->ifa_addr) {
         continue;
      }

      unsigned int prefixlen = 0;
      if(ifa->ifa_addr->sa_family == AF_INET6) {
         const auto* sa6 = reinterpret_cast<const sockaddr_in6*>(ifa->ifa_addr);
         if(filterLocalScope && (IN6_IS_ADDR_LOOPBACK(&sa6->sin6_addr) || IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr))) {
            continue;
         }
         prefixlen = ifa->ifa_netmask ? countSetBits(reinterpret_cast<const uint8_t*>(&reinterpret_cast<const sockaddr_in6*>(ifa->ifa_netmask)->sin6_addr), 16) : 128;
      }
      else if(ifa->ifa_addr->sa_family == AF_INET) {
         const auto* sa4 = reinterpret_cast<const sockaddr_in*>(ifa->ifa_addr);
         if(filterLocalScope && (ntohl(sa4->sin_addr.s_addr) == INADDR_LOOPBACK)) {
            continue;
         }
         prefixlen = ifa->ifa_netmask ? countSetBits(reinterpret_cast<const uint8_t*>(&reinterpret_cast<const sockaddr_in*>(ifa->ifa_netmask)->sin_addr), 4) : 32;
      }

      ifaList.push_back(InterfaceAddress {
         .IfName    = ifa->ifa_name,
         .Address   = ifa->ifa_addr,
         .PrefixLen = prefixlen,
         .Flags     = ifa->ifa_flags
      });
   }
   std::sort(ifaList.begin(), ifaList.end(), compareInterfaceAddresses);

   unsigned int lastIfIndex = 0;
   int          lastFamily  = AF_UNSPEC;
   std::string  addressList;
   std::string  addressKey;
   std::vector<unsigned int> netifIndices;

   for(const auto& item : ifaList) {
      unsigned int ifIndex = if_nametoindex(item.IfName.c_str());
      if(ifIndex == 0) {
         continue;
      }

      if(lastIfIndex != ifIndex) {
         addString(std::format("netif_{}_name", ifIndex), item.IfName);
         addString(std::format("netif_{}_flags", ifIndex), formatFlags(item.Flags));
         if(!addressList.empty()) {
            addString(addressKey, addressList);
            addressList.clear();
         }
         netifIndices.push_back(ifIndex);
         lastFamily = AF_UNSPEC;

#if defined(__gnu_hurd__)
         int sd = socket(AF_INET, SOCK_DGRAM, 0);
         if(sd >= 0) {
            struct ifreq ifr { };
            std::strncpy(ifr.ifr_name, item.IfName.c_str(), IFNAMSIZ - 1);
            if((ioctl(sd, SIOCGIFHWADDR, &ifr) == 0) && (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER)) {
               addString(std::format("netif_{}_mac", ifIndex), formatAddress(&ifr.ifr_hwaddr, 48));
            }
            close(sd);
         }
#endif
      }

      if(lastFamily != item.Address->sa_family) {
         if(!addressList.empty()) {
            addString(addressKey, addressList);
            addressList.clear();
         }
         if(item.Address->sa_family == AF_INET6) {
            addressKey = std::format("netif_{}_ipv6", ifIndex);
         }
         else if(item.Address->sa_family == AF_INET) {
            addressKey = std::format("netif_{}_ipv4", ifIndex);
         }
         else {
            addressKey = std::format("netif_{}_mac", ifIndex);
         }
      }

      std::string addressString = formatAddress(item.Address, item.PrefixLen);
      if(!addressString.empty()) {
         if(!addressList.empty()) {
            addressList += ' ';
         }
         addressList += addressString;
      }
      lastIfIndex = ifIndex;
      lastFamily  = item.Address->sa_family;
   }
   if(!addressList.empty()) {
      addString(addressKey, addressList);
   }

   std::string netifList;
   for(size_t i = 0; i < netifIndices.size(); ++i) {
      if(i > 0) {
         netifList += ' ';
      }
      netifList += std::to_string(netifIndices[i]);
   }
   addString("netif_list", netifList);

   freeifaddrs(ifaddr);
}
#else
// ##### Obtain network information on Windows systems ######################
void SystemInfo::obtainNetworkInformation(const bool filterLocalScope)
{
   ULONG outBufLen  = 15360;
   auto* pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(std::malloc(outBufLen));
   if(pAddresses == nullptr) {
      return;
   }

   DWORD dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);
   if(dwRetVal == ERROR_BUFFER_OVERFLOW) {
      std::free(pAddresses);
      pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(std::malloc(outBufLen));
      if(pAddresses == nullptr) {
         return;
      }
      dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);
   }
   if(dwRetVal != NO_ERROR) {
      std::free(pAddresses);
      return;
   }

   std::vector<PIP_ADAPTER_ADDRESSES> adapterArray;
   for(auto* pAdapter = pAddresses; pAdapter != nullptr; pAdapter = pAdapter->Next) {
      adapterArray.push_back(pAdapter);
   }

   std::sort(adapterArray.begin(), adapterArray.end(), [](PIP_ADAPTER_ADDRESSES a, PIP_ADAPTER_ADDRESSES b) {
      const DWORD idx1 = a->IfIndex ? a->IfIndex : a->Ipv6IfIndex;
      const DWORD idx2 = b->IfIndex ? b->IfIndex : b->Ipv6IfIndex;
      return idx1 < idx2;
   });

   std::string netifList;
   for(auto* pAdapter : adapterArray) {
      const DWORD ifIndex = pAdapter->IfIndex ? pAdapter->IfIndex : pAdapter->Ipv6IfIndex;
      if(ifIndex == 0) {
         continue;
      }

      if(filterLocalScope && (pAdapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)) {
         continue;
      }

      char friendlyName[256] { 0 };
      if(pAdapter->FriendlyName != nullptr) {
         WideCharToMultiByte(CP_UTF8, 0, pAdapter->FriendlyName, -1, friendlyName, sizeof(friendlyName), nullptr, nullptr);
      }
      if((friendlyName[0] == 0x00) && (pAdapter->AdapterName != nullptr)) {
         std::strncpy(friendlyName, pAdapter->AdapterName, sizeof(friendlyName) - 1);
      }

      unsigned int flags = 0;
      if(pAdapter->OperStatus == IfOperStatusUp) {
         flags |= IFF_UP | IFF_RUNNING;
      }
      if(pAdapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
         flags |= IFF_LOOPBACK;
      }
      if((pAdapter->IfType == IF_TYPE_PPP) || (pAdapter->IfType == IF_TYPE_TUNNEL)) {
         flags |= IFF_POINTOPOINT;
      }

      std::string ipv4List;
      std::string ipv6List;
      for(auto* pUnicast = pAdapter->FirstUnicastAddress; pUnicast != nullptr; pUnicast = pUnicast->Next) {
         if(pUnicast->Address.lpSockaddr == nullptr) {
            continue;
         }

         const auto*  sa        = pUnicast->Address.lpSockaddr;
         const auto   prefixLen = static_cast<unsigned int>(pUnicast->OnLinkPrefixLength);

         if(sa->sa_family == AF_INET) {
            const auto* sin = reinterpret_cast<const sockaddr_in*>(sa);
            if(filterLocalScope && (ntohl(sin->sin_addr.s_addr) == INADDR_LOOPBACK)) {
               continue;
            }
            std::string addressString = formatAddress(sa, prefixLen);
            if(!addressString.empty()) {
               if(!ipv4List.empty()) {
                  ipv4List += ' ';
               }
               ipv4List += addressString;
            }
         }
         else if(sa->sa_family == AF_INET6) {
            const auto* sin6 = reinterpret_cast<const sockaddr_in6*>(sa);
            if(filterLocalScope && (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr) || IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))) {
               continue;
            }
            std::string addressString = formatAddress(sa, prefixLen);
            if(!addressString.empty()) {
               if(!ipv6List.empty()) {
                  ipv6List += ' ';
               }
               ipv6List += addressString;
            }
         }
      }

      std::string macAddress;
      if(pAdapter->PhysicalAddressLength > 0) {
         for(DWORD k = 0; k < pAdapter->PhysicalAddressLength; ++k) {
            macAddress += std::format("{}{:02x}", (k > 0) ? ":" : "", pAdapter->PhysicalAddress[k]);
         }
      }

      addString(std::format("netif_{}_name", ifIndex), friendlyName);
      addString(std::format("netif_{}_flags", ifIndex), formatFlags(flags));
      if(!macAddress.empty()) {
         addString(std::format("netif_{}_mac", ifIndex), macAddress);
      }
      if(!ipv4List.empty()) {
         addString(std::format("netif_{}_ipv4", ifIndex), ipv4List);
      }
      if(!ipv6List.empty()) {
         addString(std::format("netif_{}_ipv6", ifIndex), ipv6List);
      }

      if(!netifList.empty()) {
         netifList += ' ';
      }
      netifList += std::to_string(ifIndex);
   }

   addString("netif_list", netifList);
   std::free(pAddresses);
}
#endif
