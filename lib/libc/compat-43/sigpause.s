/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
_sccsid:.asciz	"@(#)sigpause.s	5.2 (Berkeley) 06/05/85"
#endif not lint

#include "SYS.h"

SYSCALL(sigpause)
	ret
