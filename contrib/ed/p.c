/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rodney Ruddock of the University of Guelph.
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
 */

#ifndef lint
static char sccsid[] = "@(#)p.c	5.1 (Berkeley) 01/23/93";
#endif /* not lint */

#include "ed.h"

/*
 * Both the n and p code are here because they're almost identical.
 * Print out the line. If it's n print the line number, tab, and then
 * the line.
 */

void
p(inputt, errnum, flag)

FILE *inputt;
int *errnum, flag;

{
  int l_ln;

  if (start_default && End_default)
    start = End = current;
  else if (start_default)
    start = End;
  start_default = End_default = 0;

  if (start == NULL)
    {
      strcpy(help_msg, "bad address");
      *errnum = -1;
      return;
    }

  if (rol(inputt, errnum))  /* for "command-suffix pairs" */
    return;

  if (flag == 1)
    l_ln = line_number(start);
  if (sigint_flag)
    SIGINT_ACTION;
  current = start;
  while (1)
       {
         /* print out the lines */
         if (sigint_flag)
           SIGINT_ACTION;
         if (current == NULL)
           break;
         get_line(current->handle, current->len);
         if (flag == 1) /* when 'n' */
           printf("%d\t", l_ln++);
         fwrite(text, sizeof(char), current->len, stdout);
         putchar('\n');
         if (current == End)
           break;
         current = current->below;
       } /* end-while */

  *errnum = 1;
} /* end-p */
