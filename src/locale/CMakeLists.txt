#############################################################################
#### TRANSLATIONS                                                        ####
#############################################################################

SET(copyrightHolder   "Copyright (C) 2013-2024 by Thomas Dreibholz")
SET(bugTrackerAddress "https://github.com/dreibh/system-tools")

# ====== Create POT file (translation template) =============================
FILE(GLOB sources "*.sources")

SET(LIST potFiles "")
FOREACH(source IN LISTS sources)
   STRING(REGEX REPLACE ".sources" ".pot" potFile "${source}")
   GET_FILENAME_COMPONENT(textdomain "${source}" NAME_WE)

   FILE(READ "${source}" potInputFiles)
   STRING(REGEX REPLACE "\n" ";" potInputFiles "${potInputFiles}")
   SET(LIST potInputFilesAbsolute "")
   FOREACH(file ${potInputFiles})
      LIST(APPEND potInputFilesAbsolute ${potInputFilesAbsolute} "${PROJECT_SOURCE_DIR}/${file}")
   ENDFOREACH()

   MESSAGE("Translation template: ${textdomain}")
   ADD_CUSTOM_COMMAND(OUTPUT "${potFile}"
                      COMMAND "${XGETTEXT}"
                         --language Shell
                         --from-code utf-8
                         --default-domain="${textdomain}"
                         --add-comments=TRANSLATORS:
                         --files-from "${source}"
                         --package-name="${PROJECT_NAME}"
                         --package-version="${BUILD_VERSION}"
                         --copyright-holder="${copyrightHolder}"
                         --msgid-bugs-address="${bugTrackerAddress}"
                         -p "${CMAKE_CURRENT_SOURCE_DIR}"
                         -o "${potFile}"
                         ${potInputFilesAbsolute}
                       COMMAND sed -e "s/charset=CHARSET/charset=UTF-8/g" -i "${potFile}"
                       WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                       DEPENDS ${potInputFilesAbsolute}
                       VERBATIM)

    LIST(APPEND potFiles "${potFile}")
ENDFOREACH()

ADD_CUSTOM_TARGET(update-pot DEPENDS ${potFiles})


# ====== Update PO files (translations) =====================================
FOREACH(potFile IN LISTS potFiles)
   GET_FILENAME_COMPONENT(textdomain "${potFile}" NAME_WE)
   FILE(GLOB_RECURSE poFiles "*/${textdomain}.po")
   FOREACH(poFile IN LISTS poFiles)
      GET_FILENAME_COMPONENT(languageDirectory "${poFile}" DIRECTORY)
      GET_FILENAME_COMPONENT(language "${languageDirectory}" NAME "${CMAKE_CURRENT_SOURCE_DIR}")

      MESSAGE("Translation update: ${textdomain}, ${language}")
      ADD_CUSTOM_COMMAND(OUTPUT "${poFile}"
                         COMMAND "${MSGMERGE}"
                            --update "${poFile}" "${potFile}"
                         WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                         DEPENDS "${potFile}")
   ENDFOREACH()
ENDFOREACH()

ADD_CUSTOM_TARGET(update-po DEPENDS ${poFiles})


# ====== Compile PO files (translations) ====================================
SET(LIST moFiles "")
FOREACH(potFile IN LISTS potFiles)
   GET_FILENAME_COMPONENT(textdomain "${potFile}" NAME_WE)
   FILE(GLOB_RECURSE poFiles "*/${textdomain}.po")
   FOREACH(poFile IN LISTS poFiles)
      GET_FILENAME_COMPONENT(languageDirectory "${poFile}" DIRECTORY)
      GET_FILENAME_COMPONENT(language "${languageDirectory}" NAME "${CMAKE_CURRENT_SOURCE_DIR}")
      FILE(MAKE_DIRECTORY "${languageDirectory}/LC_MESSAGES")
      SET(moFile "${languageDirectory}/LC_MESSAGES/${textdomain}.mo")

      MESSAGE("Compiling translation: ${textdomain}, ${language} (${poFile} -> ${moFile})")
      ADD_CUSTOM_COMMAND(OUTPUT "${moFile}"
                         COMMAND "${MSGFMT}"
                            --statistics
                            --check
                            --output-file="${moFile}"
                            ${poFile}
                         WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                         DEPENDS "${poFile}")
      LIST(APPEND moFiles "${moFile}")
   ENDFOREACH()
ENDFOREACH()

ADD_CUSTOM_TARGET(update-mo DEPENDS ${moFiles})


#############################################################################
#### MO FILE INSTALLATION                                                ####
#############################################################################

FOREACH(poFile IN LISTS poFiles)
   GET_FILENAME_COMPONENT(languageDirectory "${poFile}" DIRECTORY)
   GET_FILENAME_COMPONENT(language "${languageDirectory}" NAME "${CMAKE_CURRENT_SOURCE_DIR}")

   SET(moFile "${language}/LC_MESSAGES/${textdomain}.mo")
   INSTALL(FILES "${moFile}" DESTINATION "${CMAKE_INSTALL_DATADIR}/locale/${moFile}")
ENDFOREACH()
