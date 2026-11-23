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
#include <string>
#include <string_view>

#if !defined(_WIN32)
#include <sys/statvfs.h>
#else
#include <windows.h>
#endif


// ###### Obtain disk usage statistics ######################################
bool SystemInfo::getDiskUsage(std::string_view mountPoint,
                              std::string_view label)
{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun) || defined(__gnu_hurd__) || defined(__APPLE__)
   struct statvfs status { };
   if(statvfs(std::string(mountPoint).c_str(), &status) != 0) {
      return false;
   }

   const uint64_t total     = static_cast<uint64_t>(status.f_blocks) * status.f_frsize;
   const uint64_t available = static_cast<uint64_t>(status.f_bavail) * status.f_frsize;
   const uint64_t freeSpace = static_cast<uint64_t>(status.f_bfree) * status.f_frsize;
#elif defined(_WIN32)
   ULARGE_INTEGER freeBytes, totalBytes, totalFree;
   if(!GetDiskFreeSpaceExA(std::string(mountPoint).c_str(), &freeBytes, &totalBytes, &totalFree)) {
      return false;
   }

   const uint64_t total     = totalBytes.QuadPart;
   const uint64_t available = freeBytes.QuadPart;
   const uint64_t freeSpace = totalFree.QuadPart;
#else
#error Missing case!
#endif
   const uint64_t used            = total - freeSpace;
   const double   usagePercentage = (total > 0) ? (100.0 * static_cast<double>(total - available) / total) : 0.0;

   addUInt64(std::format("disk_{}_total", label), total);
   addUInt64(std::format("disk_{}_used", label), used);
   addUInt64(std::format("disk_{}_available", label), available);
   addDouble(std::format("disk_{}_pct", label), usagePercentage, 3);
   return true;
}


// ##### Obtain disk information ############################################
void SystemInfo::obtainStorageInformation()
{
   getDiskUsage("/", "root");
   getDiskUsage("/tmp", "tmp");
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun) || defined(__gnu_hurd__) || defined(__APPLE__)
   getDiskUsage("/home", "home");
   addString("disk_list", "root tmp home");
#elif defined(_WIN32)
   getDiskUsage("/c", "c");
   addString("disk_list", "root tmp c");
#else
#error Missing case!
#endif
}
