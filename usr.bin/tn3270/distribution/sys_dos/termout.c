/*
 *	Copyright (c) 1984, 1985, 1986 by the Regents of the
 *	University of California and by Gregory Glenn Minshall.
 *
 *	Permission to use, copy, modify, and distribute these
 *	programs and their documentation for any purpose and
 *	without fee is hereby granted, provided that this
 *	copyright and permission appear on all copies and
 *	supporting documentation, the name of the Regents of
 *	the University of California not be used in advertising
 *	or publicity pertaining to distribution of the programs
 *	without specific prior permission, and notice be given in
 *	supporting documentation that copying and distribution is
 *	by permission of the Regents of the University of California
 *	and by Gregory Glenn Minshall.  Neither the Regents of the
 *	University of California nor Gregory Glenn Minshall make
 *	representations about the suitability of this software
 *	for any purpose.  It is provided "as is" without
 *	express or implied warranty.
 */

#ifndef lint
static	char	sccsid[] = "@(#)outbound.c	3.1  10/29/86";
#endif	/* lint */


#include <stdio.h>
#include "../general.h"

#include "../telnet.ext"

#include "../ctlr/hostctlr.h"
#include "../ctlr/inbound.ext"
#include "../ctlr/oia.h"
#include "../ctlr/options.ext"
#include "../ctlr/outbound.ext"
#include "../ctlr/screen.h"

#include "../keyboard/map3270.ext"

#include "../system/globals.h"

extern void EmptyTerminal();

#define CorrectTerminalCursor() ((TransparentClock == OutputClock)? \
		terminalCursorAddress:UnLocked? CursorAddress: HighestScreen())


static int terminalCursorAddress;	/* where the cursor is on term */
static int screenInitd; 		/* the screen has been initialized */
static int screenStopped;		/* the screen has been stopped */

static int needToRing;			/* need to ring terinal bell */

typedef struct {
    char
	data,		/* The data for this position */
	attr;		/* The attributes for this position */
} ScreenBuffer;

ScreenBuffer Screen[MAXNUMBERLINES*MAXNUMBERCOLUMNS];

/* Variables for transparent mode */

#include "disp_asc.out"


/* OurExitString - designed to keep us from going through infinite recursion */

static void
OurExitString(file, string, value)
FILE	*file;
char	*string;
int	value;
{
    static int recursion = 0;

    if (!recursion) {
	recursion = 1;
	ExitString(file, string, value);
    }
}


static void
GoAway(from, where)
char *from;		/* routine that gave error */
int	where;		/* cursor address */
{
	char foo[100];

	sprintf(foo, "ERR from %s at %d (%d, %d)\n",
		from, where, ScreenLine(where), ScreenLineOffset(where));
	OurExitString(stderr, foo, 1);
	/* NOTREACHED */
}

static void
TryToSend()
{
#define	STANDOUT	0x0a
#define	NORMAL		0x02	/* Normal mode */

#define	DoAttribute(a) 	    if (IsHighlightedAttr(a)) { \
				a = STANDOUT; \
			    } else { \
				a = NORMAL; \
			    } \
			    if (IsNonDisplayAttr(a)) { \
				a = 0; 	/* zero == don't display */ \
			    } \
			    if (!FormattedScreen()) { \
				a = 1;	/* one ==> do display on unformatted */\
			    }
    ScreenImage *p, *upper;
    ScreenBuffer *sp;
    int fieldattr;		/* spends most of its time == 0 or 1 */

/* OK.  We want to do this a quickly as possible.  So, we assume we
 * only need to go from Lowest to Highest.  However, if we find a
 * field in the middle, we do the whole screen.
 *
 * In particular, we separate out the two cases from the beginning.
 */
    if ((Highest != HighestScreen()) || (Lowest != LowestScreen())) {
	register int columnsleft;

	sp = &Screen[Lowest];
	p = &Host[Lowest];
	upper = &Host[Highest];
	fieldattr = FieldAttributes(Lowest);
	DoAttribute(fieldattr);	/* Set standout, non-display status */
	columnsleft = NumberColumns-ScreenLineOffset(p-Host);

	while (p <= upper) {
	    if (IsStartFieldPointer(p)) {	/* New field? */
		Highest = HighestScreen();
		Lowest = LowestScreen();
		TryToSend();		/* Recurse */
		return;
	    } else if (fieldattr) {	/* Should we display? */
		sp->data = disp_asc[p->data];	/* Display translated data */
		sp->attr = fieldattr;
	    } else {
		sp->data = ' ';
		sp->attr = NORMAL;
	    }
			/* If the physical screen is larger than what we
			 * are using, we need to make sure that each line
			 * starts at the beginning of the line.  Otherwise,
			 * we will just string all the lines together.
			 */
	    p++;
	    sp++;
	}
    } else {		/* Going from Lowest to Highest */
	ScreenImage *End = &Host[ScreenSize]-1;

	sp = Screen;
	p = Host;
	fieldattr = FieldAttributes(LowestScreen());
	DoAttribute(fieldattr);	/* Set standout, non-display status */

	while (p <= End) {
	    if (IsStartFieldPointer(p)) {	/* New field? */
		fieldattr = FieldAttributesPointer(p);	/* Get attributes */
		DoAttribute(fieldattr);	/* Set standout, non-display */
	    } else {
		if (fieldattr) {	/* Should we display? */
				/* Display translated data */
		    sp->data = disp_asc[p->data];
		    sp->attr = fieldattr;
		} else {
		    sp->data = ' ';
		    sp->attr = NORMAL;
		}
	    }
			/* If the physical screen is larger than what we
			 * are using, we need to make sure that each line
			 * starts at the beginning of the line.  Otherwise,
			 * we will just string all the lines together.
			 */
	    p++;
	    sp++;
	}
    }
    terminalCursorAddress = CorrectTerminalCursor();
    scrwrite(Screen+Lowest, sizeof Screen[0]*(Highest-Lowest), Lowest);
    VideoSetCursorPosition(ScreenLine(terminalCursorAddress),
		    ScreenLineOffset(terminalCursorAddress), 0);
    Lowest = HighestScreen()+1;
    Highest = LowestScreen()-1;
    if (needToRing) {
	DataToTerminal("\7", 1);
	needToRing = 0;
    }
    return;
}

