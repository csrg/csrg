/* Copyright (c) 1982 Regents of the University of California */

static char sccsid[] = "@(#)ATAN.c 1.2 01/09/89";

#include <math.h>
extern int errno;

double
ATAN(value)
	double	value;
{
	double result;

	errno = 0;
	result = atan(value);
	if (errno != 0) {
		ERROR("Argument %e is out of the domain of atan\n", value);
	}
	return result;
}
