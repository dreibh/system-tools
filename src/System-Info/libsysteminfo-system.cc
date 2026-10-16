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
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
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
#elif defined(__sun)
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
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "advapi32.lib")
#endif


// ##### Obtain hostname information ########################################
void SystemInfo::obtainHostnameInformation()
{
   char hostname[256] { 0 };
   std::string longHost = (gethostname(hostname, sizeof(hostname)) == 0)
                             ? hostname
                             : "localhost";
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


// ###### Get the system uptime #############################################
static bool getUptime(struct timespec* ts)
{
#if defined(__linux__) || defined(__OpenBSD__)
   return clock_gettime(CLOCK_BOOTTIME, ts) == 0;
#elif defined(__FreeBSD__)
   return clock_gettime(CLOCK_UPTIME_PRECISE, ts) == 0;
#elif defined(__NetBSD__) || defined(__sun) || defined(__gnu_hurd__) || defined(__APPLE__)
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


// ###### Obtain operating system release details ###########################
void SystemInfo::obtainOperatingSystemInformation()
{
   std::string osName;
   std::string osVersion;
   std::string osVersionId;
   std::string osId;
   std::string osVersionCodename;

   std::ifstream osReleaseStream("/etc/os-release");
   if(!osReleaseStream.is_open()) {
      osReleaseStream.open("/usr/lib/os-release");
   }

   if(osReleaseStream.is_open()) {
      std::string line;
      while(std::getline(osReleaseStream, line)) {
         const auto equalsPosition = line.find('=');
         if(equalsPosition == std::string::npos) {
            continue;
         }
         const std::string key   = line.substr(0, equalsPosition);
         std::string       value = line.substr(equalsPosition + 1);

         if((value.length() >= 2) &&
            (((value.front() == '"') && (value.back() == '"')) ||
             ((value.front() == '\'') && (value.back() == '\'')))) {
            value = value.substr(1, value.length() - 2);
         }

         if(key == "NAME") {
            osName = value;
         }
         else if(key == "VERSION") {
            osVersion = value;
            if(osVersionCodename.empty()) {
               const auto openParenPosition  = value.rfind('(');
               const auto closeParenPosition = value.rfind(')');
               if((openParenPosition != std::string::npos) &&
                  (closeParenPosition != std::string::npos) &&
                  (closeParenPosition > openParenPosition)) {
                  osVersionCodename = value.substr(openParenPosition + 1,
                                                   closeParenPosition - openParenPosition - 1);
               }
            }
         }
         else if(key == "VERSION_ID") {
            osVersionId = value;
         }
         else if(key == "ID") {
            osId = value;
         }
         else if(key == "VERSION_CODENAME") {
            osVersionCodename = value;
         }
      }
   }
   else {
#if defined(__APPLE__)
      osName = "macOS";
      osId   = "macos";

      FILE* pipe = popen("sw_vers -productVersion", "r");
      if(pipe != nullptr) {
         char buffer[128] { 0x00 };
         if(std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string versionString(buffer);
            while(!versionString.empty() && ((versionString.back() == '\r') || (versionString.back() == '\n'))) {
               versionString.pop_back();
            }
            osVersion   = versionString;
            osVersionId = versionString;
         }
         pclose(pipe);
      }

      int major = 0;
      if(!osVersion.empty()) {
         const auto dotPos = osVersion.find('.');
         const size_t len  = (dotPos != std::string::npos) ? dotPos : osVersion.size();
         std::from_chars(osVersion.data(), osVersion.data() + len, major);
      }

      // Actively supported releases
      switch(major) {
         case 14: osVersionCodename = "Sonoma";      break;
         case 15: osVersionCodename = "Sequoia";     break;
         case 16: // Fallthrough in case of legacy minor-bump builds
         case 26: osVersionCodename = "Tahoe";       break;
         case 27: osVersionCodename = "Golden Gate"; break;
         default: break;
      }

#elif defined(_WIN32)
      osId = "windows";

      RTL_OSVERSIONINFOEXW rovi { sizeof(rovi) };
      HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
      if(hNtdll) {
         using fnRtlGetVersion = LONG(WINAPI*)(PRTL_OSVERSIONINFOEXW);
         auto pRtlGetVersion = reinterpret_cast<fnRtlGetVersion>(GetProcAddress(hNtdll, "RtlGetVersion"));
         if(pRtlGetVersion) {
            pRtlGetVersion(&rovi);
         }
      }

      const bool isServer = (rovi.wProductType != VER_NT_WORKSTATION);
      DWORD build = rovi.dwBuildNumber;

      char displayVersion[64] { 0 };
      DWORD dispSize = sizeof(displayVersion);
      HKEY hKey = nullptr;
      if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                       0, KEY_READ, &hKey) == ERROR_SUCCESS) {
         if(RegQueryValueExA(hKey, "DisplayVersion", nullptr, nullptr,
                             reinterpret_cast<LPBYTE>(displayVersion), &dispSize) != ERROR_SUCCESS) {
            dispSize = sizeof(displayVersion);
            if(RegQueryValueExA(hKey, "ReleaseId", nullptr, nullptr,
                                reinterpret_cast<LPBYTE>(displayVersion), &dispSize) != ERROR_SUCCESS) {
               dispSize = sizeof(displayVersion);
               RegQueryValueExA(hKey, "CSDVersion", nullptr, nullptr,
                                reinterpret_cast<LPBYTE>(displayVersion), &dispSize);
            }
         }

         if(build == 0) {
            char buildBuffer[32] { 0 };
            DWORD bSize = sizeof(buildBuffer);
            if(RegQueryValueExA(hKey, "CurrentBuildNumber", nullptr, nullptr,
                                reinterpret_cast<LPBYTE>(buildBuffer), &bSize) == ERROR_SUCCESS) {
               build = std::strtoul(buildBuffer, nullptr, 10);
            }
         }
         RegCloseKey(hKey);
      }

      if(isServer) {
         osName = "Windows Server";
         if(build >= 26100) {
            osVersion = "2025";
         }
         else if(build >= 20348) {
            osVersion = "2022";
         }
         else if(build >= 17763) {
            osVersion = "2019";
         }
         else if(build >= 14393) {
            osVersion = "2016";
         }
         else {
            osVersion = "Server";
         }
      }
      else {
         osName = "Windows";
         if(build >= 22000) {
            osVersion = "11";
         }
         else if(build >= 10240) {
            osVersion = "10";
         }
         else {
            osVersion = getString("system_release");
         }
      }

      osVersionId       = osVersion;
      osVersionCodename = displayVersion;

#else
      // Legacy Linux / BSD: Try lsb_release if present
      FILE* pipe = popen("lsb_release -is 2>/dev/null", "r");
      if(pipe != nullptr) {
         char buffer[128] { 0x00 };
         if(std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string releaseName(buffer);
            while(!releaseName.empty() && ((releaseName.back() == '\r') || (releaseName.back() == '\n'))) {
               releaseName.pop_back();
            }
            osName = releaseName;
         }
         pclose(pipe);
      }

      pipe = popen("lsb_release -rs 2>/dev/null", "r");
      if(pipe != nullptr) {
         char buffer[128] { 0x00 };
         if(std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string releaseVersion(buffer);
            while(!releaseVersion.empty() && ((releaseVersion.back() == '\r') || (releaseVersion.back() == '\n'))) {
               releaseVersion.pop_back();
            }
            osVersionId = releaseVersion;
            osVersion   = releaseVersion;
         }
         pclose(pipe);
      }

      pipe = popen("lsb_release -cs 2>/dev/null", "r");
      if(pipe != nullptr) {
         char buffer[128] { 0x00 };
         if(std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string codename(buffer);
            while(!codename.empty() && ((codename.back() == '\r') || (codename.back() == '\n'))) {
               codename.pop_back();
            }
            osVersionCodename = codename;
         }
         pclose(pipe);
      }

      // Kernel fallbacks if lsb_release is unavailable
      if(osName.empty()) {
         osName      = getString("system_sysname", "Unknown");
         osVersion   = getString("system_release");
         osVersionId = osVersion;
      }

      if(!osName.empty() && osId.empty()) {
         osId = osName;
         std::transform(osId.begin(), osId.end(), osId.begin(),
                        [](const unsigned char character) {
                           return static_cast<char>(std::tolower(character));
                        });
      }
#endif
   }

   addString("os_name", osName);
   addString("os_version", osVersion);
   addString("os_version_id", osVersionId);
   addString("os_id", osId);
   addString("os_version_codename", osVersionCodename);
}