/* InitTerminal - called to initialize the screen, etc. */

void
InitTerminal()
{
    InitMapping();		/* Go do mapping file (MAP3270) first */
    if (!screenInitd) { 	/* not initialized */
	NumberLines = 24;	/* XXX */
	NumberColumns = 80;	/* XXX */
	scrini();
	savescr();		/* Save the screen buffer away */
	ClearArray(Screen);
	terminalCursorAddress = SetBufferAddress(0,0);
	screenInitd = 1;
	screenStopped = 0;		/* Not stopped */
    }
    Initialized = 1;
}


/* StopScreen - called when we are going away... */

void
StopScreen(doNewLine)
int doNewLine;
{
    if (screenInitd && !screenStopped) {
	scrrest();
	VideoSetCursorPosition(NumberLines-1, NumberColumns-1, 0);
    }
}


/* RefreshScreen - called to cause the screen to be refreshed */

void
RefreshScreen()
{
    Highest = HighestScreen();
    Lowest = LowestScreen();
    TryToSend();
}


/* ConnectScreen - called to reconnect to the screen */

void
ConnectScreen()
{
    if (screenInitd) {
	RefreshScreen();
	screenStopped = 0;
    }
}

/* LocalClearScreen() - clear the whole ball of wax, cheaply */

void
LocalClearScreen()
{
    Clear3270();
    Lowest = LowestScreen(); /* everything in sync... */
    Highest = HighestScreen();
    TryToSend();
}

/*
 * Implement the bell/error message function.
 */

int
	bellwinup = 0;		/* If != 0, length of bell message */
static int
	bellpos0 = 0;		/* Where error message goes */

static char	bellstring[100];/* Where message goes */

#define	BELL_SPACES	2	/* 2 spaces between border and bell */

#define	BELL_HIGH_LOW(h,l) { \
	    h = bellpos0+2*NumberColumns+bellwinup+BELL_SPACES*2; \
	    l = bellpos0; \
	}

void
BellOff()
{
    if (bellwinup) {
	BELL_HIGH_LOW(Highest,Lowest);
	TryToSend();
    }
}


void
RingBell(s)
char *s;
{
    needToRing = 1;
    if (s) {
	int len = strlen(s);

	if (len > sizeof bellstring-1) {
	    OurExitString(stderr, "Bell string too long.", 1);
	}
	memcpy(bellstring, s, len+1);
	BELL_HIGH_LOW(Highest,Lowest);
	TryToSend();
    }
}

/*
 * Update the OIA area.
 */

void
ScreenOIA(oia)
OIA *oia;
{
}


/* returns a 1 if no more output available (so, go ahead and block),
    or a 0 if there is more output available (so, just poll the other
    sources/destinations, don't block).
 */

int
DoTerminalOutput()
{
	/* called just before a select to conserve IO to terminal */
    if (!Initialized) {
	return 1;		/* No output if not initialized */
    }
    if ((Lowest <= Highest) || needToRing ||
			(terminalCursorAddress != CorrectTerminalCursor())) {
	TryToSend();
    }
    if (Lowest > Highest) {
	return 1;		/* no more output now */
    } else {
	return 0;		/* more output for future */
    }
}

/*
 * The following are defined to handle transparent data.
 */

void
TransStop()
{
    RefreshScreen();
}

void
TransOut(buffer, count)
unsigned char	*buffer;
int		count;
{

    while (DoTerminalOutput() == 0) {
	;
    }
    (void) DataToTerminal(buffer, count);
}

/*
 * init_screen()
 *
 * Initialize variables used by screen.
 */

void
init_screen()
{
    bellwinup = 0;
}


