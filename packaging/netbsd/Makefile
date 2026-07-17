# $NetBSD$

DISTNAME=	td-system-tools-2.7.10
CATEGORIES=	net
MASTER_SITES=	https://www.nntb.no/~dreibh/system-tools/download/
EXTRACT_SUFX=	.tar.xz

MAINTAINER=	thomas.dreibholz@gmail.com
HOMEPAGE=	https://www.nntb.no/~dreibh/system-tools/
COMMENT=	Tools for basic system management and maintenance
LICENSE=	gnu-gpl-v3

USE_LANGUAGES=	c c++
USE_CMAKE=	yes
USE_TOOLS+=	pkg-config bash:run

# Require C23 standard
FORCE_C_STD=	c23

# Shebang replacement for bash scripts
REPLACE_BASH+=	src/Fingerprint-SSH-Keys/Fingerprint-SSH-Keys
REPLACE_BASH+=	src/Reset-Machine-ID/Reset-Machine-ID
REPLACE_BASH+=	src/System-Info/System-Info
REPLACE_BASH+=	src/System-Maintenance/System-Maintenance
REPLACE_BASH+=	src/configure-grub/configure-grub
REPLACE_BASH+=	src/get-system-info/get-system-info

DEPENDS+=	getopt-[0-9]*:../../misc/getopt

.include "options.mk"

.include "../../devel/gettext-lib/buildlink3.mk"
.include "../../mk/bsd.pkg.mk"
