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
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#if defined(ENABLE_NLS)
#include <libintl.h>
#else
#define gettext(string)                          (string)
#define ngettext(singular, plural, count)        (((count) == 1) ? (singular) : (plural))
#define bindtextdomain(domain, directory)        (directory)
#define textdomain(domain)                       (domain)
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


// ###### Helper to trim whitespace from strings ############################
static std::string trimString(std::string_view stringView)
{
   const auto firstNonSpace = stringView.find_first_not_of(" \t\r\n");
   if(firstNonSpace == std::string_view::npos) {
      return "";
   }
   const auto lastNonSpace = stringView.find_last_not_of(" \t\r\n");
   return std::string(stringView.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1));
}


// ###### Helper to unescape ANSI escape sequence strings ###################
static std::string unescapeColorString(std::string_view input)
{
   std::string trimmed = trimString(input);
   if((trimmed.length() >= 2) &&
      (((trimmed.front() == '"') && (trimmed.back() == '"')) ||
       ((trimmed.front() == '\'') && (trimmed.back() == '\'')))) {
      trimmed = trimmed.substr(1, trimmed.length() - 2);
   }

   std::string result;
   for(size_t index = 0; index < trimmed.length(); ++index) {
      if((trimmed[index] == '\\') && (index + 1 < trimmed.length())) {
         // \e or \E
         if((trimmed[index + 1] == 'e') || (trimmed[index + 1] == 'E')) {
            result += '\x1b';
            ++index;
            continue;
         }
         // \033
         if((trimmed[index + 1] == '0') && (index + 3 < trimmed.length()) &&
            (trimmed[index + 2] == '3') && (trimmed[index + 3] == '3')) {
            result += '\x1b';
            index += 3;
            continue;
         }
         // \x1b, \X1b, \x1B, or \X1B
         if(((trimmed[index + 1] == 'x') || (trimmed[index + 1] == 'X')) &&
            (index + 3 < trimmed.length()) &&
            (trimmed[index + 2] == '1') &&
            ((trimmed[index + 3] == 'b') || (trimmed[index + 3] == 'B'))) {
            result += '\x1b';
            index += 3;
            continue;
         }
         // Escaped backslash
         if(trimmed[index + 1] == '\\') {
            result += '\\';
            ++index;
            continue;
         }
      }
      result += trimmed[index];
   }
   return result;
}


// ###### Struct for Color Settings #########################################
struct ColorSettings {
   std::string Basic               { unescapeColorString(R"(\e[33m)") };
   std::string Network             { unescapeColorString(R"(\e[34m)") };
   std::string Prefix              { unescapeColorString(R"(\e[36m)") };
   std::string Inactive            { unescapeColorString(R"(\e[37m)") };
   std::string None                { unescapeColorString(R"(\e[0m)") };

   std::string BatteryFull         { unescapeColorString(R"(\e[32m)") };
   std::string BatteryCharging     { unescapeColorString(R"(\e[33m)") };
   std::string BatteryNotCharging  { unescapeColorString(R"(\e[38m)") };
   std::string BatteryDischarging  { unescapeColorString(R"(\e[35m)") };

   std::string DiskNormal          { unescapeColorString(R"(\e[32m)") };
   std::string Disk80Percent       { unescapeColorString(R"(\e[33m)") };
   std::string Disk95Percent       { unescapeColorString(R"(\e[31m)") };
};

static ColorSettings colors;


// ###### Parse key-value pairs from configuration file #####################
static void parseConfigurationFile(const std::filesystem::path& configPath)
{
   std::ifstream configFileStream{ configPath };
   if(!configFileStream.is_open()) {
      return;
   }

   std::string line;
   while(std::getline(configFileStream, line)) {
      const auto commentPosition = line.find('#');
      if(commentPosition != std::string::npos) {
         line = line.substr(0, commentPosition);
      }

      const auto delimiterPosition = line.find('=');
      if(delimiterPosition == std::string::npos) {
         continue;
      }

      const std::string key   = trimString(line.substr(0, delimiterPosition));
      const std::string value = unescapeColorString(line.substr(delimiterPosition + 1));

      if(key == "color_basic") {
         colors.Basic = value;
      }
      else if(key == "color_network") {
         colors.Network = value;
      }
      else if(key == "color_prefix") {
         colors.Prefix = value;
      }
      else if(key == "color_inactive") {
         colors.Inactive = value;
      }
      else if(key == "color_none") {
         colors.None = value;
      }
      else if(key == "color_battery_full") {
         colors.BatteryFull = value;
      }
      else if(key == "color_battery_charging") {
         colors.BatteryCharging = value;
      }
      else if(key == "color_battery_not_charging") {
         colors.BatteryNotCharging = value;
      }
      else if(key == "color_battery_discharging") {
         colors.BatteryDischarging = value;
      }
      else if(key == "color_disk_normal") {
         colors.DiskNormal = value;
      }
      else if(key == "color_disk_80percent") {
         colors.Disk80Percent = value;
      }
      else if(key == "color_disk_95percent") {
         colors.Disk95Percent = value;
      }
   }
}


