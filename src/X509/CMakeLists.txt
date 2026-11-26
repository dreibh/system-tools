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

# ====== Shell / Python scripts =============================================
SET(SCRIPTS
   check-certificate
   der2pem
   extract-pem
   pem2der
   test-tls-connection
)

FOREACH(script IN LISTS SCRIPTS)
   CONFIGURE_FILE(${script} ${script}.versioned @ONLY)
   SET_PROPERTY(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${script}.versioned)
   INSTALL(PROGRAMS    ${CMAKE_CURRENT_BINARY_DIR}/${script}.versioned
           DESTINATION ${CMAKE_INSTALL_BINDIR}
           RENAME      ${script})
   INSTALL(FILES ${script}.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
   INSTALL(FILES       ${script}.bash-completion
           DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
           RENAME      ${script})
ENDFOREACH()

# ====== Shared X.509 C++ helper object library =============================
ADD_LIBRARY(x509-helpers OBJECT x509-helpers.cc)
TARGET_INCLUDE_DIRECTORIES(x509-helpers PRIVATE
   ${CMAKE_SOURCE_DIR}/src/libdisplaywidth
   ${CMAKE_SOURCE_DIR}/src/libpropertyformatter
   ${CMAKE_BINARY_DIR}/src
   ${Intl_INCLUDE_DIRS}
)
TARGET_LINK_LIBRARIES(x509-helpers PRIVATE OpenSSL::Crypto)

# ====== Compiled C++ programs ==============================================
SET(CXX_PROGRAMS
   view-certificate
   view-crl
)

FOREACH(program IN LISTS CXX_PROGRAMS)
   ADD_EXECUTABLE(${program} ${program}.cc $<TARGET_OBJECTS:x509-helpers>)
   TARGET_INCLUDE_DIRECTORIES(${program} PRIVATE
      ${CMAKE_SOURCE_DIR}/src/libdisplaywidth
      ${CMAKE_SOURCE_DIR}/src/libpropertyformatter
      ${CMAKE_BINARY_DIR}/src
      ${Intl_INCLUDE_DIRS}
   )
   TARGET_LINK_LIBRARIES(${program} PRIVATE
      OpenSSL::Crypto
      ${Intl_LIBRARIES}
   )
   IF(STATIC_BUILD)
      TARGET_LINK_LIBRARIES(${program} PRIVATE libpropertyformatter-static)
   ELSE()
      TARGET_LINK_LIBRARIES(${program} PRIVATE libpropertyformatter-shared)
   ENDIF()
   IF(WIN32)
      TARGET_INCLUDE_DIRECTORIES(${program} PRIVATE ${GETOPT_INCLUDE_DIR})
      TARGET_LINK_LIBRARIES(${program} PRIVATE ${GETOPT_LIBRARY})
   ENDIF()

   INSTALL(TARGETS ${program} DESTINATION ${CMAKE_INSTALL_BINDIR})
   INSTALL(FILES ${program}.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
   INSTALL(FILES       ${program}.bash-completion
           DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
           RENAME      ${program})
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


#############################################################################
#### TESTING                                                             ####
#############################################################################

IF (BUILD_TESTING)
   ADD_SUBDIRECTORY(tests)
ENDIF()
