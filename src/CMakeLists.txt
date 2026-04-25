# ==========================================================================
#         ____            _                     _____           _
#        / ___| _   _ ___| |_ ___ _ __ ___     |_   _|__   ___ | |___
#        \___ \| | | / __| __/ _ \ '_ ` _ \ _____| |/ _ \ / _ \| / __|
#         ___) | |_| \__ \ ||  __/ | | | | |_____| | (_) | (_) | \__ \
#        |____/ \__, |___/\__\___|_| |_| |_|     |_|\___/ \___/|_|___/
#               |___/
#                             --- System-Tools ---
#                  https://www.nntb.no/~dreibh/system-tools/
# ==========================================================================
#
# System-Tools
# Copyright (C) 2013-2026 by Thomas Dreibholz
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact: thomas.dreibholz@gmail.com


#############################################################################
#### VERSION FILE                                                        ####
#############################################################################

CONFIGURE_FILE (
   "${CMAKE_CURRENT_SOURCE_DIR}/package-version.h.in"
   "${CMAKE_CURRENT_BINARY_DIR}/package-version.h"
   @ONLY
)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})


#############################################################################
#### SUBDIRECTORIES                                                      ####
#############################################################################

IF (WITH_SYSTEM_INFO)
   ADD_SUBDIRECTORY(System-Info)
ENDIF()

IF (WITH_SYSTEM_MAINTENANCE)
   ADD_SUBDIRECTORY(System-Maintenance)
ENDIF()

IF (WITH_PRINT_UTF8)
   ADD_SUBDIRECTORY(Print-UTF8)
ENDIF()

IF (WITH_ANSI_COLOR)
   ADD_SUBDIRECTORY(ANSI-Color)
ENDIF()

IF (WITH_TEXT_BLOCK)
   ADD_SUBDIRECTORY(Text-Block)
ENDIF()

IF (WITH_FINGERPRINT_SSH_KEYS)
   ADD_SUBDIRECTORY(Fingerprint-SSH-Keys)
ENDIF()

IF (WITH_CONFIGURE_GRUB)
   ADD_SUBDIRECTORY(Configure-GRUB)
ENDIF()

IF (WITH_RESET_MACHINE_ID)
   ADD_SUBDIRECTORY(Reset-Machine-ID)
ENDIF()

IF (WITH_TRY_HARD)
   ADD_SUBDIRECTORY(Try-Hard)
ENDIF()

IF (WITH_RANDOM_SLEEP)
   ADD_SUBDIRECTORY(Random-Sleep)
ENDIF()

IF (WITH_X509_TOOLS)
   ADD_SUBDIRECTORY(X509)
ENDIF()
