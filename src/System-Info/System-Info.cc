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
// System-Info
// Copyright (C) 2013-2026 by Thomas Dreibholz
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

#include <algorithm>
#include <cctype>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <locale>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#if defined(ENABLE_NLS)
#include <libintl.h>
#else
#define gettext(string)                        (string)
#define ngettext(singular, plural, count)      (((count) == 1) ? (singular) : (plural))
#define bindtextdomain(domain, directory)      (directory)
#define textdomain(domain)                     (domain)
#define bind_textdomain_codeset(domain, codeset) (codeset)
#endif

#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#else
#include <unistd.h>
#include <wchar.h>
#endif

#include <libsysteminfo.h>

#include "package-version.h"


// ###### Global settings and formatting constants ##########################
static bool        is_ltr { true };
static std::string rlm;
static std::string lrm;

constexpr const char* colorBasic              = "\033[33m";
constexpr const char* colorNetwork            = "\033[34m";
constexpr const char* colorPrefix             = "\033[36m";
constexpr const char* colorInactive           = "\033[37m";
constexpr const char* colorNone               = "\033[0m";

constexpr const char* colorBatteryFull        = "\033[32m";
constexpr const char* colorBatteryCharging    = "\033[33m";
constexpr const char* colorBatteryNotCharging = "\033[38m";
constexpr const char* colorBatteryDischarging = "\033[35m";

constexpr const char* colorDiskNormal         = "\033[32m";
constexpr const char* colorDisk80             = "\033[33m";
constexpr const char* colorDisk95             = "\033[31m";


// ###### Evaluation helper for gettext strings with ${variable} placeholders
static std::string eval_gettext(const char*                               messageIdentifier,
                                const std::map<std::string, std::string>& variableMap)
{
   std::string result = gettext(messageIdentifier);
   for(const auto& [variableName, variableValue] : variableMap) {
      const std::string placeholder = "${" + variableName + "}";
      size_t position = 0;
      while((position = result.find(placeholder, position)) != std::string::npos) {
         result.replace(position, placeholder.length(), variableValue);
         position += variableValue.length();
      }
   }
   return result;
}


// ###### Locale thousands formatting helper ################################
static std::string formatLocaleNumber(const uint64_t value)
{
   try {
      static const std::locale environmentLocale("");
      std::ostringstream stringStream;
      stringStream.imbue(environmentLocale);
      stringStream << value;
      return stringStream.str();
   }
   catch(...) {
      std::string numberString = std::to_string(value);
      int position = static_cast<int>(numberString.length()) - 3;
      while(position > 0) {
         numberString.insert(static_cast<size_t>(position), ",");
         position -= 3;
      }
      return numberString;
   }
}


// ###### Memory string formatting helper ####################################
static std::string formatMemoryMiB(const uint64_t bytes)
{
   const uint64_t mebibytes = bytes / 1048576ULL;
   std::string formattedString = formatLocaleNumber(mebibytes);
   if(formattedString.length() < 6) {
      formattedString = std::string(6 - formattedString.length(), ' ') + formattedString;
   }
   return formattedString;
}


// ###### Percentage string formatting helper ###############################
static std::string formatPercentage5_1(const double percentage)
{
   char buffer[32];
   std::snprintf(buffer, sizeof(buffer), "%5.1f", percentage);
   return buffer;
}


