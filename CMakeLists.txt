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
# Contact: dreibh@simula.no


CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(td-system-tools)
ENABLE_LANGUAGE(C)

SET(BUILD_MAJOR "2")
SET(BUILD_MINOR "4")
SET(BUILD_PATCH "1~rc1.2")
SET(BUILD_VERSION ${BUILD_MAJOR}.${BUILD_MINOR}.${BUILD_PATCH})


#############################################################################
#### INSTALLATION_DIRECTORIES                                            ####
#############################################################################

# See: https://cmake.org/cmake/help/v3.0/module/GNUInstallDirs.html
INCLUDE(GNUInstallDirs)


#############################################################################
#### PACKAGING                                                           ####
#############################################################################

SET(CPACK_SOURCE_GENERATOR "TXZ")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${BUILD_MAJOR}.${BUILD_MINOR}.${BUILD_PATCH}")
SET(CPACK_SOURCE_IGNORE_FILES "\\\\.git;\\\\.swp$;~$;\\\\.\\\\#;/\\\\#")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "^${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}[_-]")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "\\\\.cmake$|\\\\.make$|\\\\.log$")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "/CMakeCache\\\\.txt$")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "/(CMakeFiles|CMakeScripts|_CPack_Packages)/")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "/package-version\\\\.h$")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "/packaging\\\\.conf$")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "^${PROJECT_SOURCE_DIR}/(po.*/|src.*/|)Makefile$")
INCLUDE(CPack)

ADD_CUSTOM_TARGET(dist COMMAND ${CMAKE_MAKE_PROGRAM} clean package_source)


#############################################################################
#### UNINSTALL                                                           ####
#############################################################################

IF(NOT TARGET uninstall)
   CONFIGURE_FILE(
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
      IMMEDIATE @ONLY)

   ADD_CUSTOM_TARGET(uninstall
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
ENDIF()


#############################################################################
#### OS DEPENDENT                                                        ####
#############################################################################

IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   MESSAGE(STATUS ${CMAKE_SYSTEM_NAME} " supported")

ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
   MESSAGE(STATUS ${CMAKE_SYSTEM_NAME} " supported")
   SET(CMAKE_REQUIRED_INCLUDES "/usr/local/include" "/usr/include")
   SET(CMAKE_LIBRARY_PATH "/usr/local/lib")
   INCLUDE_DIRECTORIES("/usr/local/include")

ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   MESSAGE(STATUS ${CMAKE_SYSTEM_NAME} " supported")
   SET(CMAKE_REQUIRED_INCLUDES "/usr/local/include" "/usr/include" "/usr/local/opt/openssl/include")
   SET(CMAKE_LIBRARY_PATH "/usr/local/lib")
   INCLUDE_DIRECTORIES("/usr/local/include" "/usr/local/opt/openssl/include")

ELSE()
   MESSAGE(FATAL_ERROR ${CMAKE_SYSTEM_NAME} " not supported (yet?)")

ENDIF()


#############################################################################
#### OPTIONS                                                             ####
#############################################################################

OPTION(WITH_I18N "Build with Internationalization (i18n) support" 1)

OPTION(WITH_SYSTEM_INFO           "Include System-Info"          1)
OPTION(WITH_SYSTEM_MAINTENANCE    "Include System-Maintenance"   1)
OPTION(WITH_PRINT_UTF8            "Include Print-UTF8"           1)
OPTION(WITH_TEXT_BLOCK            "Include Text-Block"           1)
OPTION(WITH_FINGERPRINT_SSH_KEYS  "Include Fingerprint-SSH-Keys" 1)
OPTION(WITH_CONFIGURE_GRUB        "Include Configure-GRUB"       1)
OPTION(WITH_RESET_MACHINE_ID      "Include Reset-Machine-ID"     1)
OPTION(WITH_TRY_HARD              "Include Try-Hard"             1)
OPTION(WITH_RANDOM_SLEEP          "Include Random-Sleep"         1)
OPTION(WITH_UNIX_TIMESTAMP_TOOLS  "Include Unix-Timestamp Tools" 1)
OPTION(WITH_X509_TOOLS            "Include X509 Tools"           1)

IF (WITH_SYSTEM_INFO AND NOT WITH_PRINT_UTF8)
   MESSAGE(FATAL_ERROR "WITH_SYSTEM_INFO requires WITH_PRINT_UTF8!")
ENDIF()


#############################################################################
#### INTERNATIONALISATION (I18N)                                         ####
#############################################################################

IF (WITH_I18N)
   FIND_PACKAGE (Intl)
   IF (Intl_FOUND)
      MESSAGE(STATUS "Internationalization (i18n) support found:")
      MESSAGE(STATUS " INTL_INCLUDE_DIRS: ${Intl_INCLUDE_DIRS}")
      MESSAGE(STATUS " INTL_LIBRARIES: ${Intl_LIBRARIES}")
      MESSAGE(STATUS " Version: ${Intl_VERSION}")
      INCLUDE_DIRECTORIES(${Intl_INCLUDE_DIRS})
      LINK_DIRECTORIES(${Intl_LIBRARY_DIRS})
   ELSE()
      MESSAGE(FATAL_ERROR
            " Cannot find Internationalization (i18n) support! Try:\n"
            " * Ubuntu/Debian: sudo apt install -y libc6-dev\n"
            " * Fedora:        sudo dnf install -y gettext-devel\n"
            " * SuSE:          sudo zypper install -y gettext-devel\n"
            " * Alpine:        sudo apk add gettext-dev\n"
            " * FreeBSD:       sudo pkg install -y gettext-runtime")
   ENDIF()

   FIND_PROGRAM(XGETTEXT xgettext REQUIRED)
   FIND_PROGRAM(MSGMERGE msgmerge REQUIRED)
   FIND_PROGRAM(MSGFMT   msgfmt   REQUIRED)

   ADD_DEFINITIONS(-DENABLE_NLS)
ENDIF()


#############################################################################
#### SUBDIRECTORIES                                                      ####
#############################################################################

ADD_SUBDIRECTORY(src)

IF (WITH_I18N)
   ADD_SUBDIRECTORY(po)
ENDIF()
