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
# This script is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This script is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this script.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact: thomas.dreibholz@gmail.com


#############################################################################
#### PROGRAMS                                                            ####
#############################################################################

SET(SCRIPTS
   check-certificate
   der2pem
   extract-pem
   pem2der
   test-tls-connection
   view-certificate
   view-crl
)

FOREACH(script IN LISTS SCRIPTS)
   CONFIGURE_FILE(${script} ${script}.versioned @ONLY)
   INSTALL(PROGRAMS    ${CMAKE_CURRENT_BINARY_DIR}/${script}.versioned
           DESTINATION ${CMAKE_INSTALL_BINDIR}
           RENAME      ${script})
   INSTALL(FILES ${script}.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
   INSTALL(FILES       ${script}.bash-completion
           DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
           RENAME      ${script})
ENDFOREACH()


#############################################################################
#### EXAMPLES                                                            ####
#############################################################################

INSTALL(FILES
           CertificateHelper.py
        DESTINATION ${CMAKE_INSTALL_DATADIR}/system-tools)
INSTALL(PROGRAMS
           generate-test-certificates
           make-test-certificates
        DESTINATION ${CMAKE_INSTALL_DATADIR}/system-tools)