// ###### UTF-8 visual column display width calculation ######################
static size_t utf8DisplayWidth(std::string_view stringView)
{
   size_t width = 0;
   size_t index = 0;

   while(index < stringView.size()) {
      // Skip ANSI escape sequences: \033[ ... [a-zA-Z]
      if((stringView[index] == '\033') && (index + 1 < stringView.size()) && (stringView[index + 1] == '[')) {
         index += 2;
         while((index < stringView.size()) && !std::isalpha(static_cast<unsigned char>(stringView[index]))) {
            ++index;
         }
         if(index < stringView.size()) {
            ++index;
         }
         continue;
      }

      // Decode UTF-8 code point
      const auto firstByte = static_cast<unsigned char>(stringView[index]);
      char32_t   codePoint = 0;
      size_t     charBytes = 0;

      if(firstByte < 0x80) {
         codePoint = firstByte;
         charBytes = 1;
      }
      else if((firstByte & 0xE0) == 0xC0) {
         if(index + 1 >= stringView.size()) {
            break;
         }
         codePoint = ((firstByte & 0x1F) << 6) | (static_cast<unsigned char>(stringView[index + 1]) & 0x3F);
         charBytes = 2;
      }
      else if((firstByte & 0xF0) == 0xE0) {
         if(index + 2 >= stringView.size()) {
            break;
         }
         codePoint = ((firstByte & 0x0F) << 12)
                   | ((static_cast<unsigned char>(stringView[index + 1]) & 0x3F) << 6)
                   | (static_cast<unsigned char>(stringView[index + 2]) & 0x3F);
         charBytes = 3;
      }
      else if((firstByte & 0xF8) == 0xF0) {
         if(index + 3 >= stringView.size()) {
            break;
         }
         codePoint = ((firstByte & 0x07) << 18)
                   | ((static_cast<unsigned char>(stringView[index + 1]) & 0x3F) << 12)
                   | ((static_cast<unsigned char>(stringView[index + 2]) & 0x3F) << 6)
                   | (static_cast<unsigned char>(stringView[index + 3]) & 0x3F);
         charBytes = 4;
      }
      else {
         ++index;
         continue;
      }
      index += charBytes;

      // Skip zero-width control code points
      if((codePoint == 0x200E) || (codePoint == 0x200F) || (codePoint == 0x200B) ||
         (codePoint == 0xFEFF) || ((codePoint >= 0x200B) && (codePoint <= 0x200F))) {
         continue;
      }

#if !defined(_WIN32)
      const int characterWidth = wcwidth(static_cast<wchar_t>(codePoint));
      if(characterWidth > 0) {
         width += static_cast<size_t>(characterWidth);
      }
#else
      if(((codePoint >= 0x1100)  && (codePoint <= 0x115F))  ||
         ((codePoint >= 0x2E80)  && (codePoint <= 0xA4CF))  ||
         ((codePoint >= 0xAC00)  && (codePoint <= 0xD7A3))  ||
         ((codePoint >= 0xF900)  && (codePoint <= 0xFAFF))  ||
         ((codePoint >= 0xFE10)  && (codePoint <= 0xFE19))  ||
         ((codePoint >= 0xFE30)  && (codePoint <= 0xFE6F))  ||
         ((codePoint >= 0xFF00)  && (codePoint <= 0xFF60))  ||
         ((codePoint >= 0xFFE0)  && (codePoint <= 0xFFE6))  ||
         ((codePoint >= 0x20000) && (codePoint <= 0x3FFFD))) {
         width += 2;
      }
      else {
         width += 1;
      }
#endif
   }
   return width;
}


// ###### Table representation ##############################################
struct TableRow {
   std::string Label;
   std::string Value;
};

static std::vector<TableRow> tableRows;

static void addTableRow(std::string_view label,
                        std::string_view value)
{
   tableRows.push_back(TableRow {
      .Label = std::string(label),
      .Value = std::string(value)
   });
}

static void finishTable(std::ostream& outputStream)
{
   size_t maxLabelWidth = 0;
   for(const auto& row : tableRows) {
      const size_t rowWidth = utf8DisplayWidth(row.Label);
      if(rowWidth > maxLabelWidth) {
         maxLabelWidth = rowWidth;
      }
   }
   const size_t columnWidth = maxLabelWidth + 1;

   for(const auto& row : tableRows) {
      const size_t labelWidth   = utf8DisplayWidth(row.Label);
      const size_t padding      = (columnWidth > labelWidth) ? (columnWidth - labelWidth) : 0;
      const std::string paddedLabel = row.Label + std::string(padding, ' ');

      if(is_ltr) {
         outputStream << paddedLabel << row.Value << '\n';
      }
      else {
         outputStream << rlm << paddedLabel << rlm << row.Value << '\n';
      }
   }
   tableRows.clear();
}