// ###### C++20 gettext runtime format helper ################################
template<typename... Args>
static std::string formatText(const char* messageIdentifier,
                              const Args&... arguments)
{
   try {
      return std::vformat(gettext(messageIdentifier),
                          std::make_format_args(arguments...));
   }
   catch(const std::format_error&) {
      return gettext(messageIdentifier);
   }
}


// ###### Read configuration file with fallback discovery ##################
static void readConfiguration(const std::string&              customConfigurationFile,
                              const std::vector<std::string>& scriptDirectories)
{
   std::error_code errorCode;

   if(!customConfigurationFile.empty()) {
      const std::filesystem::path customPath{ customConfigurationFile };
      if(!std::filesystem::is_regular_file(customPath, errorCode)) {
         std::cerr << formatText("ERROR: Unable to find configuration file {0}!\n", customConfigurationFile);
         std::exit(1);
      }
      parseConfigurationFile(customPath);
      return;
   }

   for(const auto& directory : scriptDirectories) {
      const std::filesystem::path scriptDirectoryPath{ directory };
      const std::filesystem::path configPath = scriptDirectoryPath.parent_path() / "system-info.conf";

      if(std::filesystem::is_regular_file(configPath, errorCode)) {
         parseConfigurationFile(configPath);
         break;
      }
   }
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
      return std::to_string(value);
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
      const uint32_t firstByte = static_cast<unsigned char>(stringView[index]);
      char32_t       codePoint = 0x00;
      size_t         charBytes = 0;

      const auto continuationByte = [&](const size_t offset) noexcept -> uint32_t {
         return static_cast<unsigned char>(stringView[index + offset]) & 0x3fu;
      };

      if(firstByte < 0x80u) {
         codePoint = static_cast<char32_t>(firstByte);
         charBytes = 1;
      }
      else if((firstByte & 0xe0u) == 0xc0u) {
         if(index + 1 >= stringView.size()) {
            break;
         }
         codePoint = static_cast<char32_t>(((firstByte & 0x1fu) << 6u) | continuationByte(1));
         charBytes = 2;
      }
      else if((firstByte & 0xf0u) == 0xe0u) {
         if(index + 2 >= stringView.size()) {
            break;
         }
         codePoint = static_cast<char32_t>(((firstByte & 0x0fu) << 12u) |
                                           (continuationByte(1) << 6u) |
                                           continuationByte(2));
         charBytes = 3;
      }
      else if((firstByte & 0xf8u) == 0xf0u) {
         if(index + 3 >= stringView.size()) {
            break;
         }
         codePoint = static_cast<char32_t>(((firstByte & 0x07u) << 18u) |
                                           (continuationByte(1) << 12u) |
                                           (continuationByte(2) << 6u) |
                                           continuationByte(3));
         charBytes = 4;
      }
      else {
         ++index;
         continue;
      }
      index += charBytes;

      // Skip zero-width control code points
      if((codePoint == 0x200eu) || (codePoint == 0x200fu) || (codePoint == 0x200bu) ||
         (codePoint == 0xfeffu) || ((codePoint >= 0x200bu) && (codePoint <= 0x200fu))) {
         continue;
      }

#if !defined(_WIN32)
      const int characterWidth = wcwidth(static_cast<wchar_t>(codePoint));
      if(characterWidth > 0) {
         width += static_cast<size_t>(characterWidth);
      }
#else
      if(((codePoint >= 0x1100u)  && (codePoint <= 0x115fu))  ||
         ((codePoint >= 0x2e80u)  && (codePoint <= 0xa4cfu))  ||
         ((codePoint >= 0xac00u)  && (codePoint <= 0xd7a3u))  ||
         ((codePoint >= 0xf900u)  && (codePoint <= 0xfaffu))  ||
         ((codePoint >= 0xfe10u)  && (codePoint <= 0xfe19u))  ||
         ((codePoint >= 0xfe30u)  && (codePoint <= 0xfe6fu))  ||
         ((codePoint >= 0xff00u)  && (codePoint <= 0xff60u))  ||
         ((codePoint >= 0xffe0u)  && (codePoint <= 0xffe6u))  ||
         ((codePoint >= 0x20000u) && (codePoint <= 0x3fffdu))) {
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
static void printSshKeyFingerprints(const SystemInfo& systemInfo)
{
   const std::string sshKeyList = systemInfo.getString("ssh_key_list");
   if(sshKeyList.empty()) {
      return;
   }

   std::istringstream stringStream(sshKeyList);
   std::string        keyIndex;
   unsigned int       keyNumber = 1;

   while(stringStream >> keyIndex) {
      const std::string fingerprintKey = "ssh_key_fingerprint_" + keyIndex;
      const std::string typeKey        = "ssh_key_type_" + keyIndex;
      const std::string bitsKey        = "ssh_key_bits_" + keyIndex;

      if(systemInfo.exists(fingerprintKey) != SystemInfo::ValueType::Invalid) {
         const std::string fingerprint = systemInfo.getString(fingerprintKey);
         std::string       keyType     = systemInfo.getString(typeKey);
         if(keyType.empty()) {
            keyType = systemInfo.getString("ssh_key_type");
         }
         const uint32_t bits = systemInfo.getUInt32(bitsKey);

         std::string keyInfo = fingerprint;
         if(!keyType.empty()) {
            if(bits > 0) {
               keyInfo += " (" + keyType + " " + std::to_string(bits) + ")";
            }
            else {
               keyInfo += " (" + keyType + ")";
            }
         }

         const std::string label = (keyNumber == 1) ? gettext("SSH Keys:") : "";
         addTableRow(label, formatText("#{0} {1}", keyNumber, keyInfo));
         keyNumber++;
      }
   }
}


// ###### Parse test file into SystemInfo entries ###########################
static void loadTestSystemInfo(std::string_view filePath,
                               SystemInfo&      systemInfo)
{
   std::ifstream fileStream{ std::string(filePath) };
   if(!fileStream.is_open()) {
      std::cerr << formatText("ERROR: Unable to find input file {0}!\n", filePath);
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
         if((endPointer != value.c_str()) && (*endPointer == 0x00)) {
            if((key.find("mem") != std::string::npos) ||
               (key.find("swap") != std::string::npos) ||
               (key.find("total") != std::string::npos) ||
               (key.find("used") != std::string::npos) ||
               (key.find("available") != std::string::npos) ||
               (number > 0xffffffffull)) {
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
   std::cout << colors.Basic;

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
               formatText("{0}, {1}, {2}, and {3}",
                          uptimeInDays, uptimeInHours, uptimeInMinutes, uptimeInSeconds));

   // ====== Operating System ===============================================
   std::string osName = systemInfo.getString("os_name");
   if(osName.empty()) {
      osName = systemInfo.getString("system_sysname", gettext("Unknown"));
   }

   std::string osRelease = systemInfo.getString("os_version_id");
   if(osRelease.empty()) {
      osRelease = systemInfo.getString("os_version");
   }
   if(osRelease.empty()) {
      osRelease = systemInfo.getString("system_release");
   }

   const std::string codename     = systemInfo.getString("os_version_codename");
   const std::string codeNameText = !codename.empty() ? (" (" + codename + ")") : "";
   const std::string osLabel      = osName + (!osRelease.empty() ? (" " + osRelease) : "") + codeNameText;

   addTableRow(gettext("Operating System:"),
               formatText("{0} with kernel {1}",
                          osLabel, systemInfo.getString("system_release")));

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
               formatText("{0} cores ({1}); {2}; {3}",
                          formatLocaleNumber(cores),
                          systemInfo.getString("system_machine"),
                          processesString,
                          usersString));

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

         std::string      statusString = gettext("unknown");
         std::string_view statusColor  = colors.Inactive;

         switch(status) {
            case 1:
               statusString = gettext("not charging");
               statusColor  = colors.BatteryNotCharging;
               break;
            case 2:
               statusString = gettext("charging");
               statusColor  = colors.BatteryCharging;
               break;
            case 3:
               statusString = gettext("full");
               statusColor  = colors.BatteryFull;
               break;
            case 4:
               statusString = gettext("discharging");
               statusColor  = colors.BatteryDischarging;
               break;
            default:
               break;
         }

         batterySummary += separator;
         batterySummary += statusColor;
         batterySummary += formatText("#{0} {1}, {2} %", batteryNumber, statusString, capacity);
         batterySummary += colors.Basic;
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
                  colors.Inactive + gettext("no swap available!") + colors.Basic);
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
            const double     percentage = systemInfo.getDouble(pctKey);
            std::string_view diskColor  = colors.DiskNormal;
            if(percentage >= 95.0) {
               diskColor = colors.Disk95Percent;
            }
            else if(percentage >= 80.0) {
               diskColor = colors.Disk80Percent;
            }

            char diskBuffer[128];
            std::snprintf(diskBuffer, sizeof(diskBuffer),
                          gettext("%1.0f %% on %s"),
                          percentage,
                          (lrm + mountPoint + rlm).c_str());
            diskSummary += separator;
            diskSummary += diskColor;
            diskSummary += diskBuffer;
            diskSummary += colors.Basic;
         }
      }
      addTableRow(gettext("Used Diskspace:"), diskSummary);
   }

   // ====== SSH Key Fingerprints ============================================
   printSshKeyFingerprints(systemInfo);

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

            const std::string_view colorA = isUp ? colors.Network : colors.Inactive;
            const std::string_view colorP = isUp ? colors.Prefix  : colors.Inactive;

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
                     formattedIpv4 += ipAddress.substr(0, slashPosition);
                     formattedIpv4 += colorP;
                     formattedIpv4 += " / ";
                     formattedIpv4 += ipAddress.substr(slashPosition + 1);
                     formattedIpv4 += colorA;
                  }
                  else {
                     formattedIpv4 += ipAddress;
                  }
               }
            }
            else {
               formattedIpv4 = gettext("(No IPv4)");
            }

            std::string networkLabel = "   ";
            networkLabel += rlm;
            networkLabel += colorA;
            networkLabel += ifName;
            networkLabel += ":";

            std::string networkValue = lrm;
            networkValue += formattedIpv4;
            networkValue += colorA;

            addTableRow(networkLabel, networkValue);

            if(hasIpv6) {
               std::istringstream ipv6Stream(systemInfo.getString(ipv6Key));
               std::string        ipAddress;
               while(ipv6Stream >> ipAddress) {
                  std::string formattedV6;
                  const auto slashPosition = ipAddress.find('/');
                  if(slashPosition != std::string::npos) {
                     formattedV6 += ipAddress.substr(0, slashPosition);
                     formattedV6 += colorP;
                     formattedV6 += " / ";
                     formattedV6 += ipAddress.substr(slashPosition + 1);
                     formattedV6 += colorA;
                  }
                  else {
                     formattedV6 += ipAddress;
                     formattedV6 += colorA;
                  }
                  addTableRow("", formattedV6);
               }
            }
         }
      }
   }

   finishTable(std::cout);
   std::cout << colors.None;
}


