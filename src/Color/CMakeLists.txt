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
#### LIBRARIES                                                           ####
#############################################################################

# ====== Generate color tables ==============================================
ADD_CUSTOM_COMMAND(OUTPUT colors-html.h
                   COMMAND ./make-html-table >colors-html.h
                   DEPENDS html-rgb.txt)
ADD_CUSTOM_TARGET(generate_colors_html_h DEPENDS colors-html.h)

ADD_CUSTOM_COMMAND(OUTPUT colors-x11.h
                   COMMAND ./make-x11-table >colors-x11.h
                   DEPENDS x11-rgb.txt)
ADD_CUSTOM_TARGET(generate_colors_x11_h DEPENDS colors-x11.h)


# ====== libcolor-tables ====================================================
LIST(APPEND libcolor-tables_headers
   color-tables.h
)
LIST(APPEND libcolor-tables_sources
   color-tables.c
   colors-html.h
   colors-x11.h
)

INSTALL(FILES ${libcolor-tables_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/system-tools)

ADD_LIBRARY(libcolor-tables-shared SHARED ${libcolor-tables_sources})
ADD_LIBRARY(libcolor-tables-static STATIC ${libcolor-tables_sources})
FOREACH(TYPE shared;static)
   SET_TARGET_PROPERTIES(libcolor-tables-${TYPE} PROPERTIES OUTPUT_NAME color-tables CLEAN_DIRECT_OUTPUT 1)
   SET_TARGET_PROPERTIES(libcolor-tables-${TYPE} PROPERTIES
      VERSION   ${BUILD_VERSION}
      SOVERSION ${BUILD_MAJOR}
   )
   # TARGET_LINK_LIBRARIES (libcolor-tables-${TYPE} m)
   INSTALL(TARGETS libcolor-tables-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()


#############################################################################
#### PROGRAMS                                                            ####
#############################################################################

ADD_EXECUTABLE(print-color print-color.c)
ADD_DEPENDENCIES(print-color generate_colors_html_h generate_colors_x11_h)
# SET_PROPERTY(TARGET print-color PROPERTY C_STANDARD 23)
TARGET_LINK_LIBRARIES(print-color libcolor-tables-shared ${Intl_LIBRARIES})
INSTALL(TARGETS     print-color   DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES       print-color.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
INSTALL(FILES       print-color.bash-completion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME      print-color)
