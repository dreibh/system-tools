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
#### LIBRARIES                                                           ####
#############################################################################

# ====== libsysteminfo ======================================================
IF (WITH_LIBSYSTEMINFO)
   LIST(APPEND libsysteminfo_headers libsysteminfo.h)
   LIST(APPEND libsysteminfo_sources
      libsysteminfo-management.cc
      libsysteminfo-system.cc
      libsysteminfo-memory.cc
      libsysteminfo-network.cc
      libsysteminfo-storage.cc
      libsysteminfo-battery.cc
      libsysteminfo-user.cc
   )
   INSTALL(FILES       ${libsysteminfo_headers}
           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hipercontracer)

   # --- Assemble library dependencies --------------------------------------
   SET(libsysteminfo_dependencies
      ${SYSTEMD_LIBRARY}
      ${Intl_LIBRARIES}
      ${SOCKET_LIBRARY}
      ${NSL_LIBRARY}
      ${KSTAT_LIBRARY}
   )
   IF (APPLE)
      LIST(APPEND libsysteminfo_dependencies
           "-framework IOKit"
           "-framework CoreFoundation")
   ELSEIF (WIN32)
      LIST(APPEND libsysteminfo_dependencies ws2_32 iphlpapi wtsapi32 psapi)
   ENDIF()

   # --- Library targets ----------------------------------------------------
   IF (WITH_STATIC_LIBRARIES)
      ADD_LIBRARY(libsysteminfo-static STATIC ${libsysteminfo_sources})
   ENDIF()
   IF (WITH_SHARED_LIBRARIES)
      ADD_LIBRARY(libsysteminfo-shared SHARED ${libsysteminfo_sources})
   ENDIF()

   FOREACH(TYPE shared;static)
      IF ( (("${TYPE}" STREQUAL "static") AND (WITH_STATIC_LIBRARIES)) OR
           (("${TYPE}" STREQUAL "shared") AND (WITH_SHARED_LIBRARIES)) )
         SET_TARGET_PROPERTIES(libsysteminfo-${TYPE} PROPERTIES
            OUTPUT_NAME                systeminfo
            CLEAN_DIRECT_OUTPUT        1
            VERSION                    ${BUILD_VERSION}
            SOVERSION                  ${BUILD_MAJOR}
            WINDOWS_EXPORT_ALL_SYMBOLS ON
         )
         TARGET_COMPILE_DEFINITIONS(libsysteminfo-${TYPE} PRIVATE _FILE_OFFSET_BITS=64 _TIME_BITS=64)
         TARGET_INCLUDE_DIRECTORIES(libsysteminfo-${TYPE} PRIVATE
                                    ${CMAKE_CURRENT_SOURCE_DIR}
                                    ${SYSTEMD_INCLUDE_DIRS})
         IF (NOT MSVC)
            TARGET_COMPILE_OPTIONS(libsysteminfo-${TYPE} PRIVATE -Wall -Wextra -Wpedantic -Wsign-conversion)
         ELSE()
            TARGET_COMPILE_OPTIONS(libsysteminfo-${TYPE} PRIVATE /W4)
         ENDIF()

         TARGET_LINK_LIBRARIES(libsysteminfo-${TYPE} PRIVATE ${libsysteminfo_dependencies})

         INSTALL(TARGETS             libsysteminfo-${TYPE}
                 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                 ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
      ENDIF()
   ENDFOREACH()

   # --- Generate pkg-config metadata ---------------------------------------
   SET(PC_LIBS_PRIVATE "")
   FOREACH(dependency ${libsysteminfo_dependencies})
      IF (dependency MATCHES "^-")
         STRING(APPEND PC_LIBS_PRIVATE " ${dependency}")
      ELSEIF (IS_ABSOLUTE "${dependency}")
         STRING(APPEND PC_LIBS_PRIVATE " ${dependency}")
      ELSE()
         STRING(APPEND PC_LIBS_PRIVATE " -l${dependency}")
      ENDIF()
   ENDFOREACH()
   STRING(STRIP "${PC_LIBS_PRIVATE}" PC_LIBS_PRIVATE)

   CONFIGURE_FILE(
      ${CMAKE_CURRENT_SOURCE_DIR}/libsysteminfo.pc.in
      ${CMAKE_CURRENT_BINARY_DIR}/libsysteminfo.pc
      @ONLY
   )
   INSTALL(FILES       ${CMAKE_CURRENT_BINARY_DIR}/libsysteminfo.pc
           DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
ENDIF()


#############################################################################
#### PROGRAMS                                                            ####
#############################################################################

IF (STATIC_BUILD)
   SET(libraryType static)
ELSE()
   SET(libraryType shared)
ENDIF()

IF (WITH_SYSTEM_INFO)
   CONFIGURE_FILE(System-Info System-Info.versioned @ONLY)
   SET_PROPERTY(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES System-Info.versioned)
   INSTALL(PROGRAMS    ${CMAKE_CURRENT_BINARY_DIR}/System-Info.versioned
           DESTINATION ${CMAKE_INSTALL_BINDIR}
           RENAME      System-Info)
   INSTALL(FILES System-Info.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
   INSTALL(FILES       System-Info.bash-completion
           DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
           RENAME      System-Info)
ENDIF()

IF (WITH_GET_SYSTEM_INFO)
   ADD_EXECUTABLE(get-system-info get-system-info.cc)
   TARGET_COMPILE_DEFINITIONS(get-system-info PRIVATE _FILE_OFFSET_BITS=64 _TIME_BITS=64)
   TARGET_INCLUDE_DIRECTORIES(get-system-info PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${SYSTEMD_INCLUDE_DIRS} ${Intl_INCLUDE_DIRS})
   TARGET_LINK_LIBRARIES(get-system-info PRIVATE
                         libsysteminfo-${libraryType}
                         ${Intl_LIBRARIES}
                         ${GETOPT_LIBRARY})
   IF (WIN32)
      TARGET_LINK_LIBRARIES(get-system-info PRIVATE ws2_32)
   ENDIF()
   IF (NOT MSVC)
      TARGET_COMPILE_OPTIONS(get-system-info PRIVATE -Wall -Wextra -Wpedantic -Wsign-conversion)
   ELSE()
      TARGET_COMPILE_OPTIONS(get-system-info PRIVATE /W4)
   ENDIF()
   INSTALL(TARGETS get-system-info DESTINATION ${CMAKE_INSTALL_BINDIR})
   INSTALL(FILES get-system-info.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
   INSTALL(FILES       get-system-info.bash-completion
           DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
           RENAME      get-system-info)
ENDIF()


#############################################################################
#### CONFIGURATION FILES                                                 ####
#############################################################################

IF (WITH_SYSTEM_INFO)
   INSTALL(PROGRAMS system-info.d/banner-helper DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/system-info.d)

   IF (WITH_SYSTEM_INFO_DEFAULT_BANNER)
      INSTALL(PROGRAMS system-info.d/01-example DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/system-info.d)
   ENDIF()

   IF (WITH_SYSTEM_INFO_IN_PROFILES)
      INSTALL(FILES profile.d/system-info.sh    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/profile.d)
      INSTALL(FILES profile.d/system-info.csh   DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/profile.d)
   ENDIF()
ENDIF()


#############################################################################
#### EXAMPLES                                                            ####
#############################################################################

IF (WITH_SYSTEM_INFO)
   IF (WITH_SYSTEM_INFO_EXAMPLES)
      INSTALL(PROGRAMS system-info.d/01-example              DESTINATION ${CMAKE_INSTALL_DATADIR}/System-Info)
      INSTALL(PROGRAMS system-info.d/09-hostname-example     DESTINATION ${CMAKE_INSTALL_DATADIR}/System-Info)
      INSTALL(PROGRAMS system-info.d/10-company-logo-example DESTINATION ${CMAKE_INSTALL_DATADIR}/System-Info)
      INSTALL(PROGRAMS system-info.d/banner-helper           DESTINATION ${CMAKE_INSTALL_DATADIR}/System-Info)
   ENDIF()
ENDIF()
