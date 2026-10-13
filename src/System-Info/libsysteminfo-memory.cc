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

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <unistd.h>
#endif

#if defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <vm/vm_param.h>
#elif defined(__NetBSD__)
#include <sys/param.h>
#include <sys/swap.h>
#include <sys/sysctl.h>
#include <uvm/uvm_extern.h>
#elif defined(__OpenBSD__)
#include <sys/param.h>
#include <sys/swap.h>
#include <sys/sysctl.h>
#include <uvm/uvm_extern.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>
#elif defined(_WIN32)
#include <windows.h>
#include <psapi.h>
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
