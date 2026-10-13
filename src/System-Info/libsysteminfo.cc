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
#include <array>
#include <cctype>
#include <chrono>
#include <cstring>
#include <format>
#include <fstream>
#include <iomanip>
#include <vector>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif

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
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <utmpx.h>
#include <vm/vm_param.h>
#elif defined(__NetBSD__)
#include <net/if_dl.h>
#include <sys/envsys.h>
#include <sys/ioctl.h>
#include <sys/swap.h>
#include <sys/sysctl.h>
#include <utmpx.h>
#include <uvm/uvm_extern.h>
#elif defined(__OpenBSD__)
#include <machine/apmvar.h>
#include <net/if_dl.h>
#include <sys/ioctl.h>
#include <sys/swap.h>
#include <sys/sysctl.h>
#include <utmp.h>
#include <uvm/uvm_extern.h>
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
#include <net/if_dl.h>
#include <sys/sysctl.h>
#include <utmpx.h>
#elif defined(_WIN32)
#include <windows.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wtsapi32.h>
#if !defined(IFF_POINTOPOINT)
#define IFF_POINTOPOINT 0x10
#endif
#if !defined(IFF_RUNNING)
#define IFF_RUNNING 0x40
#endif
#endif


// ###### Count set bits in byte array ######################################
static unsigned int countSetBits(const uint8_t* array, const unsigned int size)
{
   unsigned int count = 0;
   for (unsigned int i = 0; i < size; ++i) {
      uint8_t byte = array[i];
      while (byte != 0) {
         count += (byte & 1U);
         byte >>= 1U;
      }
   }
   return count;
}


// ###### Format interface flags ############################################
static std::string formatFlags(const unsigned int flags)
{
   std::string res = std::format("0x{:x}: <{}>", flags, (flags & IFF_UP) ? "UP" : "DOWN");
#if defined(IFF_LOWER_UP)
   if (flags & IFF_LOWER_UP) res += " <LOWER_UP>";
#endif
#if defined(IFF_RUNNING)
   if (flags & IFF_RUNNING) res += " <RUNNING>";
#endif
   if (flags & IFF_LOOPBACK) res += " <LOOPBACK>";
#if defined(IFF_POINTOPOINT)
   if (flags & IFF_POINTOPOINT) res += " <POINTOPOINT>";
#endif
   return res;
}


// ###### Format address ####################################################
static std::string formatAddress(const struct sockaddr* address,
                                 const unsigned int     prefixlen)
{
   if ((address->sa_family == AF_INET6) || (address->sa_family == AF_INET)) {
      char host[NI_MAXHOST];
      socklen_t len = (address->sa_family == AF_INET6) ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
      if (getnameinfo(address, len, host, sizeof(host), nullptr, 0, NI_NUMERICHOST) == 0) {
         return std::format("{}/{}", host, prefixlen);
      }
   }
#if defined(__linux__)
   else if (address->sa_family == AF_PACKET) {
      const auto* mac = reinterpret_cast<const sockaddr_ll*>(address);
      std::string res;
      for (unsigned int i = 0; i < mac->sll_halen; ++i) {
         res += std::format("{}{:02x}", (i > 0) ? ":" : "", mac->sll_addr[i]);
      }
      return res;
   }
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__APPLE__)
   else if (address->sa_family == AF_LINK) {
      const auto* mac = reinterpret_cast<const sockaddr_dl*>(address);
      const auto* lladdr = reinterpret_cast<const uint8_t*>(LLADDR(mac));
      std::string res;
      for (unsigned int i = 0; i < mac->sdl_alen; ++i) {
         res += std::format("{}{:02x}", (i > 0) ? ":" : "", lladdr[i]);
      }
      return res;
   }
#endif
   return "";
}


