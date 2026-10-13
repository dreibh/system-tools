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
#include <chrono>
#include <cstring>
#include <format>
#include <fstream>
#include <iomanip>
#include <vector>

#if !defined(_WIN32)
#include <sys/param.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif

#if defined(__linux__)
#include <dirent.h>
#include <sys/sysinfo.h>
#if defined(ENABLE_SYSTEMD)
#include <systemd/sd-login.h>
#else
#include <utmpx.h>
#endif
#elif defined(__FreeBSD__)
#include <sys/sysctl.h>
#include <sys/user.h>
#include <utmpx.h>
#include <vm/vm_param.h>
#elif defined(__NetBSD__)
#include <sys/ioctl.h>
#include <sys/swap.h>
#include <sys/sysctl.h>
#include <utmpx.h>
#include <uvm/uvm_extern.h>
#elif defined(__OpenBSD__)
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
#include <utmpx.h>
#elif defined(__APPLE__)
#include <libproc.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>
#include <utmpx.h>
#elif defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <wtsapi32.h>
#endif


// ###### Obtain SystemInfo #################################################
SystemInfo::SystemInfo(const unsigned int compatibilityVersion,
                       const uint32_t     flags)
   : CompatibilityVersion { compatibilityVersion }
{
   if(compatibilityVersion > LSI_COMPATIBILITY_VERSION) {
      throw SystemInfoException(std::format(
         "Requested compatibility version {} > available version {}!",
         compatibilityVersion, LSI_COMPATIBILITY_VERSION));
   }

   addUInt32("compatibility", compatibilityVersion);

   if(flags & Flags::Hostname) {
      obtainHostnameInformation();
   }
   if(flags & Flags::Uptime) {
      obtainUptimeInformation();
   }
   if(flags & Flags::Kernel) {
      obtainKernelInformation();
   }
   if(flags & Flags::Load) {
      obtainLoadInformation();
   }
   if(flags & Flags::Memory) {
      obtainMemoryInformation();
   }
   if(flags & Flags::Storage) {
      obtainStorageInformation();
   }
   if(flags & Flags::Network) {
      obtainNetworkInformation(true);
   }
   if(flags & Flags::Battery) {
      obtainBatteryInformation();
   }
}


// ##### Obtain SystemInfo ##################################################
void SystemInfo::print(std::ostream& os) const
{
   for(const auto& [key, entry] : Entries) {
      std::visit([&](const auto& value) {
         using Type = std::decay_t<decltype(value)>;
         if constexpr(std::is_same_v<Type, std::monostate>) {
            os << std::format("{}=(INVALID!)\n", key);
         }
         else if constexpr(std::is_same_v<Type, std::string>) {
            os << std::format("{}=\"{}\"\n", key, value);
         }
         else if constexpr(std::is_same_v<Type, double>) {
            if(entry.DisplayHint >= 0) {
               os << std::format("{}={:.{}f}\n", key, value, entry.DisplayHint);
            }
            else {
               os << std::format("{}={:f}\n", key, value);
            }
         }
         else {
            os << std::format("{}={}\n", key, value);
         }
      }, entry.Value);
   }
}


// ###### Add key with string value #########################################
void SystemInfo::addString(std::string_view key, std::string_view value)
{
   Entries[std::string(key)] = Entry {
      .Value       = std::string(value),
      .DisplayHint = 0
   };
}


// ###### Add key with 32-bit integer value #################################
void SystemInfo::addInt32(std::string_view key, const int32_t value)
{
   Entries[std::string(key)] = Entry {
      .Value       = value,
      .DisplayHint = 0
   };
}


// ###### Add key with 64-bit integer value #################################
void SystemInfo::addInt64(std::string_view key, const int64_t value)
{
   Entries[std::string(key)] = Entry {
      .Value       = value,
      .DisplayHint = 0
   };
}


// ###### Add key with 32-bit unsigned integer value ########################
void SystemInfo::addUInt32(std::string_view key, const uint32_t value)
{
   Entries[std::string(key)] = Entry {
      .Value       = value,
      .DisplayHint = 0
   };
}


// ###### Add key with 64-bit unsigned integer value ########################
void SystemInfo::addUInt64(std::string_view key, const uint64_t value)
{
   Entries[std::string(key)] = Entry {
      .Value       = value,
      .DisplayHint = 0
   };
}


// ###### Add key with double value #########################################
void SystemInfo::addDouble(std::string_view key,
                           const double     value,
                           const int        displayHint)
{
   Entries[std::string(key)] = Entry {
      .Value       = value,
      .DisplayHint = displayHint
   };
}


