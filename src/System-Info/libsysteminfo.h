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

#include <stdint.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


// Compatibility version of libsysteminfo, to allow for future changes:
// Currently, there is just version 0.
#define LSI_COMPATIBILITY_VERSION 0


struct SystemInfo;

typedef enum {
   SIET_INVALID = 0,
   SIET_STRING  = 1,
   SIET_UINT32  = 2,
   SIET_UINT64  = 3,
   SIET_INT32   = 4,
   SIET_INT64   = 5,
   SIET_DOUBLE  = 6
} SystemInfoEntryValueType;

typedef enum {
   SIOF_HOSTNAME = (1UL << 0),
   SIOF_CPU      = (1UL << 1),
   // SIOF_GPU   = (1UL << 2),
   SIOF_UPTIME   = (1UL << 3),
   SIOF_KERNEL   = (1UL << 4),
   SIOF_LOAD     = (1UL << 5),
   SIOF_MEMORY   = (1UL << 6),
   SIOF_STORAGE  = (1UL << 7),
   SIOF_NETWORK  = (1UL << 8),
   SIOF_BATTERY  = (1UL << 9)
} SystemInfoObtainFlags;

struct SystemInfo* systemInfoObtain(const unsigned int compatibilityVersion,
                                    const unsigned int flags);
void systemInfoRelease(struct SystemInfo* systemInfo);
void systemInfoPrint(const struct SystemInfo* systemInfo, FILE* fd);

void systemInfoAddString(struct SystemInfo* systemInfo, const char* key, const char* value);
void systemInfoAddInt32(struct SystemInfo* systemInfo, const char* key, const int32_t value);
void systemInfoAddInt64(struct SystemInfo* systemInfo, const char* key, const int64_t value);
void systemInfoAddUInt32(struct SystemInfo* systemInfo, const char* key, const uint32_t value);
void systemInfoAddUInt64(struct SystemInfo* systemInfo, const char* key, const uint64_t value);
void systemInfoAddDouble(struct SystemInfo* systemInfo, const char* key, const double value,
                         const int displayHint);

SystemInfoEntryValueType systemInfoExists(struct SystemInfo* systemInfo, const char* key);
int32_t systemInfoGetInt32(struct SystemInfo* systemInfo, const char* key, const int32_t defaultValue);
int64_t systemInfoGetInt64(struct SystemInfo* systemInfo, const char* key, const int64_t defaultValue);
uint32_t systemInfoGetUInt32(struct SystemInfo* systemInfo, const char* key, const uint32_t defaultValue);
uint64_t systemInfoGetUInt64(struct SystemInfo* systemInfo, const char* key, const uint64_t defaultValue);
double systemInfoGetDouble(struct SystemInfo* systemInfo, const char* key, const double defaultValue);

#ifdef __cplusplus
}
#endif

#endif
