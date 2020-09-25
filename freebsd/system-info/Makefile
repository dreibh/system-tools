# Created by: Thomas Dreibholz <dreibh@iem.uni-due.de>
# $FreeBSD$

PORTNAME=	system-info
PORTVERSION=	1.0.0~rc3.2
CATEGORIES=	net
MASTER_SITES=	https://www.uni-due.de/~be0001/system-info/download/

MAINTAINER=	dreibh@iem.uni-due.de
COMMENT=	Print basic system information and banners

LICENSE=	GPLv3+
LICENSE_FILE=	${WRKSRC}/COPYING

USES=		tar:xz cmake
CMAKE_ARGS+=	-DCMAKE_INSTALL_MANDIR=${MANPREFIX}/man

PLIST_FILES=	bin/System-Info \
		man/man1/System-Info.1.gz \
		etc/system-info/01-example


.include <bsd.port.mk>
