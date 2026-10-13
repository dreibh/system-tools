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

#ifndef LIBSYSTEMINFO_H
#define LIBSYSTEMINFO_H

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

#define LSI_COMPATIBILITY_VERSION 0

class SystemInfoException : public std::runtime_error {
public:
   using std::runtime_error::runtime_error;
};

class SystemInfo {
public:
   enum Flags : uint32_t {
      Hostname = (1U << 0),
      Cpu      = (1U << 1),
      // Gpu   = (1U << 2),
      Uptime   = (1U << 3),
      Kernel   = (1U << 4),
      Load     = (1U << 5),
      Memory   = (1U << 6),
      Storage  = (1U << 7),
      Network  = (1U << 8),
      Battery  = (1U << 9),
      User     = (1U << 10),
      All      = 0xFFFFFFFFU
   };

   enum class ValueType {
      Invalid = 0,
      String  = 1,
      UInt32  = 2,
      UInt64  = 3,
      Int32   = 4,
      Int64   = 5,
      Double  = 6
   };

   using EntryValue = std::variant<std::monostate,
                                   std::string,
                                   uint32_t,
                                   uint64_t,
                                   int32_t,
                                   int64_t,
                                   double>;

   struct Entry {
      EntryValue Value;
      int        DisplayHint { 0 };
   };

   // Constructor replacing systemInfoObtain()
   explicit SystemInfo(
      const unsigned int compatibilityVersion = LSI_COMPATIBILITY_VERSION,
      const uint32_t     flags                = Flags::All);

   // Destructor replacing systemInfoRelease()
   ~SystemInfo() = default;

   SystemInfo(const SystemInfo&) = default;
   SystemInfo& operator=(const SystemInfo&) = default;
   SystemInfo(SystemInfo&&) noexcept = default;
   SystemInfo& operator=(SystemInfo&&) noexcept = default;

   // Stream formatting replacing systemInfoPrint()
   void print(std::ostream& os = std::cout) const;

   // Mutators replacing systemInfoAdd*()
   void addString(std::string_view key, std::string_view value);
   void addInt32(std::string_view key, const int32_t value);
   void addInt64(std::string_view key, const int64_t value);
   void addUInt32(std::string_view key, const uint32_t value);
   void addUInt64(std::string_view key, const uint64_t value);
   void addDouble(std::string_view key,
                  const double     value,
                  const int        displayHint = 0);

   // Accessors replacing systemInfoGet*() and systemInfoExists()
   [[nodiscard]] ValueType exists(std::string_view key) const;
   [[nodiscard]] std::string getString(
      std::string_view key,
      std::string_view defaultValue = "") const;
   [[nodiscard]] int32_t getInt32(
      std::string_view key,
      const int32_t    defaultValue = 0) const;
   [[nodiscard]] int64_t getInt64(
      std::string_view key,
      const int64_t    defaultValue = 0) const;
   [[nodiscard]] uint32_t getUInt32(
      std::string_view key,
      const uint32_t   defaultValue = 0) const;
   [[nodiscard]] uint64_t getUInt64(
      std::string_view key,
      const uint64_t   defaultValue = 0) const;
   [[nodiscard]] double getDouble(
      std::string_view key,
      const double     defaultValue = 0.0) const;

   [[nodiscard]] const std::map<std::string, Entry>&
   getEntries() const noexcept
   {
      return Entries;
   }

   [[nodiscard]] unsigned int getCompatibilityVersion() const noexcept
   {
      return CompatibilityVersion;
   }

   friend std::ostream& operator<<(std::ostream& os, const SystemInfo& info)
   {
      info.print(os);
      return os;
   }

private:
   void obtainHostnameInformation();
   void obtainKernelInformation();
   void obtainUptimeInformation();
   void obtainLoadInformation();
   void obtainBatteryInformation();
   void obtainMemoryInformation();
   void obtainStorageInformation();
   void obtainNetworkInformation(const bool filterLocalScope = true);
   void obtainUserInformation();

   bool getDiskUsage(std::string_view mountPoint,
                     std::string_view label);

   std::map<std::string, Entry> Entries;
   unsigned int                 CompatibilityVersion { LSI_COMPATIBILITY_VERSION };
};

#endif
