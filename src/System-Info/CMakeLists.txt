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
#### PROGRAMS                                                            ####
#############################################################################

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
   ADD_EXECUTABLE(get-system-info get-system-info.c)
   # SET_PROPERTY(TARGET get-system-info PROPERTY C_STANDARD 23)
   TARGET_INCLUDE_DIRECTORIES(get-system-info PRIVATE ${Intl_INCLUDE_DIRS})
   TARGET_LINK_LIBRARIES(get-system-info ${Intl_LIBRARIES})
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