// ###### Obtain SystemInfo #################################################
SystemInfo::SystemInfo(const unsigned int compatibilityVersion,
                       const uint32_t     flags)
   : CompatibilityVersion(compatibilityVersion)
{
   if (compatibilityVersion > LSI_COMPATIBILITY_VERSION) {
      throw SystemInfoException(std::format(
         "Requested compatibility version {} > available version {}!",
         compatibilityVersion, LSI_COMPATIBILITY_VERSION));
   }

   addUInt32("compatibility", compatibilityVersion);

   if (flags & Flags::Hostname) obtainHostnameInformation();
   if (flags & Flags::Uptime)   obtainUptimeInformation();
   if (flags & Flags::Kernel)   obtainKernelInformation();
   if (flags & Flags::Load)     obtainLoadInformation();
   if (flags & Flags::Memory)   obtainMemoryInformation();
   if (flags & Flags::Storage)  obtainStorageInformation();
   if (flags & Flags::Network)  obtainNetworkInformation(true);
   if (flags & Flags::Battery)  obtainBatteryInformation();
}


// ##### Obtain SystemInfo ##################################################
void SystemInfo::print(std::ostream& os) const
{
   for (const auto& [key, entry] : Entries) {
      std::visit([&](const auto& val) {
         using T = std::decay_t<decltype(val)>;
         if constexpr (std::is_same_v<T, std::monostate>) {
            os << std::format("{}=(INVALID!)\n", key);
         } else if constexpr (std::is_same_v<T, std::string>) {
            os << std::format("{}=\"{}\"\n", key, val);
         } else if constexpr (std::is_same_v<T, double>) {
            if (entry.DisplayHint >= 0) {
               os << std::format("{}={:.{}f}\n", key, val, entry.DisplayHint);
            } else {
               os << std::format("{}={:f}\n", key, val);
            }
         } else {
            os << std::format("{}={}\n", key, val);
         }
      }, entry.Value);
   }
}


// ###### Add key with string value #########################################
void SystemInfo::addString(std::string_view key, std::string_view value)
{
   Entries[std::string(key)] = Entry{ .Value = std::string(value), .DisplayHint = 0 };
}


// ###### Add key with 32-bit integer value #################################
void SystemInfo::addInt32(std::string_view key, const int32_t value)
{
   Entries[std::string(key)] = Entry{ .Value = value, .DisplayHint = 0 };
}


// ###### Add key with 64-bit integer value #################################
void SystemInfo::addInt64(std::string_view key, const int64_t value)
{
   Entries[std::string(key)] = Entry{ .Value = value, .DisplayHint = 0 };
}


// ###### Add key with 32-bit unsigned integer value ########################
void SystemInfo::addUInt32(std::string_view key, const uint32_t value)
{
   Entries[std::string(key)] = Entry{ .Value = value, .DisplayHint = 0 };
}


// ###### Add key with 64-bit unsigned integer value ########################
void SystemInfo::addUInt64(std::string_view key, const uint64_t value)
{
   Entries[std::string(key)] = Entry{ .Value = value, .DisplayHint = 0 };
}


// ###### Add key with double value #########################################
void SystemInfo::addDouble(std::string_view key,
                           const double     value,
                           const int        displayHint)
{
   Entries[std::string(key)] = Entry{ .Value = value, .DisplayHint = displayHint };
}


// ###### Check whether key exists (returns value type or SIET_INVALID) #####
SystemInfo::ValueType SystemInfo::exists(std::string_view key) const
{
   auto it = Entries.find(std::string(key));
   if (it == Entries.end()) return ValueType::Invalid;

   return std::visit([](const auto& val) -> ValueType {
      using T = std::decay_t<decltype(val)>;
      if constexpr (std::is_same_v<T, std::string>)  return ValueType::String;
      if constexpr (std::is_same_v<T, uint32_t>)     return ValueType::UInt32;
      if constexpr (std::is_same_v<T, uint64_t>)     return ValueType::UInt64;
      if constexpr (std::is_same_v<T, int32_t>)      return ValueType::Int32;
      if constexpr (std::is_same_v<T, int64_t>)      return ValueType::Int64;
      if constexpr (std::is_same_v<T, double>)       return ValueType::Double;
      return ValueType::Invalid;
   }, it->second.Value);
}


