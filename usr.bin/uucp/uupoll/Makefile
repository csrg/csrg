#	@(#)Makefile	8.1 (Berkeley) 06/06/93

PROG=	uupoll
CFLAGS+=-I${.CURDIR}/../includes
BINMODE=6555
DPADD=	${LIBCOMPAT}
LDADD=	${LIBUU} -lcompat
MAN8=	uupoll.0

.include <bsd.prog.mk>
