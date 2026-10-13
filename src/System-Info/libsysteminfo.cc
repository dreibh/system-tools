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
#elif defined(__gnu_hurd__)
   else if (address->sa_family == ARPHRD_ETHER) {
      const auto* lladdr = reinterpret_cast<const uint8_t*>(&address->sa_data);
      std::string res;
      for (unsigned int i = 0; i < 6; ++i) {
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


#if !(defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__gnu_hurd__) || defined(__APPLE__) || defined(_WIN32))
// ###### Query information via shell #######################################
static bool queryPipe(const char* command, char* result, size_t resultMaxSize)
{
   FILE* fh = popen(command, "r");
   if (fh != nullptr) {
      size_t resultSize = 0;
      char*  resultPtr;
      do {
         resultPtr = fgets(&result[resultSize], resultMaxSize - resultSize, fh);
         if (resultPtr == nullptr) break;
         resultSize += strlen(resultPtr);
      } while (resultSize < resultMaxSize - 1);
      result[resultSize] = 0x00;
      pclose(fh);
      return (resultSize > 0);
   }
   return false;
}
#endif


// ###### Obtain the number of processes on the system ######################
static unsigned int getProcessCount()
{
   unsigned int count = 0;

#if defined(__linux__) || defined(__sun__) || defined(__gnu_hurd__)
   if (DIR* dir = opendir("/proc"); dir != nullptr) {
      while (dirent* entry = readdir(dir)) {
         if (std::isdigit(static_cast<unsigned char>(entry->d_name[0]))) count++;
      }
      closedir(dir);
   }
#elif defined(__FreeBSD__)
   const int mibKernProcProc[3] = { CTL_KERN, KERN_PROC, KERN_PROC_PROC };
   size_t parameterLength = 0;
   if (sysctl(mibKernProcProc, 3, nullptr, &parameterLength, nullptr, 0) == 0) {
      parameterLength = (parameterLength * 5) / 4;
      std::vector<char> buffer(parameterLength);
      if (sysctl(mibKernProcProc, 3, buffer.data(), &parameterLength, nullptr, 0) == 0) {
         count = parameterLength / sizeof(struct kinfo_proc);
      }
   }
#elif defined(__NetBSD__)
   int mibKernProcProc[6] = { CTL_KERN, KERN_PROC2, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc2), 0 };
   size_t parameterLength = 0;
   if (sysctl(mibKernProcProc, 6, nullptr, &parameterLength, nullptr, 0) == 0) {
      parameterLength = (parameterLength * 5) / 4;
      std::vector<char> buffer(parameterLength);
      mibKernProcProc[5] = static_cast<int>(parameterLength / sizeof(struct kinfo_proc2));
      if (sysctl(mibKernProcProc, 6, buffer.data(), &parameterLength, nullptr, 0) == 0) {
         count = parameterLength / sizeof(struct kinfo_proc2);
      }
   }
#elif defined(__OpenBSD__)
   int mibKernProcProc[6] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc), 0 };
   size_t parameterLength = 0;
   if (sysctl(mibKernProcProc, 6, nullptr, &parameterLength, nullptr, 0) == 0) {
      parameterLength = (parameterLength * 5) / 4;
      std::vector<char> buffer(parameterLength);
      mibKernProcProc[5] = static_cast<int>(parameterLength / sizeof(struct kinfo_proc));
      if (sysctl(mibKernProcProc, 6, buffer.data(), &parameterLength, nullptr, 0) == 0) {
         count = parameterLength / sizeof(struct kinfo_proc);
      }
   }
#elif defined(__APPLE__)
   const int bufferSize = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
   if (bufferSize > 0) {
      count = static_cast<unsigned int>(bufferSize) / sizeof(pid_t);
   }
#elif defined(_WIN32)
   HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if (hSnap != INVALID_HANDLE_VALUE) {
      PROCESSENTRY32 pe32{ sizeof(PROCESSENTRY32) };
      if (Process32First(hSnap, &pe32)) {
         do { count++; } while (Process32Next(hSnap, &pe32));
      }
      CloseHandle(hSnap);
   }