// ###### Obtain kernel information #########################################
void SystemInfo::obtainKernelInformation()
{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun) || defined(__gnu_hurd__) || defined(__APPLE__)
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


// ###### Obtain the number of processes on the system ######################
static unsigned int getProcessCount()
{
   unsigned int count = 0;

#if defined(__linux__) || defined(__sun) || defined(__gnu_hurd__)
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

#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__sun) || defined(__gnu_hurd__) || defined(__APPLE__)
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
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun) || defined(__gnu_hurd__) || defined(__APPLE__)
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

#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun) || defined(__gnu_hurd__) || defined(__APPLE__)
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


// ###### Obtain SSH key fingerprints #######################################
void SystemInfo::obtainSshKeyFingerprints()
{
   std::string     sshDirectory;
   std::error_code errorCode;

#if defined(_WIN32)
   char         programData[MAX_PATH] { 0 };
   const DWORD length = GetEnvironmentVariableA("ProgramData", programData, sizeof(programData));
   if((length > 0) && (length < sizeof(programData))) {
      const std::string candidate = std::string(programData) + "\\ssh";
      if(std::filesystem::is_directory(candidate, errorCode)) {
         sshDirectory = candidate;
      }
   }
   if(sshDirectory.empty()) {
      if(std::filesystem::is_directory("C:\\ProgramData\\ssh", errorCode)) {
         sshDirectory = "C:\\ProgramData\\ssh";
      }
   }
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun) || defined(__gnu_hurd__) || defined(__APPLE__)
   if(std::filesystem::is_directory("/etc/ssh", errorCode)) {
      sshDirectory = "/etc/ssh";
   }