// ###### Get string value, or default if not found #########################
std::string SystemInfo::getString(std::string_view key, std::string_view defaultValue) const
{
   if (auto it = Entries.find(std::string(key)); it != Entries.end()) {
      if (const auto* val = std::get_if<std::string>(&it->second.Value)) return *val;
   }
   return std::string(defaultValue);
}


// ###### Get 32-bit integer value, or default if not found #################
int32_t SystemInfo::getInt32(std::string_view key, const int32_t defaultValue) const
{
   if (auto it = Entries.find(std::string(key)); it != Entries.end()) {
      if (const auto* val = std::get_if<int32_t>(&it->second.Value)) return *val;
   }
   return defaultValue;
}


// ###### Get 64-bit integer value, or default if not found #################
int64_t SystemInfo::getInt64(std::string_view key, const int64_t defaultValue) const
{
   if (auto it = Entries.find(std::string(key)); it != Entries.end()) {
      if (const auto* val = std::get_if<int64_t>(&it->second.Value)) return *val;
   }
   return defaultValue;
}


// ###### Get 32-bit unsigned integer value, or default if not found ########
uint32_t SystemInfo::getUInt32(std::string_view key, const uint32_t defaultValue) const
{
   if (auto it = Entries.find(std::string(key)); it != Entries.end()) {
      if (const auto* val = std::get_if<uint32_t>(&it->second.Value)) return *val;
   }
   return defaultValue;
}


// ###### Get 64-bit unsigned integer value, or default if not found ########
uint64_t SystemInfo::getUInt64(std::string_view key, const uint64_t defaultValue) const
{
   if (auto it = Entries.find(std::string(key)); it != Entries.end()) {
      if (const auto* val = std::get_if<uint64_t>(&it->second.Value)) return *val;
   }
   return defaultValue;
}


// ###### Get double value, or default if not found #########################
double SystemInfo::getDouble(std::string_view key, const double defaultValue) const
{
   if (auto it = Entries.find(std::string(key)); it != Entries.end()) {
      if (const auto* val = std::get_if<double>(&it->second.Value)) return *val;
   }
   return defaultValue;
}


// ##### Obtain hostname information ########################################
void SystemInfo::obtainHostnameInformation()
{
   char hostname[256] = {0};
   if (gethostname(hostname, sizeof(hostname)) != 0) {
      std::strncpy(hostname, "localhost", sizeof(hostname) - 1);
   }
   std::string longHost(hostname);
   addString("hostname_long", longHost);

   std::string shortHost = longHost;
   std::string domain;
   if (auto dot = longHost.find('.'); dot != std::string::npos) {
      shortHost = longHost.substr(0, dot);
      domain = longHost.substr(dot + 1);
   }
   addString("domainname", domain);
   addString("hostname_short", shortHost);
}


// ###### Obtain kernel information #########################################
void SystemInfo::obtainKernelInformation()
{
#if !defined(_WIN32)
   struct utsname kernelInfo;
   if (uname(&kernelInfo) >= 0) {
      addString("system_sysname",  kernelInfo.sysname);
      addString("system_nodename", kernelInfo.nodename);
      addString("system_release",  kernelInfo.release);
      addString("system_version",  kernelInfo.version);
      addString("system_machine",  kernelInfo.machine);
   }
#else
   addString("system_sysname", "Windows");
   char nodename[256] = "localhost";
   DWORD size = sizeof(nodename);
   GetComputerNameA(nodename, &size);
   addString("system_nodename", nodename);

   std::string release = "10.0";
   std::string version = "Windows";
   HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
   if (hNtdll) {
      using fnRtlGetVersion = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
      auto pRtlGetVersion = reinterpret_cast<fnRtlGetVersion>(GetProcAddress(hNtdll, "RtlGetVersion"));
      if (pRtlGetVersion) {
         RTL_OSVERSIONINFOW rovi = { sizeof(rovi) };
         if (pRtlGetVersion(&rovi) == 0) {
            release = std::format("{}.{}", rovi.dwMajorVersion, rovi.dwMinorVersion);
            version = std::format("Windows Build {}", rovi.dwBuildNumber);
         }
      }
   }
   SYSTEM_INFO sysInfo;
   GetNativeSystemInfo(&sysInfo);
   const char* machine = "x86";
   if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) machine = "x86_64";
   else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) machine = "ARM64";
   else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) machine = "ARM";

   addString("system_release", release);
   addString("system_version", version);
   addString("system_machine", machine);
