/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: itereg.h 1.2 89/08/25$
 *
 *	@(#)itereg.h	7.1 (Berkeley) 05/08/90
 */

#define REGADDR		ip->regbase
#define FBBASE		((volatile u_char *)ip->fbbase)
#define	FONTROM		((u_char *)(REGADDR+0x3B))
#define ITEREGS		((struct iteregs *)(REGADDR))

/*
 * All of the HP displays use the same font ROM setup. These structures
 * are used to get at them.
 */

struct	iteregs {
	u_short	reset;
	u_short	interrupt;
	u_char	:8,
		fbwidth_h,
		:8,
		fbwidth_l,
		:8,
		fbheight_h,
		:8,
		fbheight_l,
		:8,
		dispwidth_h,
		:8,
		dispwidth_l,
		:8,
		dispheight_h,
		:8,
		dispheight_l;
};

struct	fontinfo {
	u_char	nfonts,	:8,
		fontid,	:8,
		haddr,	:8,
		laddr,	:8;
};

struct	font {
	u_char	fh,	:8,
		fw;
	u_char	pad[7],
		data[256];
};

#define draw_cursor(ip) { \
	WINDOWMOVER(ip, ip->cblanky, ip->cblankx, \
		    ip->cury * ip->ftheight, \
		    ip->curx * ip->ftwidth, \
		    ip->ftheight, ip->ftwidth, RR_XOR); \
        ip->cursorx = ip->curx; \
	ip->cursory = ip->cury; }

#define erase_cursor(ip) \
  	WINDOWMOVER(ip, ip->cblanky, ip->cblankx, \
		    ip->cursory * ip->ftheight, \
		    ip->cursorx * ip->ftwidth, \
		    ip->ftheight, ip->ftwidth, RR_XOR);