#else
   char buffer[64];
   unsigned int value;
   if (queryPipe("ps -ae -o pid= | wc -l", buffer, sizeof(buffer)) && (sscanf(buffer, "%u", &value) == 1)) {
      count = value;
   }
#endif
   return count;
}


// ###### Obtain the number of users on the system ##########################
static unsigned int getUserCount()
{
   unsigned int count = 0;

#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__sun__) || defined(__gnu_hurd__) || defined(__APPLE__)
#if defined(ENABLE_SYSTEMD)
   char** sessions = nullptr;
   int totalSessions = sd_get_sessions(&sessions);
   if (totalSessions >= 0) {
      for (int i = 0; i < totalSessions; ++i) {
         char* className = nullptr;
         if (sd_session_get_class(sessions[i], &className) >= 0) {
            if (className && std::strcmp(className, "user") == 0) count++;
            free(className);
         }
         free(sessions[i]);
      }
      free(sessions);
   }
#else
   setutxent();
   while (utmpx* utx = getutxent()) {
      if (utx->ut_type == USER_PROCESS) count++;
   }
   endutxent();
#endif
#elif defined(__OpenBSD__)
   std::ifstream fh("/var/run/utmp", std::ios::binary);
   struct utmp ut{};
   while (fh.read(reinterpret_cast<char*>(&ut), sizeof(ut))) {
      if (ut.ut_name[0] != 0x00) count++;
   }
#elif defined(_WIN32)
   PWTS_SESSION_INFOA pSessionInfo = nullptr;
   DWORD dwCount = 0;
   if (WTSEnumerateSessionsA(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwCount)) {
      for (DWORD i = 0; i < dwCount; i++) {
         if ((pSessionInfo[i].State == WTSActive) || (pSessionInfo[i].State == WTSConnected)) {
            LPSTR pUserName = nullptr;
            DWORD bytesReturned = 0;
            if (WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE,
                                            pSessionInfo[i].SessionId,
                                            WTSUserName,
                                            &pUserName,
                                            &bytesReturned)) {
               if (pUserName && (pUserName[0] != 0x00)) count++;
               WTSFreeMemory(pUserName);
            }
         }
      }
      WTSFreeMemory(pSessionInfo);
   }
   if (count == 0) count = 1;
#else
   char buffer[64];
   unsigned int value;
   if (queryPipe("who | wc -l", buffer, sizeof(buffer)) && (sscanf(buffer, "%u", &value) == 1)) {
      count = value;
   }
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


#if defined(__sun__)
static bool getKstatUint64(const kstat_named_t* kn, uint64_t* val)
{
   switch (kn->data_type) {
      case KSTAT_DATA_UINT32:
         *val = static_cast<uint64_t>(kn->value.ui32);
         return true;
      case KSTAT_DATA_UINT64:
         *val = kn->value.ui64;
         return true;
      case KSTAT_DATA_INT32:
         *val = (kn->value.i32 >= 0) ? static_cast<uint64_t>(kn->value.i32) : 0;
         return true;
      case KSTAT_DATA_INT64:
         *val = (kn->value.i64 >= 0) ? static_cast<uint64_t>(kn->value.i64) : 0;
         return true;
      default:
         return false;
   }
}
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
#elif defined(__FreeBSD__)
   int acpiFD = open("/dev/acpi", O_RDONLY);
   if (acpiFD >= 0) {
      unsigned int batteryUnits = 0;
      if (ioctl(acpiFD, ACPIIO_BATT_GET_UNITS, &batteryUnits) == 0) {
         for (unsigned int i = 0; i < batteryUnits && i < 2; ++i) {
            union acpi_battery_ioctl_arg batteryInfo{};
            batteryInfo.unit = static_cast<int>(i);
            if (ioctl(acpiFD, ACPIIO_BATT_GET_BATTINFO, &batteryInfo) == 0) {
               unsigned int status = 0;
               if (batteryInfo.battinfo.state != ACPI_BATT_STAT_NOT_PRESENT) {
                  if (batteryInfo.battinfo.state == 0) status = 3;
                  else if (batteryInfo.battinfo.state & ACPI_BATT_STAT_CHARGING) status = 2;
                  else if (batteryInfo.battinfo.state & ACPI_BATT_STAT_DISCHARG) status = 4;
                  else status = 1;
               }
               addUInt32(std::format("battery_{}_status", i), status);
               addUInt32(std::format("battery_{}_capacity", i), static_cast<unsigned int>(batteryInfo.battinfo.cap));
               batteryIDs.push_back(i);
            }
         }
      }
      close(acpiFD);
   }