// ###### Check whether key exists (returns value type or SIET_INVALID) #####
SystemInfo::ValueType SystemInfo::exists(std::string_view key) const
{
   auto iterator = Entries.find(std::string(key));
   if(iterator == Entries.end()) {
      return ValueType::Invalid;
   }

   return std::visit([](const auto& value) -> ValueType {
      using Type = std::decay_t<decltype(value)>;
      if constexpr(std::is_same_v<Type, std::string>) {
         return ValueType::String;
      }
      if constexpr(std::is_same_v<Type, uint32_t>) {
         return ValueType::UInt32;
      }
      if constexpr(std::is_same_v<Type, uint64_t>) {
         return ValueType::UInt64;
      }
      if constexpr(std::is_same_v<Type, int32_t>) {
         return ValueType::Int32;
      }
      if constexpr(std::is_same_v<Type, int64_t>) {
         return ValueType::Int64;
      }
      if constexpr(std::is_same_v<Type, double>) {
         return ValueType::Double;
      }
      return ValueType::Invalid;
   }, iterator->second.Value);
}


// ###### Get string value, or default if not found #########################
std::string SystemInfo::getString(std::string_view key,
                                  std::string_view defaultValue) const
{
   if(auto iterator = Entries.find(std::string(key)); iterator != Entries.end()) {
      if(const auto* value = std::get_if<std::string>(&iterator->second.Value)) {
         return *value;
      }
   }
   return std::string(defaultValue);
}


// ###### Get 32-bit integer value, or default if not found #################
int32_t SystemInfo::getInt32(std::string_view key,
                            const int32_t    defaultValue) const
{
   if(auto iterator = Entries.find(std::string(key)); iterator != Entries.end()) {
      if(const auto* value = std::get_if<int32_t>(&iterator->second.Value)) {
         return *value;
      }
   }
   return defaultValue;
}


// ###### Get 64-bit integer value, or default if not found #################
int64_t SystemInfo::getInt64(std::string_view key,
                            const int64_t    defaultValue) const
{
   if(auto iterator = Entries.find(std::string(key)); iterator != Entries.end()) {
      if(const auto* value = std::get_if<int64_t>(&iterator->second.Value)) {
         return *value;
      }
   }
   return defaultValue;
}


// ###### Get 32-bit unsigned integer value, or default if not found ########
uint32_t SystemInfo::getUInt32(std::string_view key,
                              const uint32_t   defaultValue) const
{
   if(auto iterator = Entries.find(std::string(key)); iterator != Entries.end()) {
      if(const auto* value = std::get_if<uint32_t>(&iterator->second.Value)) {
         return *value;
      }
   }
   return defaultValue;
}


// ###### Get 64-bit unsigned integer value, or default if not found ########
uint64_t SystemInfo::getUInt64(std::string_view key,
                              const uint64_t   defaultValue) const
{
   if(auto iterator = Entries.find(std::string(key)); iterator != Entries.end()) {
      if(const auto* value = std::get_if<uint64_t>(&iterator->second.Value)) {
         return *value;
      }
   }
   return defaultValue;
}


// ###### Get double value, or default if not found #########################
double SystemInfo::getDouble(std::string_view key,
                             const double     defaultValue) const
{
   if(auto iterator = Entries.find(std::string(key)); iterator != Entries.end()) {
      if(const auto* value = std::get_if<double>(&iterator->second.Value)) {
         return *value;
      }
   }
   return defaultValue;
}


// ##### Obtain hostname information ########################################
void SystemInfo::obtainHostnameInformation()
{
   char hostname[256] { 0 };
   if(gethostname(hostname, sizeof(hostname)) != 0) {
      std::strncpy(hostname, "localhost", sizeof(hostname) - 1);
   }
   std::string longHost(hostname);
   addString("hostname_long", longHost);

   std::string shortHost = longHost;
   std::string domain;
   if(auto dot = longHost.find('.'); dot != std::string::npos) {
      shortHost = longHost.substr(0, dot);
      domain    = longHost.substr(dot + 1);
   }
   addString("domainname", domain);
   addString("hostname_short", shortHost);
}


