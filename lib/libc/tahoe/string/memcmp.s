/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * This code is derived from software contributed to Berkeley by
 * Computer Consoles Inc.
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)memcmp.s	5.1 (Berkeley) 05/15/90"
#endif /* LIBC_SCCS and not lint */

/*
 * int
 * memcmp(s1, s2, n)
 *	void const *s1, const *s2;
 *	size_t n;
 *
 * Compare up to n bytes from s1/s2.
 * Return
 *	= 0 if s1 == s2
 *	< 0 if s1 < s2		(unsigned character comparison)
 *	> 0 if s1 > s2
 */
#include "DEFS.h"

ENTRY(memcmp, 0)
	movl	4(fp),r0
	movl	8(fp),r1
	mnegl	12(fp),r2
	jeql	2f
1:
	cmpb	(r0),(r1)
	jneq	3f
	incl	r0
	incl	r1
	aoblss	$0,r2,1b
2:
	clrl	r0
	ret
3:
	jlssu	4f
	movl	$1,r0
	ret
4:
	mnegl	$1,r0
	ret