#elif defined(__OpenBSD__)
   int apmFD = open("/dev/apm", O_RDONLY);
   if (apmFD >= 0) {
      struct apm_power_info powerInfo{};
      if (ioctl(apmFD, APM_IOC_GETPOWER, &powerInfo) == 0) {
         if ((powerInfo.battery_life <= 100) &&
             (powerInfo.battery_state != APM_BATT_UNKNOWN) &&
             (powerInfo.battery_state != APM_BATTERY_ABSENT)) {
            unsigned int status = 0;
            if (powerInfo.battery_state == APM_BATT_CHARGING) status = 2;
            else if (powerInfo.battery_life == 100) status = 3;
            else if (powerInfo.ac_state == APM_AC_ON) status = 1;
            else if (powerInfo.ac_state == APM_AC_OFF) status = 4;

            addUInt32("battery_0_status", status);
            addUInt32("battery_0_capacity", powerInfo.battery_life);
            batteryIDs.push_back(0);
         }
      }
      close(apmFD);
   }
#elif defined(_WIN32)
   SYSTEM_POWER_STATUS sysStatus;
   if (GetSystemPowerStatus(&sysStatus) && sysStatus.BatteryFlag != 255) {
      unsigned int status = 0;
      if (sysStatus.ACLineStatus == 1) {
         status = (sysStatus.BatteryLifePercent >= 100) ? 3 : 2;
      } else if (sysStatus.ACLineStatus == 0) {
         status = 4;
      }
      unsigned int capacity = sysStatus.BatteryLifePercent;
      if (capacity > 100) capacity = 100;

      addUInt32("battery_0_status", status);
      addUInt32("battery_0_capacity", capacity);
      batteryIDs.push_back(0);
   }
#endif

   std::string batList;
   for (size_t i = 0; i < batteryIDs.size(); ++i) {
      if (i > 0) batList += ' ';
      batList += std::to_string(batteryIDs[i]);
   }
   addString("battery_list", batList);
}


#if defined(_WIN32)
struct WindowsSwapContext {
   unsigned long long Total{0};
   unsigned long long Used{0};
   unsigned long long PageSize{4096};
};

static BOOL CALLBACK enumPageFilesCallback(PVOID pContext, PENUM_PAGE_FILE_INFORMATION pCasInfo, LPCSTR)
{
   auto* ctx = reinterpret_cast<WindowsSwapContext*>(pContext);
   if (pCasInfo != nullptr) {
      ctx->Total += static_cast<unsigned long long>(pCasInfo->TotalSize) * ctx->PageSize;
      ctx->Used  += static_cast<unsigned long long>(pCasInfo->TotalInUse) * ctx->PageSize;
   }
   return TRUE;
}
#endif


