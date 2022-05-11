#############################################################################
# PROGRAMS
#############################################################################

INSTALL(PROGRAMS System-Info DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES System-Info.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

INSTALL(PROGRAMS System-Maintenance DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES System-Maintenance.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

INSTALL(PROGRAMS configure-grub DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES configure-grub.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)


#############################################################################
# CONFIGURATION FILES
#############################################################################

INSTALL(PROGRAMS system-info.d/01-example DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/system-info.d)

INSTALL(PROGRAMS system-maintenance.d/XX-example DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/system-maintenance.d)

INSTALL(FILES profile.d/system-info.sh  DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/profile.d)
INSTALL(FILES profile.d/system-info.csh DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/profile.d)


#############################################################################
# EXAMPLES
#############################################################################

INSTALL(FILES grub-defaults-nornet   DESTINATION ${CMAKE_INSTALL_DATADIR}/configure-grub)
INSTALL(FILES grub-defaults-standard DESTINATION ${CMAKE_INSTALL_DATADIR}/configure-grub)
