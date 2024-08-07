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

CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(td-system-tools)
ENABLE_LANGUAGE(C)

SET(BUILD_MAJOR "1")
SET(BUILD_MINOR "7")
SET(BUILD_PATCH "1")
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
SET(CPACK_SOURCE_IGNORE_FILES
   "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}-*;${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}_*;/build/;/.git/;/config.log;/CMakeFiles/;CMakeCache.txt$;.cmake$;.tar.gz$;/_CPack_Packages;/Makefile$;~$;/build-deb;/clean-deb;/filter-empty-entries;/make-symbols;/make-ppa;/make-deb;/debian.conf;/make-rpm;/rpm.conf;${CPACK_SOURCE_IGNORE_FILES}")
INCLUDE(CPack)

ADD_CUSTOM_TARGET(dist COMMAND ${CMAKE_MAKE_PROGRAM} clean package_source)


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

ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
   MESSAGE(STATUS ${CMAKE_SYSTEM_NAME} " supported")
   SET(CMAKE_REQUIRED_INCLUDES "/usr/pkg/include" "/usr/include" "/usr/local/include")
   SET(CMAKE_LIBRARY_PATH "/usr/local/lib")
   INCLUDE_DIRECTORIES("/usr/pkg/include" "/usr/local/include")

ELSE()
   MESSAGE(FATAL_ERROR ${CMAKE_SYSTEM_NAME} " not supported (yet?)")

ENDIF()


#############################################################################
#### INTERNATIONALISATION (I18N)                                         ####
#############################################################################

FIND_PROGRAM(XGETTEXT xgettext REQUIRED)
FIND_PROGRAM(MSGMERGE msgmerge REQUIRED)
FIND_PROGRAM(MSGFMT   msgfmt   REQUIRED)


#############################################################################
#### SUBDIRECTORIES                                                      ####
#############################################################################

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(po)