#endif
}


// ###### Get the system uptime #############################################
static bool getUptime(struct timespec* ts)
{
#if defined(__linux__) || defined(__OpenBSD__)
   return clock_gettime(CLOCK_BOOTTIME, ts) == 0;
#elif defined(__FreeBSD__)
   return clock_gettime(CLOCK_UPTIME_PRECISE, ts) == 0;
#elif defined(__NetBSD__) || defined(__sun__) || defined(__gnu_hurd__) || defined(__APPLE__)
   return clock_gettime(CLOCK_MONOTONIC, ts) == 0;
#elif defined(_WIN32)
   const ULONGLONG ms = GetTickCount64();
   ts->tv_sec = static_cast<time_t>(ms / 1000);
   ts->tv_nsec = static_cast<long>((ms % 1000) * 1'000'000);
   return true;
#else
   return false;
#endif
}


// ##### Obtain uptime information ##########################################
void SystemInfo::obtainUptimeInformation()
{
   struct timespec ts{};
   if (getUptime(&ts)) {
      auto totalSecs = static_cast<uint64_t>(ts.tv_sec);
      addDouble("uptime_total", static_cast<double>(ts.tv_sec) + (static_cast<double>(ts.tv_nsec) / 1e9), 9);
      addUInt32("uptime_days",  static_cast<uint32_t>(totalSecs / 86400));
      addUInt32("uptime_hours", static_cast<uint32_t>((totalSecs / 3600) % 24));
      addUInt32("uptime_mins",  static_cast<uint32_t>((totalSecs / 60) % 60));
      addUInt32("uptime_secs",  static_cast<uint32_t>(totalSecs % 60));
      addUInt32("uptime_nsecs", static_cast<uint32_t>(ts.tv_nsec));
   }
}


// ###### Obtain the number of processes on the system ######################
static unsigned int getProcessCount()
{
   unsigned int count = 0;
#if defined(__linux__) || defined(__sun__) || defined(__gnu_hurd__)
   if (DIR* dir = opendir("/proc"); dir != nullptr) {
      while (dirent* entry = readdir(dir)) {
         if (std::isdigit(static_cast<unsigned char>(entry->d_name[0]))) ++count;
      }
      closedir(dir);
   }
#elif defined(_WIN32)
   HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if (hSnap != INVALID_HANDLE_VALUE) {
      PROCESSENTRY32 pe32{ sizeof(PROCESSENTRY32) };
      if (Process32First(hSnap, &pe32)) {
         do { ++count; } while (Process32Next(hSnap, &pe32));
      }
      CloseHandle(hSnap);
   }
#endif
   return count;
}


// ###### Obtain the number of users on the system ##########################
static unsigned int getUserCount()
{
   unsigned int count = 0;
#if defined(__linux__) && defined(ENABLE_SYSTEMD)
   char** sessions = nullptr;
   int n = sd_get_sessions(&sessions);
   if (n >= 0) {
      for (int i = 0; i < n; ++i) {
         char* cls = nullptr;
         if (sd_session_get_class(sessions[i], &cls) >= 0) {
            if (cls && std::strcmp(cls, "user") == 0) ++count;
            free(cls);
         }
         free(sessions[i]);
      }
      free(sessions);
   }
#elif !defined(_WIN32)
   setutxent();
   while (utmpx* utx = getutxent()) {
      if (utx->ut_type == USER_PROCESS) ++count;
   }
   endutxent();
#endif
   return count;
}


