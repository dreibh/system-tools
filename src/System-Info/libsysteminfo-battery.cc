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
#include <fstream>
#include <string>
#include <vector>

#if defined(__FreeBSD__)
#include <dev/acpica/acpiio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#elif defined(__OpenBSD__)
#include <fcntl.h>
#include <machine/apmvar.h>
#include <sys/ioctl.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
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
