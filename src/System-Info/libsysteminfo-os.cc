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
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#pragma comment(lib, "advapi32.lib")
#endif



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