// ##### Obtain load information ############################################
void SystemInfo::obtainLoadInformation()
{
#if !defined(_WIN32)
   const auto cores    = static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));
   const auto pageSize = static_cast<uint32_t>(sysconf(_SC_PAGESIZE));
#else
   SYSTEM_INFO sysInfo;
   GetSystemInfo(&sysInfo);
   const auto cores    = static_cast<uint32_t>(sysInfo.dwNumberOfProcessors);
   const auto pageSize = static_cast<uint32_t>(sysInfo.dwPageSize);
#endif
   addUInt32("system_cores", cores);
   addUInt32("system_pagesize", pageSize);
   addUInt32("system_procs", getProcessCount());
   addUInt32("system_users", getUserCount());

#if !defined(_WIN32)
   double loadavg[3];
   if (getloadavg(loadavg, 3) == 3) {
      double fPct = (cores > 0) ? (100.0 / cores) : 0.0;
      addDouble("system_load_avg1min",     loadavg[0], 6);
      addDouble("system_load_avg5min",     loadavg[1], 6);
      addDouble("system_load_avg15min",    loadavg[2], 6);
      addDouble("system_load_avg1minpct",  loadavg[0] * fPct, 4);
      addDouble("system_load_avg5minpct",  loadavg[1] * fPct, 4);
      addDouble("system_load_avg15minpct", loadavg[2] * fPct, 4);
   }
#endif
}


// ##### Obtain memory information ##########################################
void SystemInfo::obtainMemoryInformation()
{
   uint64_t memTotal = 0, memAvail = 0, memUsed = 0;
   uint64_t swapTotal = 0, swapAvail = 0, swapUsed = 0;

#if defined(__linux__) || defined(__gnu_hurd__)
   std::ifstream meminfo("/proc/meminfo");
   std::string line;
   while (std::getline(meminfo, line)) {
      uint64_t val = 0;
      if (sscanf(line.c_str(), "MemTotal: %llu", &val) == 1) memTotal = val * 1024;
      else if (sscanf(line.c_str(), "MemAvailable: %llu", &val) == 1) memAvail = val * 1024;
      else if (sscanf(line.c_str(), "SwapTotal: %llu", &val) == 1) swapTotal = val * 1024;
      else if (sscanf(line.c_str(), "SwapFree: %llu", &val) == 1) swapAvail = val * 1024;
   }
   memUsed = memTotal - memAvail;
   swapUsed = swapTotal - swapAvail;
#elif defined(_WIN32)
   MEMORYSTATUSEX st{ sizeof(MEMORYSTATUSEX) };
   if (GlobalMemoryStatusEx(&st)) {
      memTotal = st.ullTotalPhys;
      memAvail = st.ullAvailPhys;
      memUsed = memTotal - memAvail;
   }
#endif

   addUInt64("system_mem_total", memTotal);
   addUInt64("system_mem_used",  memUsed);
   addUInt64("system_mem_free",  memAvail);
   if (memTotal > 0) {
      addDouble("system_mem_usedpct", 100.0 * memUsed / memTotal, 1);
      addDouble("system_mem_freepct", 100.0 * memAvail / memTotal, 1);
   }
   addUInt64("system_swap_total", swapTotal);
   addUInt64("system_swap_used",  swapUsed);
   addUInt64("system_swap_free",  swapAvail);
   if (swapTotal > 0) {
      addDouble("system_swap_usedpct", 100.0 * swapUsed / swapTotal, 1);
      addDouble("system_swap_freepct", 100.0 * swapAvail / swapTotal, 1);
   }
}


