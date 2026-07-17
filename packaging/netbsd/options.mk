# $NetBSD$

PKG_OPTIONS_VAR=	PKG_OPTIONS.td-system-tools
PKG_SUPPORTED_OPTIONS=	configure-grub fingerprint-ssh-keys get-system-info gimp-scripts gimp-scripts-examples nls print-utf8 random-sleep reset-machine-id system-info system-info-default-banner system-info-examples system-info-in-profiles system-maintenance text-block try-hard unix-timestamp-tools x509-tools
PKG_SUGGESTED_OPTIONS=	fingerprint-ssh-keys get-system-info nls print-utf8 random-sleep reset-machine-id system-info system-info-default-banner system-info-examples system-info-in-profiles system-maintenance text-block try-hard unix-timestamp-tools x509-tools

.include "../../mk/bsd.options.mk"

PLIST_VARS+=	CONFIGURE_GRUB FINGERPRINT_SSH_KEYS GET_SYSTEM_INFO GIMP_SCRIPTS GIMP_SCRIPTS_EXAMPLES NLS PRINT_UTF8 RANDOM_SLEEP RESET_MACHINE_ID SYSTEM_INFO SYSTEM_INFO_DEFAULT_BANNER SYSTEM_INFO_EXAMPLES SYSTEM_INFO_IN_PROFILES SYSTEM_MAINTENANCE TEXT_BLOCK TRY_HARD UNIX_TIMESTAMP_TOOLS X509_TOOLS

# Handle option implications (dependencies)
.if !empty(PKG_OPTIONS:Mgimp-scripts-examples)
PKG_OPTIONS+=		gimp-scripts
.endif

.if !empty(PKG_OPTIONS:Msystem-info-default-banner) || !empty(PKG_OPTIONS:Msystem-info-examples) || !empty(PKG_OPTIONS:Msystem-info-in-profiles)
PKG_OPTIONS+=		system-info
.endif

.if !empty(PKG_OPTIONS:Msystem-info)
PKG_OPTIONS+=		get-system-info print-utf8
.endif

.if !empty(PKG_OPTIONS:Mtry-hard)
PKG_OPTIONS+=		random-sleep
.endif

.if !empty(PKG_OPTIONS:Mx509-tools)
PKG_OPTIONS+=		print-utf8 text-block unix-timestamp-tools
.endif

# Option: configure-grub
.if !empty(PKG_OPTIONS:Mconfigure-grub)
CMAKE_CONFIGURE_ARGS+=	-DWITH_CONFIGURE_GRUB:BOOL=TRUE
PLIST.CONFIGURE_GRUB=	yes
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_CONFIGURE_GRUB:BOOL=FALSE
.endif

# Option: fingerprint-ssh-keys
.if !empty(PKG_OPTIONS:Mfingerprint-ssh-keys)
CMAKE_CONFIGURE_ARGS+=	-DWITH_FINGERPRINT_SSH_KEYS:BOOL=TRUE
PLIST.FINGERPRINT_SSH_KEYS=	yes
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_FINGERPRINT_SSH_KEYS:BOOL=FALSE
.endif

# Option: get-system-info
.if !empty(PKG_OPTIONS:Mget-system-info)
CMAKE_CONFIGURE_ARGS+=	-DWITH_GET_SYSTEM_INFO:BOOL=TRUE
PLIST.GET_SYSTEM_INFO=	yes
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_GET_SYSTEM_INFO:BOOL=FALSE
.endif

# Option: gimp-scripts
.if !empty(PKG_OPTIONS:Mgimp-scripts)
CMAKE_CONFIGURE_ARGS+=	-DWITH_GIMP_SCRIPTS:BOOL=TRUE
PLIST.GIMP_SCRIPTS=	yes
DEPENDS+=		fontconfig-[0-9]*:../../x11/fontconfig
DEPENDS+=		gimp-[0-9]*:../../graphics/gimp
DEPENDS+=		GraphicsMagick-[0-9]*:../../graphics/GraphicsMagick
DEPENDS+=		noto-fonts-[0-9]*:../../fonts/noto-fonts
DEPENDS+=		open-sans-[0-9]*:../../fonts/open-sans
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_GIMP_SCRIPTS:BOOL=FALSE
.endif

# Option: gimp-scripts-examples
.if !empty(PKG_OPTIONS:Mgimp-scripts-examples)
CMAKE_CONFIGURE_ARGS+=	-DWITH_GIMP_SCRIPTS_EXAMPLES:BOOL=TRUE
PLIST.GIMP_SCRIPTS_EXAMPLES=	yes
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_GIMP_SCRIPTS_EXAMPLES:BOOL=FALSE
.endif

# Option: nls
.if !empty(PKG_OPTIONS:Mnls)
CMAKE_CONFIGURE_ARGS+=	-DWITH_I18N:BOOL=TRUE
PLIST.NLS=		yes
USE_TOOLS+=		msgfmt
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_I18N:BOOL=FALSE
.endif