// ###### Obtain kernel information #########################################
void SystemInfo::obtainKernelInformation()
{
#if !defined(_WIN32)
   struct utsname kernelInfo;
   if(uname(&kernelInfo) >= 0) {
      addString("system_sysname",  kernelInfo.sysname);
      addString("system_nodename", kernelInfo.nodename);
      addString("system_release",  kernelInfo.release);
      addString("system_version",  kernelInfo.version);
      addString("system_machine",  kernelInfo.machine);
   }
#else
   addString("system_sysname", "Windows");
   char  nodename[256] = "localhost";
   DWORD size          = sizeof(nodename);
   GetComputerNameA(nodename, &size);
   addString("system_nodename", nodename);

   std::string release = "10.0";
   std::string version = "Windows";
   HMODULE     hNtdll  = GetModuleHandleA("ntdll.dll");
   if(hNtdll) {
      using fnRtlGetVersion = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
      auto pRtlGetVersion = reinterpret_cast<fnRtlGetVersion>(GetProcAddress(hNtdll, "RtlGetVersion"));
      if(pRtlGetVersion) {
         RTL_OSVERSIONINFOW rovi { sizeof(rovi) };
         if(pRtlGetVersion(&rovi) == 0) {
            release = std::format("{}.{}", rovi.dwMajorVersion, rovi.dwMinorVersion);
            version = std::format("Windows Build {}", rovi.dwBuildNumber);
         }
      }
   }
   SYSTEM_INFO sysInfo;
   GetNativeSystemInfo(&sysInfo);
   const char* machine = "x86";
   if(sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
      machine = "x86_64";
   }
   else if(sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
      machine = "ARM64";
   }
   else if(sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) {
      machine = "ARM";
   }

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
   ts->tv_sec         = static_cast<time_t>(ms / 1000);
   ts->tv_nsec        = static_cast<long>((ms % 1000) * 1'000'000);
   return true;
#else
   return false;
#endif
}


// ##### Obtain uptime information ##########################################
void SystemInfo::obtainUptimeInformation()
{
   struct timespec ts { };
   if(getUptime(&ts)) {
      const auto totalSeconds = static_cast<uint64_t>(ts.tv_sec);
      const auto days         = static_cast<uint32_t>(totalSeconds / 86400);
      const auto hours        = static_cast<uint32_t>((totalSeconds / 3600) % 24);
      const auto minutes      = static_cast<uint32_t>((totalSeconds / 60) % 60);
      const auto seconds      = static_cast<uint32_t>(totalSeconds % 60);
      const auto nanoseconds  = static_cast<uint32_t>(ts.tv_nsec);

      addDouble("uptime_total", static_cast<double>(ts.tv_sec) + (static_cast<double>(ts.tv_nsec) / 1e9), 9);
      addUInt32("uptime_days",  days);
      addUInt32("uptime_hours", hours);
      addUInt32("uptime_mins",  minutes);
      addUInt32("uptime_secs",  seconds);
      addUInt32("uptime_nsecs", nanoseconds);
   }
}


#if !(defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__gnu_hurd__) || defined(__APPLE__) || defined(_WIN32))
// ###### Query information via shell #######################################
static bool queryPipe(const char*  command,
                      char*        result,
                      const size_t resultMaxSize)
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
      } while(resultSize < resultMaxSize - 1);
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
   if(DIR* dir = opendir("/proc"); dir != nullptr) {
      while(dirent* entry = readdir(dir)) {
         if(std::isdigit(static_cast<unsigned char>(entry->d_name[0]))) {
            count++;
         }
      }
      closedir(dir);
   }
#elif defined(__FreeBSD__)
   const int          mibKernProcProc[3]  = { CTL_KERN, KERN_PROC, KERN_PROC_PROC };
   const unsigned int mibKernProcProcSize = sizeof(mibKernProcProc) / sizeof(mibKernProcProc[0]);
   size_t             parameterLength     = 0;
   if(sysctl(mibKernProcProc, mibKernProcProcSize, nullptr, &parameterLength, nullptr, 0) == 0) {
      parameterLength = (parameterLength * 5) / 4;
      std::vector<char> buffer(parameterLength);
      if(sysctl(mibKernProcProc, mibKernProcProcSize, buffer.data(), &parameterLength, nullptr, 0) == 0) {
         count = parameterLength / sizeof(struct kinfo_proc);
      }
   }