// ##### Obtain memory information ##########################################
void SystemInfo::obtainMemoryInformation()
{
   uint64_t memoryTotal     = 0;
   uint64_t memoryAvailable = 0;
   uint64_t memoryUsed      = 0;
   uint64_t swapTotal       = 0;
   uint64_t swapAvailable   = 0;
   uint64_t swapUsed        = 0;

#if defined(__linux__) || defined(__gnu_hurd__)
   std::ifstream meminfo("/proc/meminfo");
   std::string line;
   while (std::getline(meminfo, line)) {
      uint64_t val = 0;
      if (sscanf(line.c_str(), "MemTotal: %llu", &val) == 1) memoryTotal = val * 1024;
      else if (sscanf(line.c_str(), "MemAvailable: %llu", &val) == 1) memoryAvailable = val * 1024;
#if defined(__gnu_hurd__)
      else if (sscanf(line.c_str(), "MemFree: %llu", &val) == 1) memoryAvailable = val * 1024;
#endif
      else if (sscanf(line.c_str(), "SwapTotal: %llu", &val) == 1) swapTotal = val * 1024;
      else if (sscanf(line.c_str(), "SwapFree: %llu", &val) == 1) swapAvailable = val * 1024;
   }
   memoryUsed = (memoryTotal >= memoryAvailable) ? (memoryTotal - memoryAvailable) : 0;
   swapUsed   = (swapTotal >= swapAvailable) ? (swapTotal - swapAvailable) : 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
   const int mibPageSize[2] = { CTL_HW, HW_PAGESIZE };
   unsigned int pageSize = 0;
   size_t len = sizeof(pageSize);
   sysctl(mibPageSize, 2, &pageSize, &len, nullptr, 0);

#if defined(__FreeBSD__)
   size_t physMem = 0;
   const int mibPhysMem[2] = { CTL_HW, HW_PHYSMEM };
#else
   uint64_t physMem = 0;
   const int mibPhysMem[2] = { CTL_HW, HW_PHYSMEM64 };
#endif
   len = sizeof(physMem);
   sysctl(mibPhysMem, 2, &physMem, &len, nullptr, 0);

   unsigned int vInactiveCount = 0;
   unsigned int vFreeCount = 0;
#if defined(__FreeBSD__)
   len = sizeof(vInactiveCount);
   sysctlbyname("vm.stats.vm.v_inactive_count", &vInactiveCount, &len, nullptr, 0);
   len = sizeof(vFreeCount);
   sysctlbyname("vm.stats.vm.v_free_count", &vFreeCount, &len, nullptr, 0);
#elif defined(__NetBSD__)
   struct uvmexp_sysctl uvm{};
   const int mibUvmExp[2] = { CTL_VM, VM_UVMEXP2 };
   len = sizeof(uvm);
   sysctl(mibUvmExp, 2, &uvm, &len, nullptr, 0);
   vInactiveCount = uvm.inactive;
   vFreeCount     = uvm.free;
#elif defined(__OpenBSD__)
   struct uvmexp uvm{};
   const int mibUvmExp[2] = { CTL_VM, VM_UVMEXP };
   len = sizeof(uvm);
   sysctl(mibUvmExp, 2, &uvm, &len, nullptr, 0);
   vInactiveCount = uvm.inactive;
   vFreeCount     = uvm.free;
#endif
   memoryTotal = physMem;
   memoryAvailable = (static_cast<uint64_t>(vInactiveCount) + vFreeCount) * pageSize;
   memoryUsed = (memoryTotal >= memoryAvailable) ? (memoryTotal - memoryAvailable) : 0;

#if defined(__NetBSD__) || defined(__OpenBSD__)
   const int numSwap = swapctl(SWAP_NSWAP, nullptr, 0);
   if (numSwap > 0) {
      std::vector<struct swapent> swapDevices(numSwap);
      int swapRecords = swapctl(SWAP_STATS, swapDevices.data(), numSwap);
      for (int i = 0; i < swapRecords; ++i) {
         if (swapDevices[i].se_flags & SWF_INUSE) {
            uint64_t totalBytes = static_cast<uint64_t>(swapDevices[i].se_nblks) * DEV_BSIZE;
            uint64_t usedBytes  = static_cast<uint64_t>(swapDevices[i].se_inuse) * DEV_BSIZE;
            swapTotal     += totalBytes;
            swapUsed      += usedBytes;
            swapAvailable += (totalBytes - usedBytes);
         }
      }
   }
#endif
#elif defined(__APPLE__)
   int64_t memSize = 0;
   size_t len = sizeof(memSize);
   if (sysctlbyname("hw.memsize", &memSize, &len, nullptr, 0) == 0) {
      memoryTotal = static_cast<uint64_t>(memSize);
   }
   mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
   vm_statistics64_data_t memoryStatistics{};
   vm_size_t pageSize = 0;
   host_page_size(mach_host_self(), &pageSize);
   if (host_statistics64(mach_host_self(), HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&memoryStatistics), &count) == KERN_SUCCESS) {
      memoryAvailable = (static_cast<uint64_t>(memoryStatistics.free_count) +
                         memoryStatistics.inactive_count +
                         memoryStatistics.speculative_count) * pageSize;
      memoryUsed = (memoryTotal >= memoryAvailable) ? (memoryTotal - memoryAvailable) : 0;
   }
   struct xsw_usage swapUsage{};
   len = sizeof(swapUsage);
   if (sysctlbyname("vm.swapusage", &swapUsage, &len, nullptr, 0) == 0) {
      swapTotal     = swapUsage.xsu_total;
      swapUsed      = swapUsage.xsu_used;
      swapAvailable = swapUsage.xsu_avail;
   }
