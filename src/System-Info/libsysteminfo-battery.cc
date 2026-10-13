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

#include <cstring>
#include <format>
#include <fstream>
#include <string>
#include <vector>

#if defined(__FreeBSD__)
#include <dev/acpica/acpiio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#elif defined(__NetBSD__)
#include <fcntl.h>
#include <sys/envsys.h>
#include <sys/ioctl.h>
#include <unistd.h>
#elif defined(__OpenBSD__)
#include <fcntl.h>
#include <machine/apmvar.h>
#include <sys/ioctl.h>
#include <unistd.h>
#elif defined(__sun__)
#include <kstat.h>
#elif defined(_WIN32)
#include <windows.h>
#endif


#if defined(__sun__)
// ###### Get 64-bit unsigned integer value from kstat ######################
static bool getKstatUint64(const kstat_named_t* kn,
                           uint64_t*            value)
{
   switch(kn->data_type) {
      case KSTAT_DATA_UINT32:
         *value = static_cast<uint64_t>(kn->value.ui32);
         return true;
      case KSTAT_DATA_UINT64:
         *value = kn->value.ui64;
         return true;
      case KSTAT_DATA_INT32:
         *value = (kn->value.i32 >= 0) ? static_cast<uint64_t>(kn->value.i32) : 0;
         return true;
      case KSTAT_DATA_INT64:
         *value = (kn->value.i64 >= 0) ? static_cast<uint64_t>(kn->value.i64) : 0;
         return true;
      default:
         return false;
   }
}
#endif


