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
                      COMMAND xgettext
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
                         COMMAND msgmerge
                            --update "${poFile}" "${potFile}"
                         WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                         DEPENDS "${potFile}")
   ENDFOREACH()
ENDFOREACH()

ADD_CUSTOM_TARGET(update-po DEPENDS ${poFiles})


# ====== Compile PO files (translations) ====================================
FOREACH(potFile IN LISTS potFiles)
   GET_FILENAME_COMPONENT(textdomain "${potFile}" NAME_WE)
   FILE(GLOB_RECURSE poFiles "*/${textdomain}.po")
   FOREACH(poFile IN LISTS poFiles)
      GET_FILENAME_COMPONENT(languageDirectory "${poFile}" DIRECTORY)
      GET_FILENAME_COMPONENT(language "${languageDirectory}" NAME "${CMAKE_CURRENT_SOURCE_DIR}")
      MESSAGE("Compiling translation: ${textdomain}, ${language} (${poFile} -> ${moFile})")

      FILE(MAKE_DIRECTORY "${languageDirectory}/LC_MESSAGES")
      SET(moFile "${languageDirectory}/LC_MESSAGES/${textdomain}.mo")
      ADD_CUSTOM_COMMAND(OUTPUT "${moFile}"
                         COMMAND msgfmt
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



  ## msgfmt
  #foreach(lang ${languages})
    #add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${lang}.gmo"
                       #COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_CURRENT_BINARY_DIR}/${lang}.gmo"
                       #COMMAND msgfmt -c --statistics -o "${CMAKE_CURRENT_BINARY_DIR}/${lang}.gmo"
                         #"${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po"
                       #COMMAND touch "${CMAKE_CURRENT_BINARY_DIR}/${lang}.gmo"
                       #DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po"
                       #VERBATIM)
    #install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${lang}.gmo"
            #DESTINATION "${SCHROOT_LOCALE_DIR}/${lang}/LC_MESSAGES"
            #RENAME "${domain}.mo")
    #set(gmo_files ${gmo_files} "${CMAKE_CURRENT_BINARY_DIR}/${lang}.gmo")
  #endforeach(lang)

#if(GIT_RELEASE_ENABLE)
  #add_dependencies(git-release update-pot update-po)
#endif(GIT_RELEASE_ENABLE)

  #add_custom_target(update-gmo ALL DEPENDS ${gmo_files})

  #add_custom_target(po-notify
                    #COMMAND podebconf-report-po --call --withtranslators --noforce --podir=${CMAKE_CURRENT_SOURCE_DIR})]]