#else
#error Missing case!
#endif

   if(sshDirectory.empty()) {
      return;
   }

   std::vector<std::string> keyFiles;
   for(const auto& directoryEntry : std::filesystem::directory_iterator(sshDirectory, errorCode)) {
      if(directoryEntry.is_regular_file(errorCode)) {
         const std::string filename = directoryEntry.path().filename().string();
         if((filename.rfind("ssh_host_", 0) == 0) &&
            (filename.length() >= 4) &&
            (filename.substr(filename.length() - 4) == ".pub") &&
            (filename.find("_dsa_key.pub") == std::string::npos)) {
            keyFiles.push_back(directoryEntry.path().string());
         }
      }
   }
   std::sort(keyFiles.begin(), keyFiles.end());

   const std::regex keyRegex(R"(^([0-9]+)\s+([^ ]+)\s+.*?\s+\((.*?)\)$)");
   unsigned int     keyNumber = 1;
   std::string      keyList;

   for(const auto& keyPath : keyFiles) {
      const std::string command = "ssh-keygen -lf \"" + keyPath + "\"";
#if !defined(_WIN32)
      FILE* pipe = popen(command.c_str(), "r");
#else
      FILE* pipe = _popen(command.c_str(), "r");
#endif
      if(pipe == nullptr) {
         continue;
      }

      char lineBuffer[512] { 0x00 };
      if(std::fgets(lineBuffer, sizeof(lineBuffer), pipe) != nullptr) {
         std::string line(lineBuffer);
         while(!line.empty() && ((line.back() == '\r') || (line.back() == '\n'))) {
            line.pop_back();
         }

         std::smatch match;
         if(std::regex_match(line, match, keyRegex) && (match.size() == 4)) {
            const std::string bitsString  = match[1].str();
            const std::string fingerprint = match[2].str();
            const std::string keyType     = match[3].str();

            if(!fingerprint.empty() && std::isupper(static_cast<unsigned char>(fingerprint[0]))) {
               const std::string indexString = std::to_string(keyNumber);

               addString("ssh_key_fingerprint_" + indexString, fingerprint);
               addString("ssh_key_type_" + indexString, keyType);

               char* endPointer = nullptr;
               const unsigned long bits = std::strtoul(bitsString.c_str(), &endPointer, 10);
               if((endPointer != bitsString.c_str()) && (*endPointer == 0x00)) {
                  addUInt32("ssh_key_bits_" + indexString, static_cast<uint32_t>(bits));
               }

               if(keyNumber == 1) {
                  addString("ssh_key_type", keyType);
               }

               if(!keyList.empty()) {
                  keyList += " ";
               }
               keyList += indexString;

               keyNumber++;
            }
         }
      }
#if !defined(_WIN32)
      pclose(pipe);
#else
      _pclose(pipe);
#endif
   }

   if(!keyList.empty()) {
      addString("ssh_key_list", keyList);
   }
}
