/*
 * sccsid = "@(#)machdep.h	2.6 03/04/85";
 */

#define LOGFILE "/usr/games/lib/saillog"	/* has to match the makefile */

#define TIMEOUT 300				/* Sync() timeout in seconds */

/* for 4.2bsd machines */
#define blockalarm()	((void) sigblock(1 << SIGALRM-1))
#define unblockalarm()	((void) sigsetmask(sigblock(0) & ~(1 << SIGALRM-1)))

/* for 2.9bsd machines (onyx)
typedef int void;
#define blockalarm()	((void) sighold(SIGALRM))
#define unblockalarm()	((void) sigrelse(SIGALRM))
*/