// ##### Obtain battery information #########################################
void SystemInfo::obtainBatteryInformation()
{
   std::vector<unsigned int> batteryIDs;

#if defined(__linux__)
   for(unsigned int i = 0; i < 2; ++i) {
      std::ifstream capFile(std::format("/sys/class/power_supply/BAT{}/capacity", i));
      unsigned int  capacity = 0;
      if(capFile >> capacity) {
         std::ifstream statFile(std::format("/sys/class/power_supply/BAT{}/status", i));
         std::string   statusString;
         std::getline(statFile, statusString);
         unsigned int status = 0;
         if(statusString == "Not charging") {
            status = 1;
         }
         else if(statusString == "Charging") {
            status = 2;
         }
         else if(statusString == "Full") {
            status = 3;
         }
         else if(statusString == "Discharging") {
            status = 4;
         }

         addUInt32(std::format("battery_{}_status", i), status);
         addUInt32(std::format("battery_{}_capacity", i), capacity);
         batteryIDs.push_back(i);
      }
   }
#elif defined(__FreeBSD__)
   int acpiFD = open("/dev/acpi", O_RDONLY);
   if(acpiFD >= 0) {
      unsigned int batteryUnits = 0;
      if(ioctl(acpiFD, ACPIIO_BATT_GET_UNITS, &batteryUnits) == 0) {
         for(unsigned int i = 0; (i < batteryUnits) && (i < 2); ++i) {
            union acpi_battery_ioctl_arg batteryInfo { };
            batteryInfo.unit = static_cast<int>(i);
            if(ioctl(acpiFD, ACPIIO_BATT_GET_BATTINFO, &batteryInfo) == 0) {
               unsigned int status = 0;
               if(batteryInfo.battinfo.state != ACPI_BATT_STAT_NOT_PRESENT) {
                  if(batteryInfo.battinfo.state == 0) {
                     status = 3;
                  }
                  else if(batteryInfo.battinfo.state & ACPI_BATT_STAT_CHARGING) {
                     status = 2;
                  }
                  else if(batteryInfo.battinfo.state & ACPI_BATT_STAT_DISCHARG) {
                     status = 4;
                  }
                  else {
                     status = 1;
                  }
               }
               addUInt32(std::format("battery_{}_status", i), status);
               addUInt32(std::format("battery_{}_capacity", i), static_cast<unsigned int>(batteryInfo.battinfo.cap));
               batteryIDs.push_back(i);
            }
         }
      }
      close(acpiFD);
   }
#elif defined(__NetBSD__)
   int sysmonFD = open("/dev/sysmon", O_RDONLY);
   if(sysmonFD >= 0) {
      bool                foundBattery  { false };
      bool                isPresent     { false };
      bool                isCharging    { false };
      bool                isDischarging { false };
      unsigned int        capacity      { 0 };
      unsigned int        status        { 0 };
      envsys_basic_info_t info          { };
      envsys_tre_data_t   data          { };

      for(unsigned int i = 0; i < 256; ++i) {
         std::memset(&info, 0, sizeof(info));
         info.sensor = i;
         if(ioctl(sysmonFD, ENVSYS_GTREINFO, &info) == -1) {
            continue;
         }
         if(!(info.validflags & ENVSYS_FVALID)) {
            continue;
         }

         std::memset(&data, 0, sizeof(data));
         data.sensor = i;
         if(ioctl(sysmonFD, ENVSYS_GTREDATA, &data) == -1) {
            continue;
         }
         if(!(data.validflags & ENVSYS_FCURVALID)) {
            continue;
         }

         if(std::strstr(info.desc, "acpibat0") != nullptr) {
            foundBattery = true;
            if(std::strstr(info.desc, "present") != nullptr) {
               if(data.cur.data_us > 0) {
                  isPresent = true;
               }
            }
            else if((std::strstr(info.desc, "charge") != nullptr) &&
                    (std::strstr(info.desc, "rate") == nullptr)) {
               if(data.max.data_us > 0) {
                  capacity  = static_cast<unsigned int>((static_cast<uint64_t>(data.cur.data_us) * 100) / data.max.data_us);
                  isPresent = true;
               }
            }
            else if(std::strstr(info.desc, "charging") != nullptr) {
               if(data.cur.data_us > 0) {
                  isCharging = true;
               }
            }
            else if(std::strstr(info.desc, "discharge rate") != nullptr) {
               if(data.cur.data_us > 0) {
                  isDischarging = true;
               }
            }
         }
      }
      close(sysmonFD);

      if(foundBattery && isPresent) {
         if(capacity > 100) {
            capacity = 100;
         }

         if(capacity == 100) {
            status = 3;
         }
         else if(isCharging) {
            status = 2;
         }
         else if(isDischarging) {
            status = 4;
         }
         else {
            status = 1;
         }

         addUInt32("battery_0_status", status);
         addUInt32("battery_0_capacity", capacity);
         batteryIDs.push_back(0);
      }
   }
#elif defined(__OpenBSD__)
   int apmFD = open("/dev/apm", O_RDONLY);
   if(apmFD >= 0) {
      struct apm_power_info powerInfo { };
      if(ioctl(apmFD, APM_IOC_GETPOWER, &powerInfo) == 0) {
         if((powerInfo.battery_life <= 100) &&
            (powerInfo.battery_state != APM_BATT_UNKNOWN) &&
            (powerInfo.battery_state != APM_BATTERY_ABSENT)) {
            unsigned int status = 0;
            if(powerInfo.battery_state == APM_BATT_CHARGING) {
               status = 2;
            }
            else if(powerInfo.battery_life == 100) {
               status = 3;
            }
            else if(powerInfo.ac_state == APM_AC_ON) {
               status = 1;
            }
            else if(powerInfo.ac_state == APM_AC_OFF) {
               status = 4;
            }

            addUInt32("battery_0_status", status);
            addUInt32("battery_0_capacity", powerInfo.battery_life);
            batteryIDs.push_back(0);
         }
      }
      close(apmFD);
   }
#elif defined(__sun__)
   kstat_ctl_t* kstatControl = kstat_open();
   if(kstatControl != nullptr) {
      struct SolarisBatteryInfo {
         uint64_t lastCapacity           { 0 };
         uint64_t remainingCapacity      { 0 };
         uint64_t state                  { 0 };
         bool     foundLastCapacity      { false };
         bool     foundRemainingCapacity { false };
         bool     foundState             { false };
      } batteryInfo[2];

      for(kstat_t* kstatPointer = kstatControl->kc_chain; kstatPointer != nullptr; kstatPointer = kstatPointer->ks_next) {
         if(std::strcmp(kstatPointer->ks_module, "acpi_drv") == 0) {
            unsigned int batteryIdentifier { 0 };
            bool         isBIF             { false };
            bool         isBST             { false };

            if(sscanf(kstatPointer->ks_name, "battery BIF%u", &batteryIdentifier) == 1) {
               isBIF = true;
            }
            else if(std::strcmp(kstatPointer->ks_name, "battery BIF") == 0) {
               batteryIdentifier = 0;
               isBIF             = true;
            }
            else if(sscanf(kstatPointer->ks_name, "battery BST%u", &batteryIdentifier) == 1) {
               isBST = true;
            }
            else if(std::strcmp(kstatPointer->ks_name, "battery BST") == 0) {
               batteryIdentifier = 0;
               isBST             = true;
            }

            if((isBIF || isBST) && (batteryIdentifier < 2)) {
               if(kstat_read(kstatControl, kstatPointer, nullptr) != -1) {
                  const auto* kstatNamedArray = reinterpret_cast<const kstat_named_t*>(kstatPointer->ks_data);
                  for(size_t i = 0; i < kstatPointer->ks_ndata; ++i) {
                     const char* name = kstatNamedArray[i].name;
                     if(isBIF && (std::strcmp(name, "bif_last_cap") == 0)) {
                        batteryInfo[batteryIdentifier].foundLastCapacity =
                           getKstatUint64(&kstatNamedArray[i], &batteryInfo[batteryIdentifier].lastCapacity);
                     }
                     else if(isBST && (std::strcmp(name, "bst_rem_cap") == 0)) {
                        batteryInfo[batteryIdentifier].foundRemainingCapacity =
                           getKstatUint64(&kstatNamedArray[i], &batteryInfo[batteryIdentifier].remainingCapacity);
                     }
                     else if(isBST && (std::strcmp(name, "bst_state") == 0)) {
                        batteryInfo[batteryIdentifier].foundState =
                           getKstatUint64(&kstatNamedArray[i], &batteryInfo[batteryIdentifier].state);
                     }
                  }
               }
            }
         }
      }
      kstat_close(kstatControl);

      for(unsigned int i = 0; i < 2; ++i) {
         if(batteryInfo[i].foundLastCapacity &&
            batteryInfo[i].foundRemainingCapacity &&
            batteryInfo[i].foundState &&
            (batteryInfo[i].lastCapacity > 0)) {

            unsigned int capacity = static_cast<unsigned int>(
               (batteryInfo[i].remainingCapacity * 100) / batteryInfo[i].lastCapacity);
            if(capacity > 100) {
               capacity = 100;
            }

            unsigned int status = 0;
            if(batteryInfo[i].state & 0x02) {
               status = 2;
            }
            else if(batteryInfo[i].state & 0x01) {
               status = 4;
            }
            else if(capacity == 100) {
               status = 3;
            }
            else {
               status = 1;
            }

            addUInt32(std::format("battery_{}_status", i), status);
            addUInt32(std::format("battery_{}_capacity", i), capacity);
            batteryIDs.push_back(i);
         }
      }
   }
#elif defined(_WIN32)
   SYSTEM_POWER_STATUS sysStatus;
   if(GetSystemPowerStatus(&sysStatus) && (sysStatus.BatteryFlag != 255)) {
      unsigned int status = 0;
      if(sysStatus.ACLineStatus == 1) {
         status = (sysStatus.BatteryLifePercent >= 100) ? 3 : 2;
      }
      else if(sysStatus.ACLineStatus == 0) {
         status = 4;
      }
      unsigned int capacity = sysStatus.BatteryLifePercent;
      if(capacity > 100) {
         capacity = 100;
      }

      addUInt32("battery_0_status", status);
      addUInt32("battery_0_capacity", capacity);
      batteryIDs.push_back(0);
   }
#elif defined(__APPLE__) || defined(__gnu_hurd__)
   // No battery query support implemented on macOS or GNU Hurd
#else
#error Missing case!
#endif

   std::string batList;
   for(size_t i = 0; i < batteryIDs.size(); ++i) {
      if(i > 0) {
         batList += ' ';
      }
      batList += std::to_string(batteryIDs[i]);
   }
   addString("battery_list", batList);
}
