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

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
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
#elif defined(__NetBSD__)
#include <sys/sysctl.h>
#include <utmpx.h>
#elif defined(__OpenBSD__)
#include <sys/sysctl.h>
#include <utmp.h>
#elif defined(__sun__)
#include <dirent.h>
#include <sys/loadavg.h>
#include <utmpx.h>
#elif defined(__gnu_hurd__)
#include <dirent.h>
#include <utmpx.h>
#elif defined(__APPLE__)
#include <libproc.h>
#include <sys/sysctl.h>
#include <utmpx.h>
#elif defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <wtsapi32.h>
#endif


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
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__gnu_hurd__) || defined(__APPLE__)
   struct utsname kernelInfo;
   if(uname(&kernelInfo) >= 0) {
      addString("system_sysname",  kernelInfo.sysname);
      addString("system_nodename", kernelInfo.nodename);
      addString("system_release",  kernelInfo.release);
      addString("system_version",  kernelInfo.version);
      addString("system_machine",  kernelInfo.machine);
   }
#elif defined(_WIN32)
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
#else
#error Missing case!
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
#error Missing case!
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
         count = static_cast<unsigned int>(parameterLength / sizeof(struct kinfo_proc));
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
         count = static_cast<unsigned int>(parameterLength / sizeof(struct kinfo_proc2));
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
         count = static_cast<unsigned int>(parameterLength / sizeof(struct kinfo_proc));
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
#error Missing case!
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
            std::free(className);
         }
         std::free(sessions[i]);
      }
      std::free(sessions);
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
#error Missing case!
#endif
   return count;
}


// ##### Obtain load information ############################################
void SystemInfo::obtainLoadInformation()
{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__gnu_hurd__) || defined(__APPLE__)
   const auto cores    = static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));
   const auto pageSize = static_cast<uint32_t>(sysconf(_SC_PAGESIZE));
#elif defined(_WIN32)
   SYSTEM_INFO sysInfo;
   GetSystemInfo(&sysInfo);
   const auto cores    = static_cast<uint32_t>(sysInfo.dwNumberOfProcessors);
   const auto pageSize = static_cast<uint32_t>(sysInfo.dwPageSize);
#else
#error Missing case!
#endif
   addUInt32("system_cores", cores);
   addUInt32("system_pagesize", pageSize);
   addUInt32("system_procs", getProcessCount());
   addUInt32("system_users", getUserCount());

#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__) || defined(__gnu_hurd__) || defined(__APPLE__)
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
#elif defined(_WIN32)
   // Load average is not supported on Windows
#else
#error Missing case!
#endif
}
