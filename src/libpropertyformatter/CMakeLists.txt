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
# Property Formatter Library
# Copyright (C) 2026 by Thomas Dreibholz
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


SET(libpropertyformatter_headers propertyformatter.h)
SET(libpropertyformatter_sources propertyformatter.cc)

INSTALL(FILES       ${libpropertyformatter_headers}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# --- Library targets -------------------------------------------------------
IF (WITH_STATIC_LIBRARIES)
   ADD_LIBRARY(libpropertyformatter-static STATIC ${libpropertyformatter_sources})
ENDIF()
IF (WITH_SHARED_LIBRARIES)
   ADD_LIBRARY(libpropertyformatter-shared SHARED ${libpropertyformatter_sources})
ENDIF()

FOREACH(TYPE shared;static)
   IF ( (("${TYPE}" STREQUAL "static") AND (WITH_STATIC_LIBRARIES)) OR
        (("${TYPE}" STREQUAL "shared") AND (WITH_SHARED_LIBRARIES)) )
      SET_TARGET_PROPERTIES(libpropertyformatter-${TYPE} PROPERTIES
         OUTPUT_NAME                propertyformatter
         CLEAN_DIRECT_OUTPUT        1
         VERSION                    ${PROJECT_VERSION}
         SOVERSION                  ${PROJECT_VERSION_MAJOR}
         WINDOWS_EXPORT_ALL_SYMBOLS ON
      )
      TARGET_INCLUDE_DIRECTORIES(libpropertyformatter-${TYPE} PUBLIC
                                 ${CMAKE_CURRENT_SOURCE_DIR})
      IF (NOT MSVC)
         TARGET_COMPILE_OPTIONS(libpropertyformatter-${TYPE} PRIVATE -Wall -Wextra -Wpedantic -Wsign-conversion)
      ELSE()
         TARGET_COMPILE_OPTIONS(libpropertyformatter-${TYPE} PRIVATE /W4)
      ENDIF()

      TARGET_LINK_LIBRARIES(libpropertyformatter-${TYPE} PUBLIC libdisplaywidth-${TYPE})

      INSTALL(TARGETS             libpropertyformatter-${TYPE}
              RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
              ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
              LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
   ENDIF()
ENDFOREACH()

# --- Generate pkg-config metadata ------------------------------------------
CONFIGURE_FILE(
   ${CMAKE_CURRENT_SOURCE_DIR}/libpropertyformatter.pc.in
   ${CMAKE_CURRENT_BINARY_DIR}/libpropertyformatter.pc
   @ONLY
)
INSTALL(FILES       ${CMAKE_CURRENT_BINARY_DIR}/libpropertyformatter.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
