#############################################################################
# PROGRAMS
#############################################################################

INSTALL(PROGRAMS System-Info DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES System-Info.1  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)


#############################################################################
# CONFIGURATION FILES
#############################################################################

INSTALL(PROGRAMS 01-example DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/system-info.d)
