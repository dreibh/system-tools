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
# Copyright (C) 2013-2025 by Thomas Dreibholz
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
)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})


#############################################################################
#### PROGRAMS                                                            ####
#############################################################################

INSTALL(PROGRAMS System-Info DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES System-Info.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES       System-Info.bash-completion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME      System-Info)

INSTALL(PROGRAMS System-Maintenance DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES System-Maintenance.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES       System-Maintenance.bash-completion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME      System-Maintenance)

INSTALL(PROGRAMS Reset-Machine-ID DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES Reset-Machine-ID.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES       Reset-Machine-ID.bash-completion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME      Reset-Machine-ID)

INSTALL(PROGRAMS Fingerprint-SSH-Keys DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES Fingerprint-SSH-Keys.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES       Fingerprint-SSH-Keys.bash-completion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME      Fingerprint-SSH-Keys)

INSTALL(PROGRAMS configure-grub DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES configure-grub.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES       configure-grub.bash-completion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME      configure-grub)

ADD_EXECUTABLE(get-system-info get-system-info.c)
SET_PROPERTY(TARGET get-system-info PROPERTY C_STANDARD 11)
INSTALL(TARGETS get-system-info DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES get-system-info.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES       get-system-info.bash-completion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME      get-system-info)

ADD_EXECUTABLE(print-utf8 print-utf8.c)
SET_PROPERTY(TARGET print-utf8 PROPERTY C_STANDARD 11)
INSTALL(TARGETS print-utf8 DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES print-utf8.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES              print-utf8.bash-completion
        DESTINATION        ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME             print-utf8)

ADD_EXECUTABLE(random-sleep random-sleep.c)
TARGET_LINK_LIBRARIES(random-sleep ${Intl_LIBRARIES})
INSTALL(TARGETS     random-sleep   DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES       random-sleep.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES       random-sleep.bash-completion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME      random-sleep)

INSTALL(PROGRAMS    try-hard   DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES       try-hard.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES       try-hard.bash-completion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME      try-hard)

ADD_EXECUTABLE(text-block text-block.c)
SET_PROPERTY(TARGET text-block PROPERTY C_STANDARD 11)
INSTALL(PROGRAMS    text-block   DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES       text-block.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES       text-block.bash-completion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME      text-block)


#############################################################################
#### CONFIGURATION FILES                                                 ####
#############################################################################

INSTALL(PROGRAMS system-info.d/01-example    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/system-info.d)
INSTALL(PROGRAMS system-info.d/banner-helper DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/system-info.d)

INSTALL(PROGRAMS system-maintenance.d/XX-example DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/system-maintenance.d)

INSTALL(FILES profile.d/system-info.sh  DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/profile.d)
INSTALL(FILES profile.d/system-info.csh DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/profile.d)


#############################################################################
#### EXAMPLES                                                            ####
#############################################################################

INSTALL(PROGRAMS system-info.d/01-example              DESTINATION ${CMAKE_INSTALL_DATADIR}/System-Info)
INSTALL(PROGRAMS system-info.d/09-hostname-example     DESTINATION ${CMAKE_INSTALL_DATADIR}/System-Info)
INSTALL(PROGRAMS system-info.d/10-company-logo-example DESTINATION ${CMAKE_INSTALL_DATADIR}/System-Info)
INSTALL(PROGRAMS system-info.d/banner-helper           DESTINATION ${CMAKE_INSTALL_DATADIR}/System-Info)

INSTALL(FILES grub-defaults-nornet   DESTINATION ${CMAKE_INSTALL_DATADIR}/configure-grub)
INSTALL(FILES grub-defaults-standard DESTINATION ${CMAKE_INSTALL_DATADIR}/configure-grub)
