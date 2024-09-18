# System-Tools
# Copyright (C) 2013-2024 by Thomas Dreibholz
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


#############################################################################
#### PROGRAMS                                                            ####
#############################################################################

INSTALL(PROGRAMS System-Info DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES System-Info.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

INSTALL(PROGRAMS System-Maintenance DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES System-Maintenance.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

INSTALL(PROGRAMS Reset-Machine-ID DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES Reset-Machine-ID.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

INSTALL(PROGRAMS Fingerprint-SSH-Keys DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES Fingerprint-SSH-Keys.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

INSTALL(PROGRAMS configure-grub DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES configure-grub.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(random-sleep random-sleep.c)
INSTALL(TARGETS random-sleep DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES random-sleep.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

INSTALL(PROGRAMS try-hard DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES try-hard.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)


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