// ###### Print banner scripts ##############################################
static void printBanners(const std::vector<std::string>& directories)
{
   for(const auto& directory : directories) {
      std::error_code errorCode;
      if(!std::filesystem::is_directory(directory, errorCode)) {
         continue;
      }

      std::vector<std::string> bannerScripts;
      for(const auto& directoryEntry : std::filesystem::directory_iterator(
             directory,
             std::filesystem::directory_options::follow_directory_symlink,
             errorCode)) {
         if(directoryEntry.is_regular_file(errorCode)) {
            const std::string filename = directoryEntry.path().filename().string();
            if((filename.length() >= 2) &&
               std::isdigit(static_cast<unsigned char>(filename[0])) &&
               std::isdigit(static_cast<unsigned char>(filename[1])) &&
               (filename.back() != '~')) {
               bannerScripts.push_back(directoryEntry.path().string());
            }
         }
      }
      std::sort(bannerScripts.rbegin(), bannerScripts.rend());

      for(const auto& script : bannerScripts) {
         if(std::system(script.c_str()) != 0) {
            break;
         }
      }
   }
}


// ###### Print SSH public key fingerprints ##################################
static void printSshKeyFingerprints()
{
   std::string     sshDirectory;
   std::error_code errorCode;

#if defined(_WIN32)
   const char* programData = std::getenv("ProgramData");
   if(programData != nullptr) {
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
   unsigned int keyNumber = 1;

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

      char lineBuffer[512] { 0 };
      if(std::fgets(lineBuffer, sizeof(lineBuffer), pipe) != nullptr) {
         std::string line(lineBuffer);
         while(!line.empty() && ((line.back() == '\r') || (line.back() == '\n'))) {
            line.pop_back();
         }

         std::smatch match;
         if(std::regex_match(line, match, keyRegex) && (match.size() == 4)) {
            const std::string bits        = match[1].str();
            const std::string fingerprint = match[2].str();
            const std::string keyType     = match[3].str();
            const std::string keyInfo     = fingerprint + " (" + keyType + " " + bits + ")";

            if(!fingerprint.empty() && std::isupper(static_cast<unsigned char>(fingerprint[0]))) {
               const std::string label = (keyNumber == 1) ? gettext("SSH Keys:") : "";
               addTableRow(label,
                           eval_gettext("#${keyNumber} ${keyInfo}", {
                              { "keyNumber", std::to_string(keyNumber) },
                              { "keyInfo",    keyInfo }
                           }));
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
}


// ###### Operating system release details ##################################
struct OperatingSystemInformation {
   std::string Identifier;
   std::string Release;
   std::string Codename;
};


// ###### Read operating system release information #########################
static OperatingSystemInformation readOperatingSystemInformation(const SystemInfo& systemInfo)
{
   OperatingSystemInformation operatingSystemInfo;
   operatingSystemInfo.Identifier = gettext("Unknown");
   operatingSystemInfo.Release    = operatingSystemInfo.Identifier;

   std::ifstream osReleaseStream("/etc/os-release");
   if(osReleaseStream.is_open()) {
      std::string line;
      while(std::getline(osReleaseStream, line)) {
         const auto equalsPosition = line.find('=');
         if(equalsPosition == std::string::npos) {
            continue;
         }
         const std::string key   = line.substr(0, equalsPosition);
         std::string       value = line.substr(equalsPosition + 1);
         if((value.length() >= 2) && (value.front() == '"') && (value.back() == '"')) {
            value = value.substr(1, value.length() - 2);
         }

         if(key == "NAME") {
            operatingSystemInfo.Identifier = value;
         }
         else if(key == "VERSION_ID") {
            operatingSystemInfo.Release = value;
         }
         else if(key == "VERSION") {
            const auto openParenPosition  = value.rfind('(');
            const auto closeParenPosition = value.rfind(')');
            if((openParenPosition != std::string::npos) &&
               (closeParenPosition != std::string::npos) &&
               (closeParenPosition > openParenPosition)) {
               operatingSystemInfo.Codename = value.substr(openParenPosition + 1,
                                                           closeParenPosition - openParenPosition - 1);
            }
         }
         else if((key == "VERSION_CODENAME") && operatingSystemInfo.Codename.empty()) {
            operatingSystemInfo.Codename = value;
         }
      }
   }
   else {
      operatingSystemInfo.Identifier = systemInfo.getString("system_sysname", gettext("Unknown"));
      operatingSystemInfo.Release    = systemInfo.getString("system_release", operatingSystemInfo.Identifier);
   }

   return operatingSystemInfo;
}


// ###### Parse test file into SystemInfo entries ###########################
static void loadTestSystemInfo(std::string_view filePath,
                               SystemInfo&      systemInfo)
{
   std::ifstream fileStream{ std::string(filePath) };
   if(!fileStream.is_open()) {
      std::cerr << eval_gettext("ERROR: Unable to find input file ${testSystemInfoInput}!", {
         { "testSystemInfoInput", std::string(filePath) }
      }) << '\n';
      std::exit(1);
   }

   std::string line;
   while(std::getline(fileStream, line)) {
      const auto delimiterPosition = line.find('=');
      if(delimiterPosition == std::string::npos) {
         continue;
      }
      const std::string key   = line.substr(0, delimiterPosition);
      std::string       value = line.substr(delimiterPosition + 1);

      if(value == "(INVALID!)") {
         continue;
      }

      if((value.length() >= 2) && (value.front() == '"') && (value.back() == '"')) {
         systemInfo.addString(key, value.substr(1, value.length() - 2));
      }
      else if(value.find('.') != std::string::npos) {
         systemInfo.addDouble(key, std::strtod(value.c_str(), nullptr));
      }
      else {
         char* endPointer = nullptr;
         const unsigned long long number = std::strtoull(value.c_str(), &endPointer, 10);
         if((endPointer != value.c_str()) && (*endPointer == '\0')) {
            if((key.find("mem") != std::string::npos) ||
               (key.find("swap") != std::string::npos) ||
               (key.find("total") != std::string::npos) ||
               (key.find("used") != std::string::npos) ||
               (key.find("available") != std::string::npos) ||
               (number > 0xFFFFFFFFULL)) {
               systemInfo.addUInt64(key, number);
            }
            else {
               systemInfo.addUInt32(key, static_cast<uint32_t>(number));
            }
         }
         else {
            systemInfo.addString(key, value);
         }
      }
   }
}


// ###### Print formatted system information ################################
static void printSystemInformation(const SystemInfo& systemInfo)
{
   std::cout << colorBasic;

   // ====== Hostname =======================================================
   addTableRow(gettext("Host:"), systemInfo.getString("hostname_long"));

   // ====== Uptime =========================================================
   const uint32_t uptimeDays  = systemInfo.getUInt32("uptime_days");
   const uint32_t uptimeHours = systemInfo.getUInt32("uptime_hours");
   const uint32_t uptimeMins  = systemInfo.getUInt32("uptime_mins");
   const uint32_t uptimeSecs  = systemInfo.getUInt32("uptime_secs");

   char buffer[128];
   std::snprintf(buffer, sizeof(buffer),
                 ngettext("%s day", "%s days", static_cast<unsigned long>(uptimeDays)),
                 formatLocaleNumber(uptimeDays).c_str());
   const std::string uptimeInDays = buffer;

   std::snprintf(buffer, sizeof(buffer),
                 ngettext("%d hour", "%d hours", static_cast<unsigned long>(uptimeHours)),
                 static_cast<int>(uptimeHours));
   const std::string uptimeInHours = buffer;

   std::snprintf(buffer, sizeof(buffer),
                 ngettext("%d minute", "%d minutes", static_cast<unsigned long>(uptimeMins)),
                 static_cast<int>(uptimeMins));
   const std::string uptimeInMinutes = buffer;

   std::snprintf(buffer, sizeof(buffer),
                 ngettext("%d second", "%d seconds", static_cast<unsigned long>(uptimeSecs)),
                 static_cast<int>(uptimeSecs));
   const std::string uptimeInSeconds = buffer;

   addTableRow(gettext("Uptime:"),
               eval_gettext("${uptimeInDays}, ${uptimeInHours}, ${uptimeInMinutes}, and ${uptimeInSeconds}", {
                  { "uptimeInDays",    uptimeInDays },
                  { "uptimeInHours",   uptimeInHours },
                  { "uptimeInMinutes", uptimeInMinutes },
                  { "uptimeInSeconds", uptimeInSeconds }
               }));

   // ====== Operating System ===============================================
   const OperatingSystemInformation operatingSystemInfo = readOperatingSystemInformation(systemInfo);
   const std::string codeNameText = !operatingSystemInfo.Codename.empty() ? (" (" + operatingSystemInfo.Codename + ")") : "";
   const std::string osLabel      = operatingSystemInfo.Identifier + " " + operatingSystemInfo.Release + codeNameText;

   addTableRow(gettext("Operating System:"),
               eval_gettext("${os_label} with kernel ${system_release}", {
                  { "os_label",       osLabel },
                  { "system_release", systemInfo.getString("system_release") }
               }));

   // ====== Processor ======================================================
   const uint32_t procs = systemInfo.getUInt32("system_procs");
   const uint32_t users = systemInfo.getUInt32("system_users");
   const uint32_t cores = systemInfo.getUInt32("system_cores");

   std::snprintf(buffer, sizeof(buffer), gettext("%s processes"), formatLocaleNumber(procs).c_str());
   const std::string processesString = buffer;

   std::snprintf(buffer, sizeof(buffer),
                 ngettext("%s user", "%s users", static_cast<unsigned long>(users)),
                 formatLocaleNumber(users).c_str());
   const std::string usersString = buffer;

   addTableRow(gettext("Processor:"),
               eval_gettext("${systemCoresString} cores (${systemMachineString}); ${processesString}; ${usersString}", {
                  { "systemCoresString",   formatLocaleNumber(cores) },
                  { "systemMachineString", systemInfo.getString("system_machine") },
                  { "processesString",     processesString },
                  { "usersString",         usersString }
               }));

   // ====== Load ===========================================================
#if !defined(_WIN32)
   std::snprintf(buffer, sizeof(buffer),
                 gettext("%1.1f %% / %1.1f %% / %1.1f %%"),
                 systemInfo.getDouble("system_load_avg1min"),
                 systemInfo.getDouble("system_load_avg5min"),
                 systemInfo.getDouble("system_load_avg15min"));
   addTableRow(gettext("Load (1/5/15 min):"), buffer);
#endif

   // ====== Battery ========================================================
   const std::string batteryList = systemInfo.getString("battery_list");
   if(!batteryList.empty()) {
      std::istringstream stringStream(batteryList);
      std::string        batteryIndex;
      std::string        batterySummary;
      unsigned int       batteryNumber = 0;

      while(stringStream >> batteryIndex) {
         batteryNumber++;
         const std::string separator = (batteryNumber > 1) ? " |  " : "";
         const uint32_t status   = systemInfo.getUInt32("battery_" + batteryIndex + "_status");
         const uint32_t capacity = systemInfo.getUInt32("battery_" + batteryIndex + "_capacity");

         std::string statusString = gettext("unknown");
         const char* statusColor  = colorInactive;

         switch(status) {
            case 1:
               statusString = gettext("not charging");
               statusColor  = colorBatteryNotCharging;
               break;
            case 2:
               statusString = gettext("charging");
               statusColor  = colorBatteryCharging;
               break;
            case 3:
               statusString = gettext("full");
               statusColor  = colorBatteryFull;
               break;
            case 4:
               statusString = gettext("discharging");
               statusColor  = colorBatteryDischarging;
               break;
            default:
               break;
         }

         batterySummary += separator + statusColor +
            eval_gettext("#${batteryNumber} ${batteryStatus}, ${batteryCapacity} %", {
               { "batteryNumber",   std::to_string(batteryNumber) },
               { "batteryStatus",   statusString },
               { "batteryCapacity", std::to_string(capacity) }
            }) + colorBasic;
      }
      addTableRow(gettext("Battery:"), batterySummary);
   }

   // ====== Memory =========================================================
   const char* memoryFormat = gettext("%s MiB of %s MiB (%s MiB available: %s %%)");
   std::snprintf(buffer, sizeof(buffer), memoryFormat,
                 formatMemoryMiB(systemInfo.getUInt64("system_mem_used")).c_str(),
                 formatMemoryMiB(systemInfo.getUInt64("system_mem_total")).c_str(),
                 formatMemoryMiB(systemInfo.getUInt64("system_mem_free")).c_str(),
                 formatPercentage5_1(systemInfo.getDouble("system_mem_freepct")).c_str());
   addTableRow(gettext("Used Memory:"), buffer);

   if(systemInfo.getUInt64("system_swap_total") > 0) {
      std::snprintf(buffer, sizeof(buffer), memoryFormat,
                    formatMemoryMiB(systemInfo.getUInt64("system_swap_used")).c_str(),
                    formatMemoryMiB(systemInfo.getUInt64("system_swap_total")).c_str(),
                    formatMemoryMiB(systemInfo.getUInt64("system_swap_free")).c_str(),
                    formatPercentage5_1(systemInfo.getDouble("system_swap_freepct")).c_str());
      addTableRow(gettext("Used Swap:"), buffer);
   }
   else {
      addTableRow(gettext("Used Swap:"),
                  std::string(colorInactive) + gettext("no swap available!") + colorBasic);
   }

   // ====== Storage =========================================================
   const std::string diskList = systemInfo.getString("disk_list");
   if(!diskList.empty()) {
      std::istringstream stringStream(diskList);
      std::string        mountPointName;
      std::string        diskSummary;
      bool               isFirstDisk = true;

      while(stringStream >> mountPointName) {
         const std::string mountPoint = (mountPointName == "root") ? "/" : ("/" + mountPointName);
         const std::string separator  = isFirstDisk ? "" : " |  ";
         isFirstDisk = false;

         const std::string pctKey = "disk_" + mountPointName + "_pct";
         if(systemInfo.exists(pctKey) != SystemInfo::ValueType::Invalid) {
            const double percentage = systemInfo.getDouble(pctKey);
            const char* diskColor = colorDiskNormal;
            if(percentage >= 95.0) {
               diskColor = colorDisk95;
            }
            else if(percentage >= 80.0) {
               diskColor = colorDisk80;
            }

            char diskBuffer[128];
            std::snprintf(diskBuffer, sizeof(diskBuffer),
                          gettext("%1.0f %% on %s"),
                          percentage,
                          (lrm + mountPoint + rlm).c_str());
            diskSummary += separator + diskColor + diskBuffer + colorBasic;
         }
      }
      addTableRow(gettext("Used Diskspace:"), diskSummary);
   }

   // ====== SSH Key Fingerprints ============================================
   printSshKeyFingerprints();

   // ====== Network =========================================================
   addTableRow(gettext("Network:"), "");

   const std::string netifList = systemInfo.getString("netif_list");
   if(!netifList.empty()) {
      std::istringstream stringStream(netifList);
      std::string        interfaceIndex;

      while(stringStream >> interfaceIndex) {
         const std::string nameKey  = "netif_" + interfaceIndex + "_name";
         const std::string flagsKey = "netif_" + interfaceIndex + "_flags";
         const std::string ipv4Key  = "netif_" + interfaceIndex + "_ipv4";
         const std::string ipv6Key  = "netif_" + interfaceIndex + "_ipv6";

         const bool hasIpv4 = (systemInfo.exists(ipv4Key) != SystemInfo::ValueType::Invalid);
         const bool hasIpv6 = (systemInfo.exists(ipv6Key) != SystemInfo::ValueType::Invalid);

         if(hasIpv4 || hasIpv6) {
            const std::string ifName  = systemInfo.getString(nameKey);
            const std::string ifFlags = systemInfo.getString(flagsKey);
            const bool isUp = (ifFlags.find("<UP>") != std::string::npos);

            const char* colorA = isUp ? colorNetwork : colorInactive;
            const char* colorP = isUp ? colorPrefix  : colorInactive;

            std::string formattedIpv4;
            if(hasIpv4) {
               std::istringstream ipv4Stream(systemInfo.getString(ipv4Key));
               std::string        ipAddress;
               while(ipv4Stream >> ipAddress) {
                  if(!formattedIpv4.empty()) {
                     formattedIpv4 += "   ";
                  }
                  const auto slashPosition = ipAddress.find('/');
                  if(slashPosition != std::string::npos) {
                     formattedIpv4 += ipAddress.substr(0, slashPosition) + colorP + " / " + ipAddress.substr(slashPosition + 1) + colorA;
                  }
                  else {
                     formattedIpv4 += ipAddress;
                  }
               }
            }
            else {
               formattedIpv4 = gettext("(No IPv4)");
            }

            addTableRow("   " + rlm + colorA + ifName + ":", lrm + formattedIpv4 + colorA);

            if(hasIpv6) {
               std::istringstream ipv6Stream(systemInfo.getString(ipv6Key));
               std::string        ipAddress;
               while(ipv6Stream >> ipAddress) {
                  std::string formattedV6;
                  const auto slashPosition = ipAddress.find('/');
                  if(slashPosition != std::string::npos) {
                     formattedV6 = ipAddress.substr(0, slashPosition) + colorP + " / " + ipAddress.substr(slashPosition + 1) + colorA;
                  }
                  else {
                     formattedV6 = ipAddress + colorA;
                  }
                  addTableRow("", formattedV6);
               }
            }
         }
      }
   }

   finishTable(std::cout);
   std::cout << colorNone;
}


// ###### Usage ##############################################################
[[noreturn]] static void usage(const char* program,
                               const int   exitCode)
{
   std::cerr << gettext("Usage:") << " " << program
             << " [-S|--scripts scripts_directory] [-X|--test-date date_string]"
             << " [-Y|--test-system-info input_file] [-h|--help] [-v|--version]\n";
   std::exit(exitCode);
}


// ###### Version ############################################################
[[noreturn]] static void version()
{
   std::cout << "System-Info " << SYSTEMTOOLS_VERSION << '\n';
   std::exit(0);
}


// ###### Main program #######################################################
int main(int argc, char** argv)
{
   // ====== Configure UTF-8 Console on Windows =============================
#if defined(_WIN32)
   SetConsoleCP(CP_UTF8);
   SetConsoleOutputCP(CP_UTF8);
#endif

   // ====== Initialize locale support ======================================
   if(std::setlocale(LC_ALL, "") == nullptr) {
      std::setlocale(LC_ALL, "C.UTF-8");
   }

#if defined(ENABLE_NLS)
   std::string localeDirectory;
   const char* environmentLocaleDirectory = std::getenv("TEXTDOMAINDIR");
   if(environmentLocaleDirectory != nullptr) {
      localeDirectory = environmentLocaleDirectory;
   }
#if defined(_WIN32)
   else {
      char modulePath[MAX_PATH];
      if(GetModuleFileNameA(nullptr, modulePath, MAX_PATH) > 0) {
         const std::filesystem::path executablePath(modulePath);
         const std::filesystem::path candidatePath = executablePath.parent_path().parent_path() / "share" / "locale";
         std::error_code             errorCode;
         if(std::filesystem::is_directory(candidatePath, errorCode)) {
            localeDirectory = candidatePath.string();
         }
      }
   }
#elif defined(SYSTEMTOOLS_LOCALEDIR)
   else {
      localeDirectory = SYSTEMTOOLS_LOCALEDIR;
   }
#endif

   if(!localeDirectory.empty()) {
      bindtextdomain("System-Info", localeDirectory.c_str());
   }
   bind_textdomain_codeset("System-Info", "UTF-8");
   textdomain("System-Info");
#endif

   is_ltr = (std::string(gettext("LTR")) != "RTL");
   if(!is_ltr) {
      rlm = "\u200F";
      lrm = "\u200E";
   }

   std::vector<std::string> scriptDirectories = {
      "/etc/system-info.d",
      "/usr/local/etc/system-info.d"
   };
#if defined(__linux__)
   scriptDirectories.push_back("/home/linuxbrew/.linuxbrew/etc/system-info.d");
#elif defined(__NetBSD__)
   scriptDirectories.push_back("/usr/pkg/etc/system-info.d");
#elif defined(__APPLE__)
   scriptDirectories.push_back("/opt/homebrew/etc/system-info.d");
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__sun) || defined(__gnu_hurd__) || defined(_WIN32)
   // No additional directories
#else
#error Missing case!
#endif

   std::string testDate;
   std::string testSystemInfoInput;

   static const struct option longOptions[] = {
      { "scripts",          required_argument, nullptr, 'S' },
      { "test-date",        required_argument, nullptr, 'X' },
      { "test-system-info", required_argument, nullptr, 'Y' },
      { "help",             no_argument,       nullptr, 'h' },
      { "version",          no_argument,       nullptr, 'v' },
      { nullptr,            0,                 nullptr, 0   }
   };

   int option    { 0 };
   int longIndex { 0 };
   while((option = getopt_long(argc, argv, "S:X:Y:hv", longOptions, &longIndex)) != -1) {
      switch(option) {
         case 'S':
            scriptDirectories = { optarg };
            break;
         case 'X':
            testDate = optarg;
            break;
         case 'Y':
            testSystemInfoInput = optarg;
            break;
         case 'h':
            usage(argv[0], 0);
            break;
         case 'v':
            version();
            break;
         default:
            usage(argv[0], 1);
            break;
      }
   }
   if(optind < argc) {
      usage(argv[0], 1);
   }

#if defined(_WIN32)
   struct WinSockScope {
      WSADATA Data { };
      WinSockScope() { WSAStartup(MAKEWORD(2, 2), &Data); }
      ~WinSockScope() { WSACleanup(); }
   } winSockScope;
#endif

   std::string nowString = testDate;
   if(nowString.empty()) {
      const std::time_t now = std::time(nullptr);
      char timeBuffer[128];
      struct tm timeStructure { };
#if defined(_WIN32)
      localtime_s(&timeStructure, &now);
#else
      localtime_r(&now, &timeStructure);
#endif
      std::strftime(timeBuffer, sizeof(timeBuffer), "%c", &timeStructure);
      nowString = timeBuffer;
   }

   std::cout << '\n'
             << eval_gettext("System information on ${now}", { { "now", nowString } })
             << "\n\n";

   printBanners(scriptDirectories);

   try {
      if(testSystemInfoInput.empty()) {
         SystemInfo systemInfo(LSI_COMPATIBILITY_VERSION, SystemInfo::Flags::All);
         printSystemInformation(systemInfo);
      }
      else {
         SystemInfo systemInfo(LSI_COMPATIBILITY_VERSION, 0);
         loadTestSystemInfo(testSystemInfoInput, systemInfo);
         printSystemInformation(systemInfo);
      }
   }
   catch(const std::exception& exception) {
      std::cerr << "ERROR: " << exception.what() << '\n';
      return 1;
   }

   return 0;
}