#elif defined(__NetBSD__)
   int                mibKernProcProc[6]  = { CTL_KERN, KERN_PROC2, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc2), 0 };
   const unsigned int mibKernProcProcSize = sizeof(mibKernProcProc) / sizeof(mibKernProcProc[0]);
   size_t             parameterLength     = 0;
   if(sysctl(mibKernProcProc, mibKernProcProcSize, nullptr, &parameterLength, nullptr, 0) == 0) {
      parameterLength = (parameterLength * 5) / 4;
      std::vector<char> buffer(parameterLength);
      mibKernProcProc[5] = static_cast<int>(parameterLength / sizeof(struct kinfo_proc2));
      if(sysctl(mibKernProcProc, mibKernProcProcSize, buffer.data(), &parameterLength, nullptr, 0) == 0) {
         count = parameterLength / sizeof(struct kinfo_proc2);
      }
   }
#elif defined(__OpenBSD__)
   int                mibKernProcProc[6]  = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc), 0 };
   const unsigned int mibKernProcProcSize = sizeof(mibKernProcProc) / sizeof(mibKernProcProc[0]);
   size_t             parameterLength     = 0;
   if(sysctl(mibKernProcProc, mibKernProcProcSize, nullptr, &parameterLength, nullptr, 0) == 0) {
      parameterLength = (parameterLength * 5) / 4;
      std::vector<char> buffer(parameterLength);
      mibKernProcProc[5] = static_cast<int>(parameterLength / sizeof(struct kinfo_proc));
      if(sysctl(mibKernProcProc, mibKernProcProcSize, buffer.data(), &parameterLength, nullptr, 0) == 0) {
         count = parameterLength / sizeof(struct kinfo_proc);
      }
   }
#elif defined(__APPLE__)
   const int bufferSize = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
   if(bufferSize > 0) {
      count = static_cast<unsigned int>(bufferSize) / sizeof(pid_t);
   }
#elif defined(_WIN32)
   HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if(hSnap != INVALID_HANDLE_VALUE) {
      PROCESSENTRY32 pe32 { sizeof(PROCESSENTRY32) };
      if(Process32First(hSnap, &pe32)) {
         do {
            count++;
         } while(Process32Next(hSnap, &pe32));
      }
      CloseHandle(hSnap);
   }
#else
   char         buffer[64];
   unsigned int value;
   if(queryPipe("ps -ae -o pid= | wc -l", buffer, sizeof(buffer)) && (sscanf(buffer, "%u", &value) == 1)) {
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
   char** sessions      = nullptr;
   int    totalSessions = sd_get_sessions(&sessions);
   if(totalSessions >= 0) {
      for(int i = 0; i < totalSessions; ++i) {
         char* className = nullptr;
         if(sd_session_get_class(sessions[i], &className) >= 0) {
            if(className && (std::strcmp(className, "user") == 0)) {
               count++;
            }
            free(className);
         }
         free(sessions[i]);
      }
      free(sessions);
   }
#else
   setutxent();
   while(utmpx* utx = getutxent()) {
      if(utx->ut_type == USER_PROCESS) {
         count++;
      }
   }
   endutxent();
#endif
#elif defined(__OpenBSD__)
   std::ifstream fh("/var/run/utmp", std::ios::binary);
   struct utmp   ut { };
   while(fh.read(reinterpret_cast<char*>(&ut), sizeof(ut))) {
      if(ut.ut_name[0] != 0x00) {
         count++;
      }
   }
#elif defined(_WIN32)
   PWTS_SESSION_INFOA pSessionInfo = nullptr;
   DWORD              dwCount      = 0;
   if(WTSEnumerateSessionsA(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwCount)) {
      for(DWORD i = 0; i < dwCount; i++) {
         if((pSessionInfo[i].State == WTSActive) || (pSessionInfo[i].State == WTSConnected)) {
            LPSTR pUserName     = nullptr;
            DWORD bytesReturned = 0;
            if(WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE,
                                            pSessionInfo[i].SessionId,
                                            WTSUserName,
                                            &pUserName,
                                            &bytesReturned)) {
               if(pUserName && (pUserName[0] != 0x00)) {
                  count++;
               }
               WTSFreeMemory(pUserName);
            }
         }
      }
      WTSFreeMemory(pSessionInfo);
   }
   if(count == 0) {
      count = 1;
   }
#else
   char         buffer[64];
   unsigned int value;
   if(queryPipe("who | wc -l", buffer, sizeof(buffer)) && (sscanf(buffer, "%u", &value) == 1)) {
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
   if(getloadavg(loadavg, 3) == 3) {
      const double loadPercentageFactor = (cores > 0) ? (100.0 / static_cast<double>(cores)) : 0.0;
      addDouble("system_load_avg1min",     loadavg[0], 6);
      addDouble("system_load_avg5min",     loadavg[1], 6);
      addDouble("system_load_avg15min",    loadavg[2], 6);
      addDouble("system_load_avg1minpct",  loadavg[0] * loadPercentageFactor, 4);
      addDouble("system_load_avg5minpct",  loadavg[1] * loadPercentageFactor, 4);
      addDouble("system_load_avg15minpct", loadavg[2] * loadPercentageFactor, 4);
   }
#endif
}