#elif defined(_WIN32)
   MEMORYSTATUSEX statex{ sizeof(MEMORYSTATUSEX) };
   if (GlobalMemoryStatusEx(&statex)) {
      memoryTotal     = statex.ullTotalPhys;
      memoryAvailable = statex.ullAvailPhys;
      memoryUsed      = memoryTotal - memoryAvailable;

      SYSTEM_INFO sysInfo;
      GetSystemInfo(&sysInfo);
      WindowsSwapContext swapCtx{ .PageSize = sysInfo.dwPageSize ? sysInfo.dwPageSize : 4096 };
      if (EnumPageFilesA(enumPageFilesCallback, &swapCtx) && (swapCtx.Total > 0)) {
         swapTotal     = swapCtx.Total;
         swapUsed      = swapCtx.Used;
         swapAvailable = (swapTotal >= swapUsed) ? (swapTotal - swapUsed) : 0;
      }
   }
#endif

   addUInt64("system_mem_total", memoryTotal);
   addUInt64("system_mem_used",  memoryUsed);
   addUInt64("system_mem_free",  memoryAvailable);
   if (memoryTotal > 0) {
      addDouble("system_mem_usedpct", 100.0 * memoryUsed / memoryTotal, 1);
      addDouble("system_mem_freepct", 100.0 * memoryAvailable / memoryTotal, 1);
   }
   addUInt64("system_swap_total", swapTotal);
   addUInt64("system_swap_used",  swapUsed);
   addUInt64("system_swap_free",  swapAvailable);
   if (swapTotal > 0) {
      addDouble("system_swap_usedpct", 100.0 * swapUsed / swapTotal, 1);
      addDouble("system_swap_freepct", 100.0 * swapAvailable / swapTotal, 1);
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
#if defined(__linux__)
   if (a.Address->sa_family == AF_PACKET) {
      const auto* mac1 = reinterpret_cast<const sockaddr_ll*>(a.Address);
      const auto* mac2 = reinterpret_cast<const sockaddr_ll*>(b.Address);
      if (mac1->sll_halen != mac2->sll_halen) return mac1->sll_halen < mac2->sll_halen;
      return std::memcmp(mac1->sll_addr, mac2->sll_addr, mac1->sll_halen) < 0;
   }
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
   if (a.Address->sa_family == AF_LINK) {
      const auto* mac1 = reinterpret_cast<const sockaddr_dl*>(a.Address);
      const auto* mac2 = reinterpret_cast<const sockaddr_dl*>(b.Address);
      if (mac1->sdl_alen != mac2->sdl_alen) return mac1->sdl_alen < mac2->sdl_alen;
      return std::memcmp(LLADDR(mac1), LLADDR(mac2), mac1->sdl_alen) < 0;
   }
#endif
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

#if defined(__gnu_hurd__)
         int sd = socket(AF_INET, SOCK_DGRAM, 0);
         if (sd >= 0) {
            struct ifreq ifr{};
            std::strncpy(ifr.ifr_name, item.IfName.c_str(), IFNAMSIZ - 1);
            if (ioctl(sd, SIOCGIFHWADDR, &ifr) == 0 && ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
               addString(std::format("netif_{}_mac", ifIdx), formatAddress(&ifr.ifr_hwaddr, 48));
            }
            close(sd);
         }
#endif
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
void SystemInfo::obtainNetworkInformation(const bool filterLocalScope)
{
   ULONG outBufLen = 15360;
   auto* pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(std::malloc(outBufLen));
   if (pAddresses == nullptr) return;

   DWORD dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);
   if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
      std::free(pAddresses);
      pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(std::malloc(outBufLen));
      if (pAddresses == nullptr) return;
      dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);
   }
   if (dwRetVal != NO_ERROR) {
      std::free(pAddresses);
      return;
   }

   std::vector<PIP_ADAPTER_ADDRESSES> adapterArray;
   for (auto* pAdapter = pAddresses; pAdapter != nullptr; pAdapter = pAdapter->Next) {
      adapterArray.push_back(pAdapter);
   }

   std::sort(adapterArray.begin(), adapterArray.end(), [](PIP_ADAPTER_ADDRESSES a, PIP_ADAPTER_ADDRESSES b) {
      const DWORD idx1 = a->IfIndex ? a->IfIndex : a->Ipv6IfIndex;
      const DWORD idx2 = b->IfIndex ? b->IfIndex : b->Ipv6IfIndex;
      return idx1 < idx2;
   });

   std::string netifList;
   for (auto* pAdapter : adapterArray) {
      const DWORD ifIndex = pAdapter->IfIndex ? pAdapter->IfIndex : pAdapter->Ipv6IfIndex;
      if (ifIndex == 0) continue;

      if (filterLocalScope && (pAdapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)) continue;

      char friendlyName[256] = {0};
      if (pAdapter->FriendlyName != nullptr) {
         WideCharToMultiByte(CP_UTF8, 0, pAdapter->FriendlyName, -1, friendlyName, sizeof(friendlyName), nullptr, nullptr);
      }
      if (friendlyName[0] == 0x00 && pAdapter->AdapterName != nullptr) {
         std::strncpy(friendlyName, pAdapter->AdapterName, sizeof(friendlyName) - 1);
      }

      unsigned int flags = 0;
      if (pAdapter->OperStatus == IfOperStatusUp) {
         flags |= IFF_UP | IFF_RUNNING;
      }
      if (pAdapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) flags |= IFF_LOOPBACK;
      if ((pAdapter->IfType == IF_TYPE_PPP) || (pAdapter->IfType == IF_TYPE_TUNNEL)) flags |= IFF_POINTOPOINT;

      std::string ipv4List;
      std::string ipv6List;
      for (auto* pUnicast = pAdapter->FirstUnicastAddress; pUnicast != nullptr; pUnicast = pUnicast->Next) {
         if (pUnicast->Address.lpSockaddr == nullptr) continue;

         const auto* sa = pUnicast->Address.lpSockaddr;
         const auto prefixLen = static_cast<unsigned int>(pUnicast->OnLinkPrefixLength);

         if (sa->sa_family == AF_INET) {
            const auto* sin = reinterpret_cast<const sockaddr_in*>(sa);
            if (filterLocalScope && (ntohl(sin->sin_addr.s_addr) == INADDR_LOOPBACK)) continue;
            std::string addrStr = formatAddress(sa, prefixLen);
            if (!addrStr.empty()) {
               if (!ipv4List.empty()) ipv4List += ' ';
               ipv4List += addrStr;
            }
         } else if (sa->sa_family == AF_INET6) {
            const auto* sin6 = reinterpret_cast<const sockaddr_in6*>(sa);
            if (filterLocalScope && (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr) || IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))) continue;
            std::string addrStr = formatAddress(sa, prefixLen);
            if (!addrStr.empty()) {
               if (!ipv6List.empty()) ipv6List += ' ';
               ipv6List += addrStr;
            }
         }
      }

      std::string macAddress;
      if (pAdapter->PhysicalAddressLength > 0) {
         for (DWORD k = 0; k < pAdapter->PhysicalAddressLength; ++k) {
            macAddress += std::format("{}{:02x}", (k > 0) ? ":" : "", pAdapter->PhysicalAddress[k]);
         }
      }

      addString(std::format("netif_{}_name", ifIndex), friendlyName);
      addString(std::format("netif_{}_flags", ifIndex), formatFlags(flags));
      if (!macAddress.empty()) addString(std::format("netif_{}_mac", ifIndex), macAddress);
      if (!ipv4List.empty())   addString(std::format("netif_{}_ipv4", ifIndex), ipv4List);
      if (!ipv6List.empty())   addString(std::format("netif_{}_ipv6", ifIndex), ipv6List);

      if (!netifList.empty()) netifList += ' ';
      netifList += std::to_string(ifIndex);
   }

   addString("netif_list", netifList);
   std::free(pAddresses);
}
#endif