// ###### Usage ##############################################################
[[noreturn]] static void usage(const char* program,
                               const int   exitCode)
{
   std::cerr << gettext("Usage:") << " " << program
             << " [-C|--config config_file] [-S|--scripts scripts_directory]"
             << " [-X|--test-date date_string] [-Y|--test-system-info input_file]"
             << " [-h|--help] [-v|--version]\n";
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
      rlm = "\u200f";
      lrm = "\u200e";
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

   std::string customConfigurationFile;
   std::string testDate;
   std::string testSystemInfoInput;

   static const struct option longOptions[] = {
      { "config",           required_argument, nullptr, 'C' },
      { "scripts",          required_argument, nullptr, 'S' },
      { "test-date",        required_argument, nullptr, 'X' },
      { "test-system-info", required_argument, nullptr, 'Y' },
      { "help",             no_argument,       nullptr, 'h' },
      { "version",          no_argument,       nullptr, 'v' },
      { nullptr,            0,                 nullptr, 0   }
   };

   int option    { 0 };
   int longIndex { 0 };
   while((option = getopt_long(argc, argv, "C:S:X:Y:hv", longOptions, &longIndex)) != -1) {
      switch(option) {
         case 'C':
            customConfigurationFile = optarg;
            break;
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

   // ====== Read configuration =============================================
   readConfiguration(customConfigurationFile, scriptDirectories);

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
             << formatText("System information on {0}", nowString)
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