#if defined(__sun__)
static bool getKstatUint64(const kstat_named_t* kn,
                           uint64_t*            value)
{
   switch(kn->data_type) {
      case KSTAT_DATA_UINT32:
         *value = static_cast<uint64_t>(kn->value.ui32);
         return true;
      case KSTAT_DATA_UINT64:
         *value = kn->value.ui64;
         return true;
      case KSTAT_DATA_INT32:
         *value = (kn->value.i32 >= 0) ? static_cast<uint64_t>(kn->value.i32) : 0;
         return true;
      case KSTAT_DATA_INT64:
         *value = (kn->value.i64 >= 0) ? static_cast<uint64_t>(kn->value.i64) : 0;
         return true;
      default:
         return false;
   }
}
#endif


#if defined(_WIN32)
struct WindowsSwapContext {
   unsigned long long Total { 0 };
   unsigned long long Used { 0 };
   unsigned long long PageSize { 4096 };
};

static BOOL CALLBACK enumPageFilesCallback(PVOID                       pContext,
                                           PENUM_PAGE_FILE_INFORMATION pCasInfo,
                                           LPCSTR)
{
   auto* context = reinterpret_cast<WindowsSwapContext*>(pContext);
   if(pCasInfo != nullptr) {
      context->Total += static_cast<unsigned long long>(pCasInfo->TotalSize) * context->PageSize;
      context->Used  += static_cast<unsigned long long>(pCasInfo->TotalInUse) * context->PageSize;
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
   std::string   line;
   while(std::getline(meminfo, line)) {
      unsigned long long value = 0;
      if(sscanf(line.c_str(), "MemTotal: %llu", &value) == 1) {
         memoryTotal = value * 1024;
      }
      else if(sscanf(line.c_str(), "MemAvailable: %llu", &value) == 1) {
         memoryAvailable = value * 1024;
      }
#if defined(__gnu_hurd__)
      else if(sscanf(line.c_str(), "MemFree: %llu", &value) == 1) {
         memoryAvailable = value * 1024;
      }
#endif
      else if(sscanf(line.c_str(), "SwapTotal: %llu", &value) == 1) {
         swapTotal = value * 1024;
      }
      else if(sscanf(line.c_str(), "SwapFree: %llu", &value) == 1) {
         swapAvailable = value * 1024;
      }
   }
   memoryUsed = (memoryTotal >= memoryAvailable) ? (memoryTotal - memoryAvailable) : 0;
   swapUsed   = (swapTotal >= swapAvailable) ? (swapTotal - swapAvailable) : 0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
   const int    mibPageSize[2] = { CTL_HW, HW_PAGESIZE };
   unsigned int pageSize       = 0;
   size_t       length         = sizeof(pageSize);
   sysctl(mibPageSize, 2, &pageSize, &length, nullptr, 0);

#if defined(__FreeBSD__)
   size_t    physMem       = 0;
   const int mibPhysMem[2] = { CTL_HW, HW_PHYSMEM };
#else
   uint64_t  physMem       = 0;
   const int mibPhysMem[2] = { CTL_HW, HW_PHYSMEM64 };
#endif
   length = sizeof(physMem);
   sysctl(mibPhysMem, 2, &physMem, &length, nullptr, 0);

   unsigned int vInactiveCount = 0;
   unsigned int vFreeCount     = 0;
#if defined(__FreeBSD__)
   length = sizeof(vInactiveCount);
   sysctlbyname("vm.stats.vm.v_inactive_count", &vInactiveCount, &length, nullptr, 0);
   length = sizeof(vFreeCount);
   sysctlbyname("vm.stats.vm.v_free_count", &vFreeCount, &length, nullptr, 0);
#elif defined(__NetBSD__)
   struct uvmexp_sysctl uvm { };
   const int            mibUvmExp[2] = { CTL_VM, VM_UVMEXP2 };
   length = sizeof(uvm);
   sysctl(mibUvmExp, 2, &uvm, &length, nullptr, 0);
   vInactiveCount = uvm.inactive;
   vFreeCount     = uvm.free;
#elif defined(__OpenBSD__)
   struct uvmexp uvm { };
   const int     mibUvmExp[2] = { CTL_VM, VM_UVMEXP };
   length = sizeof(uvm);
   sysctl(mibUvmExp, 2, &uvm, &length, nullptr, 0);
   vInactiveCount = uvm.inactive;
   vFreeCount     = uvm.free;
#endif
   memoryTotal     = physMem;
   memoryAvailable = (static_cast<uint64_t>(vInactiveCount) + vFreeCount) * pageSize;
   memoryUsed      = (memoryTotal >= memoryAvailable) ? (memoryTotal - memoryAvailable) : 0;

#if defined(__NetBSD__) || defined(__OpenBSD__)
   const int numSwap = swapctl(SWAP_NSWAP, nullptr, 0);
   if(numSwap > 0) {
      std::vector<struct swapent> swapDevices(numSwap);
      const int swapRecords = swapctl(SWAP_STATS, swapDevices.data(), numSwap);
      for(int i = 0; i < swapRecords; ++i) {
         if(swapDevices[i].se_flags & SWF_INUSE) {
            const uint64_t totalBytes = static_cast<uint64_t>(swapDevices[i].se_nblks) * DEV_BSIZE;
            const uint64_t usedBytes  = static_cast<uint64_t>(swapDevices[i].se_inuse) * DEV_BSIZE;
            swapTotal     += totalBytes;
            swapUsed      += usedBytes;
            swapAvailable += (totalBytes - usedBytes);
         }
      }
   }
#endif
#elif defined(__APPLE__)
   int64_t memSize = 0;
   size_t  length  = sizeof(memSize);
   if(sysctlbyname("hw.memsize", &memSize, &length, nullptr, 0) == 0) {
      memoryTotal = static_cast<uint64_t>(memSize);
   }
   mach_msg_type_number_t count          = HOST_VM_INFO64_COUNT;
   vm_statistics64_data_t memoryStatistics { };
   vm_size_t              pageSize       = 0;
   host_page_size(mach_host_self(), &pageSize);
   if(host_statistics64(mach_host_self(), HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&memoryStatistics), &count) == KERN_SUCCESS) {
      memoryAvailable = (static_cast<uint64_t>(memoryStatistics.free_count) +
                         memoryStatistics.inactive_count +
                         memoryStatistics.speculative_count) * pageSize;
      memoryUsed      = (memoryTotal >= memoryAvailable) ? (memoryTotal - memoryAvailable) : 0;
   }
   struct xsw_usage swapUsage { };
   length = sizeof(swapUsage);
   if(sysctlbyname("vm.swapusage", &swapUsage, &length, nullptr, 0) == 0) {
      swapTotal     = swapUsage.xsu_total;
      swapUsed      = swapUsage.xsu_used;
      swapAvailable = swapUsage.xsu_avail;
   }
#elif defined(_WIN32)
   MEMORYSTATUSEX statex { sizeof(MEMORYSTATUSEX) };
   if(GlobalMemoryStatusEx(&statex)) {
      memoryTotal     = statex.ullTotalPhys;
      memoryAvailable = statex.ullAvailPhys;
      memoryUsed      = memoryTotal - memoryAvailable;

      SYSTEM_INFO sysInfo;
      GetSystemInfo(&sysInfo);
      WindowsSwapContext swapContext {
         .Total    = 0,
         .Used     = 0,
         .PageSize = sysInfo.dwPageSize ? sysInfo.dwPageSize : 4096
      };
      if(EnumPageFilesA(enumPageFilesCallback, &swapContext) && (swapContext.Total > 0)) {
         swapTotal     = swapContext.Total;
         swapUsed      = swapContext.Used;
         swapAvailable = (swapTotal >= swapUsed) ? (swapTotal - swapUsed) : 0;
      }
   }
#endif

   addUInt64("system_mem_total", memoryTotal);
   addUInt64("system_mem_used",  memoryUsed);
   addUInt64("system_mem_free",  memoryAvailable);
   if(memoryTotal > 0) {
      addDouble("system_mem_usedpct", 100.0 * memoryUsed / memoryTotal, 1);
      addDouble("system_mem_freepct", 100.0 * memoryAvailable / memoryTotal, 1);
   }
   addUInt64("system_swap_total", swapTotal);
   addUInt64("system_swap_used",  swapUsed);
   addUInt64("system_swap_free",  swapAvailable);
   if(swapTotal > 0) {
      addDouble("system_swap_usedpct", 100.0 * swapUsed / swapTotal, 1);
      addDouble("system_swap_freepct", 100.0 * swapAvailable / swapTotal, 1);
   }
}
