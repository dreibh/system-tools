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

#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>


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
   if(flags & Flags::User) {
      obtainUserInformation();
   }
   if((flags & Flags::SSH) != 0) {
      obtainSshKeyFingerprints();
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


// ###### Check whether key exists (returns value type or Invalid) ##########
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
