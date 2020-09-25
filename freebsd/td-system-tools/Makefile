# Created by: Thomas Dreibholz <dreibh@iem.uni-due.de>
# $FreeBSD$

PORTNAME=	td-system-tools
PORTVERSION=	1.0.0~rc3.2
CATEGORIES=	net
MASTER_SITES=	https://www.uni-due.de/~be0001/system-tools/download/

MAINTAINER=	dreibh@iem.uni-due.de
COMMENT=	Printing basic system information and system maintenance

LICENSE=	GPLv3+
LICENSE_FILE=	${WRKSRC}/COPYING

USES=		tar:xz cmake
CMAKE_ARGS+=	-DCMAKE_INSTALL_MANDIR=${MANPREFIX}/man

PLIST_FILES=	bin/System-Info \
		bin/System-Maintenance \
		man/man1/System-Info.1.gz \
		man/man1/SystemMaintenance.1.gz \
		etc/system-tools.d/01-example \
		etc/system-maintenance.d/XX-example


.include <bsd.port.mk>
