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

ADD_EXECUTABLE(time2unixts time2unixts.c)
# SET_PROPERTY(TARGET time2unixts PROPERTY C_STANDARD 23)
TARGET_LINK_LIBRARIES(time2unixts ${Intl_LIBRARIES})
INSTALL(TARGETS     time2unixts   DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES       time2unixts.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES       time2unixts.bash-completion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME      time2unixts)

ADD_EXECUTABLE(unixts2time unixts2time.c)
# SET_PROPERTY(TARGET unixts2time PROPERTY C_STANDARD 23)
TARGET_LINK_LIBRARIES(unixts2time ${Intl_LIBRARIES})
INSTALL(TARGETS     unixts2time   DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES       unixts2time.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES       unixts2time.bash-completion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME      unixts2time)