// ###### Obtain disk usage statistics ######################################
bool SystemInfo::getDiskUsage(std::string_view mountPoint, std::string_view label)
{
#if !defined(_WIN32)
   struct statvfs status{};
   if (statvfs(std::string(mountPoint).c_str(), &status) != 0) return false;

   uint64_t total = static_cast<uint64_t>(status.f_blocks) * status.f_frsize;
   uint64_t avail = static_cast<uint64_t>(status.f_bavail) * status.f_frsize;
   uint64_t freeSp = static_cast<uint64_t>(status.f_bfree) * status.f_frsize;
#else
   ULARGE_INTEGER freeBytes, totalBytes, totalFree;
   if (!GetDiskFreeSpaceExA(std::string(mountPoint).c_str(), &freeBytes, &totalBytes, &totalFree)) return false;

   uint64_t total = totalBytes.QuadPart;
   uint64_t avail = freeBytes.QuadPart;
   uint64_t freeSp = totalFree.QuadPart;
#endif
   uint64_t used = total - freeSp;
   double pct = (total > 0) ? (100.0 * static_cast<double>(total - avail) / total) : 0.0;

   addUInt64(std::format("disk_{}_total", label), total);
   addUInt64(std::format("disk_{}_used", label), used);
   addUInt64(std::format("disk_{}_available", label), avail);
   addDouble(std::format("disk_{}_pct", label), pct, 3);
   return true;
}


// ##### Obtain disk information ############################################
void SystemInfo::obtainStorageInformation()
{
   getDiskUsage("/", "root");
   getDiskUsage("/tmp", "tmp");
#if !defined(_WIN32)
   getDiskUsage("/home", "home");
   addString("disk_list", "root tmp home");
#else
   getDiskUsage("/c", "c");
   addString("disk_list", "root tmp c");
#endif
}


#if !defined(_WIN32)
struct InterfaceAddress {
   std::string  IfName;
   const sockaddr* Address{nullptr};
   unsigned int PrefixLen{0};
   unsigned int Flags{0};
};


// ###### Comparison function for interfaceaddress type #####################
static bool compareInterfaceAddresses(const InterfaceAddress& a, const InterfaceAddress& b)
{
   if (a.IfName != b.IfName) return a.IfName < b.IfName;
   if (a.Address->sa_family != b.Address->sa_family) return a.Address->sa_family < b.Address->sa_family;

   if (a.Address->sa_family == AF_INET6) {
      const auto* ip1 = reinterpret_cast<const sockaddr_in6*>(a.Address);
      const auto* ip2 = reinterpret_cast<const sockaddr_in6*>(b.Address);
      return std::memcmp(&ip1->sin6_addr, &ip2->sin6_addr, 16) < 0;
   }
   if (a.Address->sa_family == AF_INET) {
      const auto* ip1 = reinterpret_cast<const sockaddr_in*>(a.Address);
      const auto* ip2 = reinterpret_cast<const sockaddr_in*>(b.Address);
      return std::memcmp(&ip1->sin_addr, &ip2->sin_addr, 4) < 0;
   }
   return false;
}