# Option: print-utf8
.if !empty(PKG_OPTIONS:Mprint-utf8)
CMAKE_CONFIGURE_ARGS+=	-DWITH_PRINT_UTF8:BOOL=TRUE
PLIST.PRINT_UTF8=	yes
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_PRINT_UTF8:BOOL=FALSE
.endif

# Option: random-sleep
.if !empty(PKG_OPTIONS:Mrandom-sleep)
CMAKE_CONFIGURE_ARGS+=	-DWITH_RANDOM_SLEEP:BOOL=TRUE
PLIST.RANDOM_SLEEP=	yes
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_RANDOM_SLEEP:BOOL=FALSE
.endif

# Option: reset-machine-id
.if !empty(PKG_OPTIONS:Mreset-machine-id)
CMAKE_CONFIGURE_ARGS+=	-DWITH_RESET_MACHINE_ID:BOOL=TRUE
PLIST.RESET_MACHINE_ID=	yes
DEPENDS+=		sudo-[0-9]*:../../security/sudo
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_RESET_MACHINE_ID:BOOL=FALSE
.endif

# Option: system-info
.if !empty(PKG_OPTIONS:Msystem-info)
CMAKE_CONFIGURE_ARGS+=	-DWITH_SYSTEM_INFO:BOOL=TRUE
PLIST.SYSTEM_INFO=	yes
DEPENDS+=		figlet-[0-9]*:../../misc/figlet
DEPENDS+=		mbuffer-[0-9]*:../../misc/mbuffer
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_SYSTEM_INFO:BOOL=FALSE
.endif

# Option: system-info-default-banner
.if !empty(PKG_OPTIONS:Msystem-info-default-banner)
CMAKE_CONFIGURE_ARGS+=	-DWITH_SYSTEM_INFO_DEFAULT_BANNER:BOOL=TRUE
PLIST.SYSTEM_INFO_DEFAULT_BANNER=	yes
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_SYSTEM_INFO_DEFAULT_BANNER:BOOL=FALSE
.endif

# Option: system-info-examples
.if !empty(PKG_OPTIONS:Msystem-info-examples)
CMAKE_CONFIGURE_ARGS+=	-DWITH_SYSTEM_INFO_EXAMPLES:BOOL=TRUE
PLIST.SYSTEM_INFO_EXAMPLES=	yes
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_SYSTEM_INFO_EXAMPLES:BOOL=FALSE
.endif

# Option: system-info-in-profiles
.if !empty(PKG_OPTIONS:Msystem-info-in-profiles)
CMAKE_CONFIGURE_ARGS+=	-DWITH_SYSTEM_INFO_IN_PROFILES:BOOL=TRUE
PLIST.SYSTEM_INFO_IN_PROFILES=	yes
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_SYSTEM_INFO_IN_PROFILES:BOOL=FALSE
.endif

# Option: system-maintenance
.if !empty(PKG_OPTIONS:Msystem-maintenance)
CMAKE_CONFIGURE_ARGS+=	-DWITH_SYSTEM_MAINTENANCE:BOOL=TRUE
PLIST.SYSTEM_MAINTENANCE=	yes
DEPENDS+=		sudo-[0-9]*:../../security/sudo
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_SYSTEM_MAINTENANCE:BOOL=FALSE
.endif

# Option: text-block
.if !empty(PKG_OPTIONS:Mtext-block)
CMAKE_CONFIGURE_ARGS+=	-DWITH_TEXT_BLOCK:BOOL=TRUE
PLIST.TEXT_BLOCK=	yes
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_TEXT_BLOCK:BOOL=FALSE
.endif

# Option: try-hard
.if !empty(PKG_OPTIONS:Mtry-hard)
CMAKE_CONFIGURE_ARGS+=	-DWITH_TRY_HARD:BOOL=TRUE
PLIST.TRY_HARD=	yes
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_TRY_HARD:BOOL=FALSE
.endif

# Option: unix-timestamp-tools
.if !empty(PKG_OPTIONS:Munix-timestamp-tools)
CMAKE_CONFIGURE_ARGS+=	-DWITH_UNIX_TIMESTAMP_TOOLS:BOOL=TRUE
PLIST.UNIX_TIMESTAMP_TOOLS=	yes
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_UNIX_TIMESTAMP_TOOLS:BOOL=FALSE
.endif

# Option: x509-tools
.if !empty(PKG_OPTIONS:Mx509-tools)
CMAKE_CONFIGURE_ARGS+=	-DWITH_X509_TOOLS:BOOL=TRUE
PLIST.X509_TOOLS=	yes
DEPENDS+=		mbuffer-[0-9]*:../../misc/mbuffer
REPLACE_PYTHON+=	src/X509/generate-test-certificates
.include "../../lang/python/application.mk"
.include "../../security/openssl/buildlink3.mk"
.else
CMAKE_CONFIGURE_ARGS+=	-DWITH_X509_TOOLS:BOOL=FALSE
.endif