// ##### Obtain network information on Unix systems #########################
void SystemInfo::obtainNetworkInformation(const bool filterLocalScope)
{
   ifaddrs* ifaddr = nullptr;
   if (getifaddrs(&ifaddr) == -1) return;

   std::vector<InterfaceAddress> ifaList;
   for (auto* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
      if (!ifa->ifa_addr) continue;

      unsigned int prefixlen = 0;
      if (ifa->ifa_addr->sa_family == AF_INET6) {
         const auto* sa6 = reinterpret_cast<const sockaddr_in6*>(ifa->ifa_addr);
         if (filterLocalScope && (IN6_IS_ADDR_LOOPBACK(&sa6->sin6_addr) || IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr))) {
            continue;
         }
         prefixlen = ifa->ifa_netmask ? countSetBits(reinterpret_cast<const uint8_t*>(&reinterpret_cast<const sockaddr_in6*>(ifa->ifa_netmask)->sin6_addr), 16) : 128;
      } else if (ifa->ifa_addr->sa_family == AF_INET) {
         const auto* sa4 = reinterpret_cast<const sockaddr_in*>(ifa->ifa_addr);
         if (filterLocalScope && (ntohl(sa4->sin_addr.s_addr) == INADDR_LOOPBACK)) {
            continue;
         }
         prefixlen = ifa->ifa_netmask ? countSetBits(reinterpret_cast<const uint8_t*>(&reinterpret_cast<const sockaddr_in*>(ifa->ifa_netmask)->sin_addr), 4) : 32;
      }
      ifaList.push_back({ ifa->ifa_name, ifa->ifa_addr, prefixlen, ifa->ifa_flags });
   }
   std::sort(ifaList.begin(), ifaList.end(), compareInterfaceAddresses);

   unsigned int lastIfIndex = 0;
   int lastFamily = AF_UNSPEC;
   std::string addressList;
   std::string addressKey;
   std::vector<unsigned int> netifIndices;

   for (const auto& item : ifaList) {
      unsigned int ifIdx = if_nametoindex(item.IfName.c_str());
      if (ifIdx == 0) continue;

      if (lastIfIndex != ifIdx) {
         addString(std::format("netif_{}_name", ifIdx), item.IfName);
         addString(std::format("netif_{}_flags", ifIdx), formatFlags(item.Flags));
         if (!addressList.empty()) {
            addString(addressKey, addressList);
            addressList.clear();
         }
         netifIndices.push_back(ifIdx);
         lastFamily = AF_UNSPEC;
      }

      if (lastFamily != item.Address->sa_family) {
         if (!addressList.empty()) {
            addString(addressKey, addressList);
            addressList.clear();
         }
         if (item.Address->sa_family == AF_INET6) addressKey = std::format("netif_{}_ipv6", ifIdx);
         else if (item.Address->sa_family == AF_INET) addressKey = std::format("netif_{}_ipv4", ifIdx);
         else addressKey = std::format("netif_{}_mac", ifIdx);
      }

      std::string addrStr = formatAddress(item.Address, item.PrefixLen);
      if (!addrStr.empty()) {
         if (!addressList.empty()) addressList += ' ';
         addressList += addrStr;
      }
      lastIfIndex = ifIdx;
      lastFamily = item.Address->sa_family;
   }
   if (!addressList.empty()) addString(addressKey, addressList);

   std::string netifList;
   for (size_t i = 0; i < netifIndices.size(); ++i) {
      if (i > 0) netifList += ' ';
      netifList += std::to_string(netifIndices[i]);
   }
   addString("netif_list", netifList);

   freeifaddrs(ifaddr);
}
#else
// ##### Obtain network information on Windows systems ######################
void SystemInfo::obtainNetworkInformation(const bool) {}
#endif


// ##### Obtain battery information #########################################
void SystemInfo::obtainBatteryInformation()
{
   std::vector<unsigned int> batteryIDs;
#if defined(__linux__)
   for (unsigned int i = 0; i < 2; ++i) {
      std::ifstream capFile(std::format("/sys/class/power_supply/BAT{}/capacity", i));
      unsigned int cap = 0;
      if (capFile >> cap) {
         std::ifstream statFile(std::format("/sys/class/power_supply/BAT{}/status", i));
         std::string statusStr;
         std::getline(statFile, statusStr);
         unsigned int status = 0;
         if (statusStr == "Not charging") status = 1;
         else if (statusStr == "Charging") status = 2;
         else if (statusStr == "Full") status = 3;
         else if (statusStr == "Discharging") status = 4;

         addUInt32(std::format("battery_{}_status", i), status);
         addUInt32(std::format("battery_{}_capacity", i), cap);
         batteryIDs.push_back(i);
      }
   }
#endif
   std::string batList;
   for (size_t i = 0; i < batteryIDs.size(); ++i) {
      if (i > 0) batList += ' ';
      batList += std::to_string(batteryIDs[i]);
   }
   addString("battery_list", batList);
}
