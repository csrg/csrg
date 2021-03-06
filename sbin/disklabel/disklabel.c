/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Symmetric Computer Systems.
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
static char copyright[] =
"@(#) Copyright (c) 1987, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)disklabel.c	8.4 (Berkeley) 05/04/95";
/* from static char sccsid[] = "@(#)disklabel.c	1.2 (Symmetric) 11/28/85"; */
#endif /* not lint */

#include <sys/param.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#define DKTYPENAMES
#include <sys/disklabel.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "pathnames.h"

/*
 * Disklabel: read and write disklabels.
 * The label is usually placed on one of the first sectors of the disk.
 * Many machines also place a bootstrap in the same area,
 * in which case the label is embedded in the bootstrap.
 * The bootstrap source must leave space at the proper offset
 * for the label on such machines.
 */

#ifdef tahoe
#define RAWPARTITION	'a'
#else
#define RAWPARTITION	'c'
#endif

#ifndef BBSIZE
#define	BBSIZE	8192			/* size of boot area, with label */
#endif

#ifdef tahoe
#define	NUMBOOT	0
#else
#if defined(hp300) || defined(hp800)
#define	NUMBOOT	1
#else
#define	NUMBOOT	2
#endif
#endif

#define	DEFEDITOR	_PATH_VI
#define	streq(a,b)	(strcmp(a,b) == 0)

char	*dkname;
char	*specname;
char	tmpfil[] = _PATH_TMP;

extern	int errno;
char	namebuf[BBSIZE], *np = namebuf;
struct	disklabel lab;
struct	disklabel *readlabel(), *makebootarea();
char	bootarea[BBSIZE];

#if NUMBOOT > 0
int	installboot;	/* non-zero if we should install a boot program */
char	*bootbuf;	/* pointer to buffer with remainder of boot prog */
int	bootsize;	/* size of remaining boot program */
char	*xxboot;	/* primary boot */
char	*bootxx;	/* secondary boot */
char	boot0[MAXPATHLEN];
char	boot1[MAXPATHLEN];
#endif

enum	{
	UNSPEC, EDIT, NOWRITE, READ, RESTORE, WRITE, WRITEABLE, WRITEBOOT
} op = UNSPEC;

int	rflag;

#ifdef DEBUG
int	debug;
#define OPTIONS	"BNRWb:ders:w"
#else
#define OPTIONS	"BNRWb:ers:w"
#endif


main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	register struct disklabel *lp;
	FILE *t;
	int ch, f, flag, error = 0;
	char *name = 0;

	while ((ch = getopt(argc, argv, OPTIONS)) != EOF)
		switch (ch) {
#if NUMBOOT > 0
			case 'B':
				++installboot;
				break;
			case 'b':
				xxboot = optarg;
				break;
#if NUMBOOT > 1
			case 's':
				bootxx = optarg;
				break;
#endif
#endif
			case 'N':
				if (op != UNSPEC)
					usage();
				op = NOWRITE;
				break;
			case 'R':
				if (op != UNSPEC)
					usage();
				op = RESTORE;
				break;
			case 'W':
				if (op != UNSPEC)
					usage();
				op = WRITEABLE;
				break;
			case 'e':
				if (op != UNSPEC)
					usage();
				op = EDIT;
				break;
			case 'r':
				++rflag;
				break;
			case 'w':
				if (op != UNSPEC)
					usage();
				op = WRITE;
				break;
#ifdef DEBUG
			case 'd':
				debug++;
				break;
#endif
			case '?':
			default:
				usage();
		}
	argc -= optind;
	argv += optind;
#if NUMBOOT > 0
	if (installboot) {
		rflag++;
		if (op == UNSPEC)
			op = WRITEBOOT;
	} else {
		if (op == UNSPEC)
			op = READ;
		xxboot = bootxx = 0;
	}
#else
	if (op == UNSPEC)
		op = READ;
#endif
	if (argc < 1)
		usage();

	dkname = argv[0];
	if (dkname[0] != '/') {
		(void)sprintf(np, "%sr%s%c", _PATH_DEV, dkname, RAWPARTITION);
		specname = np;
		np += strlen(specname) + 1;
	} else
		specname = dkname;
	f = open(specname, op == READ ? O_RDONLY : O_RDWR);
	if (f < 0 && errno == ENOENT && dkname[0] != '/') {
		(void)sprintf(specname, "%sr%s", _PATH_DEV, dkname);
		np = namebuf + strlen(specname) + 1;
		f = open(specname, op == READ ? O_RDONLY : O_RDWR);
	}
	if (f < 0)
		Perror(specname);

	switch(op) {

	case EDIT:
		if (argc != 1)
			usage();
		lp = readlabel(f);
		error = edit(lp, f);
		break;

	case NOWRITE:
		flag = 0;
		if (ioctl(f, DIOCWLABEL, (char *)&flag) < 0)
			Perror("ioctl DIOCWLABEL");
		break;

	case READ:
		if (argc != 1)
			usage();
		lp = readlabel(f);
		display(stdout, lp);
		error = checklabel(lp);
		break;

	case RESTORE:
#if NUMBOOT > 0
		if (installboot && argc == 3) {
			makelabel(argv[2], 0, &lab);
			argc--;
		}
#endif
		if (argc != 2)
			usage();
		lp = makebootarea(bootarea, &lab, f);
		if (!(t = fopen(argv[1], "r")))
			Perror(argv[1]);
		if (getasciilabel(t, lp))
			error = writelabel(f, bootarea, lp);
		break;

	case WRITE:
		if (argc == 3) {
			name = argv[2];
			argc--;
		}
		if (argc != 2)
			usage();
		makelabel(argv[1], name, &lab);
		lp = makebootarea(bootarea, &lab, f);
		*lp = lab;
		if (checklabel(lp) == 0)
			error = writelabel(f, bootarea, lp);
		break;

	case WRITEABLE:
		flag = 1;
		if (ioctl(f, DIOCWLABEL, (char *)&flag) < 0)
			Perror("ioctl DIOCWLABEL");
		break;

#if NUMBOOT > 0
	case WRITEBOOT:
	{
		struct disklabel tlab;

		lp = readlabel(f);
		tlab = *lp;
		if (argc == 2)
			makelabel(argv[1], 0, &lab);
		lp = makebootarea(bootarea, &lab, f);
		*lp = tlab;
		if (checklabel(lp) == 0)
			error = writelabel(f, bootarea, lp);
		break;
	}
#endif
	}
	exit(error);
}

/*
 * Construct a prototype disklabel from /etc/disktab.  As a side
 * effect, set the names of the primary and secondary boot files
 * if specified.
 */
makelabel(type, name, lp)
	char *type, *name;
	register struct disklabel *lp;
{
	register struct disklabel *dp;
	char *strcpy();

	dp = getdiskbyname(type);
	if (dp == NULL) {
		fprintf(stderr, "%s: unknown disk type\n", type);
		exit(1);
	}
	*lp = *dp;
#if NUMBOOT > 0
	/*
	 * Set bootstrap name(s).
	 * 1. If set from command line, use those,
	 * 2. otherwise, check if disktab specifies them (b0 or b1),
	 * 3. otherwise, makebootarea() will choose ones based on the name
	 *    of the disk special file. E.g. /dev/ra0 -> raboot, bootra
	 */
	if (!xxboot && lp->d_boot0) {
		if (*lp->d_boot0 != '/')
			(void)sprintf(boot0, "%s/%s",
				      _PATH_BOOTDIR, lp->d_boot0);
		else
			(void)strcpy(boot0, lp->d_boot0);
		xxboot = boot0;
	}
#if NUMBOOT > 1
	if (!bootxx && lp->d_boot1) {
		if (*lp->d_boot1 != '/')
			(void)sprintf(boot1, "%s/%s",
				      _PATH_BOOTDIR, lp->d_boot1);
		else
			(void)strcpy(boot1, lp->d_boot1);
		bootxx = boot1;
	}
#endif
#endif
	/* d_packname is union d_boot[01], so zero */
	memset(lp->d_packname, 0, sizeof(lp->d_packname));
	if (name)
		(void)strncpy(lp->d_packname, name, sizeof(lp->d_packname));
}

writelabel(f, boot, lp)
	int f;
	char *boot;
	register struct disklabel *lp;
{
	register int i;
	int flag;

	setbootflag(lp);
	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	if (rflag) {
		/*
		 * First set the kernel disk label,
		 * then write a label to the raw disk.
		 * If the SDINFO ioctl fails because it is unimplemented,
		 * keep going; otherwise, the kernel consistency checks
		 * may prevent us from changing the current (in-core)
		 * label.
		 */
		if (ioctl(f, DIOCSDINFO, lp) < 0 &&
		    errno != ENODEV && errno != ENOTTY) {
			l_perror("ioctl DIOCSDINFO");
			return (1);
		}
		(void)lseek(f, (off_t)0, SEEK_SET);
		/*
		 * write enable label sector before write (if necessary),
		 * disable after writing.
		 */
		flag = 1;
		if (ioctl(f, DIOCWLABEL, &flag) < 0)
			perror("ioctl DIOCWLABEL");
		if (write(f, boot, lp->d_bbsize) != lp->d_bbsize) {
			perror("write");
			return (1);
		}
#if NUMBOOT > 0
		/*
		 * Output the remainder of the disklabel
		 */
		if (bootbuf && write(f, bootbuf, bootsize) != bootsize) {
			perror("write");
			return(1);
		}
#endif
		flag = 0;
		(void) ioctl(f, DIOCWLABEL, &flag);
	} else if (ioctl(f, DIOCWDINFO, lp) < 0) {
		l_perror("ioctl DIOCWDINFO");
		return (1);
	}
#ifdef vax
	if (lp->d_type == DTYPE_SMD && lp->d_flags & D_BADSECT) {
		daddr_t alt;

		alt = lp->d_ncylinders * lp->d_secpercyl - lp->d_nsectors;
		for (i = 1; i < 11 && i < lp->d_nsectors; i += 2) {
			(void)lseek(f, (off_t)((alt + i) * lp->d_secsize),
			    SEEK_SET);
			if (write(f, boot, lp->d_secsize) < lp->d_secsize) {
				int oerrno = errno;
				fprintf(stderr, "alternate label %d ", i/2);
				errno = oerrno;
				perror("write");
			}
		}
	}
#endif
	return (0);
}

l_perror(s)
	char *s;
{
	int saverrno = errno;

	fprintf(stderr, "disklabel: %s: ", s);

	switch (saverrno) {

	case ESRCH:
		fprintf(stderr, "No disk label on disk;\n");
		fprintf(stderr,
		    "use \"disklabel -r\" to install initial label\n");
		break;

	case EINVAL:
		fprintf(stderr, "Label magic number or checksum is wrong!\n");
		fprintf(stderr, "(disklabel or kernel is out of date?)\n");
		break;

	case EBUSY:
		fprintf(stderr, "Open partition would move or shrink\n");
		break;

	case EXDEV:
		fprintf(stderr,
	"Labeled partition or 'a' partition must start at beginning of disk\n");
		break;

	default:
		errno = saverrno;
		perror((char *)NULL);
		break;
	}
}

/*
 * Fetch disklabel for disk.
 * Use ioctl to get label unless -r flag is given.
 */
struct disklabel *
readlabel(f)
	int f;
{
	register struct disklabel *lp;

	if (rflag) {
		if (read(f, bootarea, BBSIZE) < BBSIZE)
			Perror(specname);
		for (lp = (struct disklabel *)bootarea;
		    lp <= (struct disklabel *)(bootarea + BBSIZE - sizeof(*lp));
		    lp = (struct disklabel *)((char *)lp + 16))
			if (lp->d_magic == DISKMAGIC &&
			    lp->d_magic2 == DISKMAGIC)
				break;
		if (lp > (struct disklabel *)(bootarea+BBSIZE-sizeof(*lp)) ||
		    lp->d_magic != DISKMAGIC || lp->d_magic2 != DISKMAGIC ||
		    dkcksum(lp) != 0) {
			fprintf(stderr,
	"Bad pack magic number (label is damaged, or pack is unlabeled)\n");
			/* lp = (struct disklabel *)(bootarea + LABELOFFSET); */
			exit (1);
		}
	} else {
		lp = &lab;
		if (ioctl(f, DIOCGDINFO, lp) < 0)
			Perror("ioctl DIOCGDINFO");
	}
	return (lp);
}

/*
 * Construct a bootarea (d_bbsize bytes) in the specified buffer ``boot''
 * Returns a pointer to the disklabel portion of the bootarea.
 */
struct disklabel *
makebootarea(boot, dp, f)
	char *boot;
	register struct disklabel *dp;
	int f;
{
	struct disklabel *lp;
	register char *p;
	int b;
#if NUMBOOT > 0
	char *dkbasename;
	struct stat sb;
#endif

	/* XXX */
	if (dp->d_secsize == 0) {
		dp->d_secsize = DEV_BSIZE;
		dp->d_bbsize = BBSIZE;
	}
	lp = (struct disklabel *)
		(boot + (LABELSECTOR * dp->d_secsize) + LABELOFFSET);
	memset(lp, 0, sizeof *lp);
#if NUMBOOT > 0
	/*
	 * If we are not installing a boot program but we are installing a
	 * label on disk then we must read the current bootarea so we don't
	 * clobber the existing boot.
	 */
	if (!installboot) {
		if (rflag) {
			if (read(f, boot, BBSIZE) < BBSIZE)
				Perror(specname);
			memset(lp, 0, sizeof *lp);
		}
		return (lp);
	}
	/*
	 * We are installing a boot program.  Determine the name(s) and
	 * read them into the appropriate places in the boot area.
	 */
	if (!xxboot || !bootxx) {
		dkbasename = np;
		if ((p = strrchr(dkname, '/')) == NULL)
			p = dkname;
		else
			p++;
		while (*p && !isdigit(*p))
			*np++ = *p++;
		*np++ = '\0';

		if (!xxboot) {
			(void)sprintf(np, "%s/%sboot",
				      _PATH_BOOTDIR, dkbasename);
			if (access(np, F_OK) < 0 && dkbasename[0] == 'r')
				dkbasename++;
			xxboot = np;
			(void)sprintf(xxboot, "%s/%sboot",
				      _PATH_BOOTDIR, dkbasename);
			np += strlen(xxboot) + 1;
		}
#if NUMBOOT > 1
		if (!bootxx) {
			(void)sprintf(np, "%s/boot%s",
				      _PATH_BOOTDIR, dkbasename);
			if (access(np, F_OK) < 0 && dkbasename[0] == 'r')
				dkbasename++;
			bootxx = np;
			(void)sprintf(bootxx, "%s/boot%s",
				      _PATH_BOOTDIR, dkbasename);
			np += strlen(bootxx) + 1;
		}
#endif
	}
#ifdef DEBUG
	if (debug)
		fprintf(stderr, "bootstraps: xxboot = %s, bootxx = %s\n",
			xxboot, bootxx ? bootxx : "NONE");
#endif

	/*
	 * Strange rules:
	 * 1. One-piece bootstrap (hp300/hp800)
	 *	up to d_bbsize bytes of ``xxboot'' go in bootarea, the rest
	 *	is remembered and written later following the bootarea.
	 * 2. Two-piece bootstraps (vax/i386?/mips?)
	 *	up to d_secsize bytes of ``xxboot'' go in first d_secsize
	 *	bytes of bootarea, remaining d_bbsize-d_secsize filled
	 *	from ``bootxx''.
	 */
	b = open(xxboot, O_RDONLY);
	if (b < 0)
		Perror(xxboot);
#if NUMBOOT > 1
	if (read(b, boot, (int)dp->d_secsize) < 0)
		Perror(xxboot);
	(void)close(b);
	b = open(bootxx, O_RDONLY);
	if (b < 0)
		Perror(bootxx);
	if (read(b, &boot[dp->d_secsize], (int)(dp->d_bbsize-dp->d_secsize)) < 0)
		Perror(bootxx);
#else
	if (read(b, boot, (int)dp->d_bbsize) < 0)
		Perror(xxboot);
	(void)fstat(b, &sb);
	bootsize = (int)sb.st_size - dp->d_bbsize;
	if (bootsize > 0) {
		/* XXX assume d_secsize is a power of two */
		bootsize = (bootsize + dp->d_secsize-1) & ~(dp->d_secsize-1);
		bootbuf = (char *)malloc((size_t)bootsize);
		if (bootbuf == 0)
			Perror(xxboot);
		if (read(b, bootbuf, bootsize) < 0) {
			free(bootbuf);
			Perror(xxboot);
		}
	}
#endif
	(void)close(b);
#endif
	/*
	 * Make sure no part of the bootstrap is written in the area
	 * reserved for the label.
	 */
	for (p = (char *)lp; p < (char *)lp + sizeof(struct disklabel); p++)
		if (*p) {
			fprintf(stderr,
			    "Bootstrap doesn't leave room for disk label\n");
			exit(2);
		}
	return (lp);
}

display(f, lp)
	FILE *f;
	register struct disklabel *lp;
{
	register int i, j;
	register struct partition *pp;

	fprintf(f, "# %s:\n", specname);
	if ((unsigned) lp->d_type < DKMAXTYPES)
		fprintf(f, "type: %s\n", dktypenames[lp->d_type]);
	else
		fprintf(f, "type: %d\n", lp->d_type);
	fprintf(f, "disk: %.*s\n", sizeof(lp->d_typename), lp->d_typename);
	fprintf(f, "label: %.*s\n", sizeof(lp->d_packname), lp->d_packname);
	fprintf(f, "flags:");
	if (lp->d_flags & D_REMOVABLE)
		fprintf(f, " removeable");
	if (lp->d_flags & D_ECC)
		fprintf(f, " ecc");
	if (lp->d_flags & D_BADSECT)
		fprintf(f, " badsect");
	fprintf(f, "\n");
	fprintf(f, "bytes/sector: %d\n", lp->d_secsize);
	fprintf(f, "sectors/track: %d\n", lp->d_nsectors);
	fprintf(f, "tracks/cylinder: %d\n", lp->d_ntracks);
	fprintf(f, "sectors/cylinder: %d\n", lp->d_secpercyl);
	fprintf(f, "cylinders: %d\n", lp->d_ncylinders);
	fprintf(f, "rpm: %d\n", lp->d_rpm);
	fprintf(f, "interleave: %d\n", lp->d_interleave);
	fprintf(f, "trackskew: %d\n", lp->d_trackskew);
	fprintf(f, "cylinderskew: %d\n", lp->d_cylskew);
	fprintf(f, "headswitch: %d\t\t# milliseconds\n", lp->d_headswitch);
	fprintf(f, "track-to-track seek: %d\t# milliseconds\n", lp->d_trkseek);
	fprintf(f, "drivedata: ");
	for (i = NDDATA - 1; i >= 0; i--)
		if (lp->d_drivedata[i])
			break;
	if (i < 0)
		i = 0;
	for (j = 0; j <= i; j++)
		fprintf(f, "%d ", lp->d_drivedata[j]);
	fprintf(f, "\n\n%d partitions:\n", lp->d_npartitions);
	fprintf(f,
	    "#        size   offset    fstype   [fsize bsize   cpg]\n");
	pp = lp->d_partitions;
	for (i = 0; i < lp->d_npartitions; i++, pp++) {
		if (pp->p_size) {
			fprintf(f, "  %c: %8d %8d  ", 'a' + i,
			   pp->p_size, pp->p_offset);
			if ((unsigned) pp->p_fstype < FSMAXTYPES)
				fprintf(f, "%8.8s", fstypenames[pp->p_fstype]);
			else
				fprintf(f, "%8d", pp->p_fstype);
			switch (pp->p_fstype) {

			case FS_UNUSED:				/* XXX */
				fprintf(f, "    %5d %5d %5.5s ",
				    pp->p_fsize, pp->p_fsize * pp->p_frag, "");
				break;

			case FS_BSDFFS:
				fprintf(f, "    %5d %5d %5d ",
				    pp->p_fsize, pp->p_fsize * pp->p_frag,
				    pp->p_cpg);
				break;

			default:
				fprintf(f, "%20.20s", "");
				break;
			}
			fprintf(f, "\t# (Cyl. %4d",
			    pp->p_offset / lp->d_secpercyl);
			if (pp->p_offset % lp->d_secpercyl)
			    putc('*', f);
			else
			    putc(' ', f);
			fprintf(f, "- %d",
			    (pp->p_offset + 
			    pp->p_size + lp->d_secpercyl - 1) /
			    lp->d_secpercyl - 1);
			if (pp->p_size % lp->d_secpercyl)
			    putc('*', f);
			fprintf(f, ")\n");
		}
	}
	fflush(f);
}

edit(lp, f)
	struct disklabel *lp;
	int f;
{
	register int c;
	struct disklabel label;
	FILE *fd;
	char *mktemp();

	(void) mktemp(tmpfil);
	fd = fopen(tmpfil, "w");
	if (fd == NULL) {
		fprintf(stderr, "%s: Can't create\n", tmpfil);
		return (1);
	}
	(void)fchmod(fileno(fd), 0600);
	display(fd, lp);
	fclose(fd);
	for (;;) {
		if (!editit())
			break;
		fd = fopen(tmpfil, "r");
		if (fd == NULL) {
			fprintf(stderr, "%s: Can't reopen for reading\n",
				tmpfil);
			break;
		}
		memset(&label, 0, sizeof(label));
		if (getasciilabel(fd, &label)) {
			*lp = label;
			if (writelabel(f, bootarea, lp) == 0) {
				(void) unlink(tmpfil);
				return (0);
			}
		}
		printf("re-edit the label? [y]: "); fflush(stdout);
		c = getchar();
		if (c != EOF && c != (int)'\n')
			while (getchar() != (int)'\n')
				;
		if  (c == (int)'n')
			break;
	}
	(void) unlink(tmpfil);
	return (1);
}

editit()
{
	register int pid, xpid;
	int stat, omask;
	extern char *getenv();

	omask = sigblock(sigmask(SIGINT)|sigmask(SIGQUIT)|sigmask(SIGHUP));
	while ((pid = fork()) < 0) {
		extern int errno;

		if (errno == EPROCLIM) {
			fprintf(stderr, "You have too many processes\n");
			return(0);
		}
		if (errno != EAGAIN) {
			perror("fork");
			return(0);
		}
		sleep(1);
	}
	if (pid == 0) {
		register char *ed;

		sigsetmask(omask);
		setgid(getgid());
		setuid(getuid());
		if ((ed = getenv("EDITOR")) == (char *)0)
			ed = DEFEDITOR;
		execlp(ed, ed, tmpfil, 0);
		perror(ed);
		exit(1);
	}
	while ((xpid = wait(&stat)) >= 0)
		if (xpid == pid)
			break;
	sigsetmask(omask);
	return(!stat);
}

char *
skip(cp)
	register char *cp;
{

	while (*cp != '\0' && isspace(*cp))
		cp++;
	if (*cp == '\0' || *cp == '#')
		return ((char *)NULL);
	return (cp);
}

char *
word(cp)
	register char *cp;
{
	register char c;

	while (*cp != '\0' && !isspace(*cp) && *cp != '#')
		cp++;
	if ((c = *cp) != '\0') {
		*cp++ = '\0';
		if (c != '#')
			return (skip(cp));
	}
	return ((char *)NULL);
}

/*
 * Read an ascii label in from fd f,
 * in the same format as that put out by display(),
 * and fill in lp.
 */
getasciilabel(f, lp)
	FILE	*f;
	register struct disklabel *lp;
{
	register char **cpp, *cp;
	register struct partition *pp;
	char *tp, *s, line[BUFSIZ];
	int v, lineno = 0, errors = 0;

	lp->d_bbsize = BBSIZE;				/* XXX */
	lp->d_sbsize = SBSIZE;				/* XXX */
	while (fgets(line, sizeof(line) - 1, f)) {
		lineno++;
		if (cp = strchr(line,'\n'))
			*cp = '\0';
		cp = skip(line);
		if (cp == NULL)
			continue;
		tp = strchr(cp, ':');
		if (tp == NULL) {
			fprintf(stderr, "line %d: syntax error\n", lineno);
			errors++;
			continue;
		}
		*tp++ = '\0', tp = skip(tp);
		if (streq(cp, "type")) {
			if (tp == NULL)
				tp = "unknown";
			cpp = dktypenames;
			for (; cpp < &dktypenames[DKMAXTYPES]; cpp++)
				if ((s = *cpp) && streq(s, tp)) {
					lp->d_type = cpp - dktypenames;
					goto next;
				}
			v = atoi(tp);
			if ((unsigned)v >= DKMAXTYPES)
				fprintf(stderr, "line %d:%s %d\n", lineno,
				    "Warning, unknown disk type", v);
			lp->d_type = v;
			continue;
		}
		if (streq(cp, "flags")) {
			for (v = 0; (cp = tp) && *cp != '\0';) {
				tp = word(cp);
				if (streq(cp, "removeable"))
					v |= D_REMOVABLE;
				else if (streq(cp, "ecc"))
					v |= D_ECC;
				else if (streq(cp, "badsect"))
					v |= D_BADSECT;
				else {
					fprintf(stderr,
					    "line %d: %s: bad flag\n",
					    lineno, cp);
					errors++;
				}
			}
			lp->d_flags = v;
			continue;
		}
		if (streq(cp, "drivedata")) {
			register int i;

			for (i = 0; (cp = tp) && *cp != '\0' && i < NDDATA;) {
				lp->d_drivedata[i++] = atoi(cp);
				tp = word(cp);
			}
			continue;
		}
		if (sscanf(cp, "%d partitions", &v) == 1) {
			if (v == 0 || (unsigned)v > MAXPARTITIONS) {
				fprintf(stderr,
				    "line %d: bad # of partitions\n", lineno);
				lp->d_npartitions = MAXPARTITIONS;
				errors++;
			} else
				lp->d_npartitions = v;
			continue;
		}
		if (tp == NULL)
			tp = "";
		if (streq(cp, "disk")) {
			strncpy(lp->d_typename, tp, sizeof (lp->d_typename));
			continue;
		}
		if (streq(cp, "label")) {
			strncpy(lp->d_packname, tp, sizeof (lp->d_packname));
			continue;
		}
		if (streq(cp, "bytes/sector")) {
			v = atoi(tp);
			if (v <= 0 || (v % 512) != 0) {
				fprintf(stderr,
				    "line %d: %s: bad sector size\n",
				    lineno, tp);
				errors++;
			} else
				lp->d_secsize = v;
			continue;
		}
		if (streq(cp, "sectors/track")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_nsectors = v;
			continue;
		}
		if (streq(cp, "sectors/cylinder")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_secpercyl = v;
			continue;
		}
		if (streq(cp, "tracks/cylinder")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_ntracks = v;
			continue;
		}
		if (streq(cp, "cylinders")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_ncylinders = v;
			continue;
		}
		if (streq(cp, "rpm")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_rpm = v;
			continue;
		}
		if (streq(cp, "interleave")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_interleave = v;
			continue;
		}
		if (streq(cp, "trackskew")) {
			v = atoi(tp);
			if (v < 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_trackskew = v;
			continue;
		}
		if (streq(cp, "cylinderskew")) {
			v = atoi(tp);
			if (v < 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_cylskew = v;
			continue;
		}
		if (streq(cp, "headswitch")) {
			v = atoi(tp);
			if (v < 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_headswitch = v;
			continue;
		}
		if (streq(cp, "track-to-track seek")) {
			v = atoi(tp);
			if (v < 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_trkseek = v;
			continue;
		}
		if ('a' <= *cp && *cp <= 'z' && cp[1] == '\0') {
			unsigned part = *cp - 'a';

			if (part > lp->d_npartitions) {
				fprintf(stderr,
				    "line %d: bad partition name\n", lineno);
				errors++;
				continue;
			}
			pp = &lp->d_partitions[part];
#define NXTNUM(n) { \
	cp = tp, tp = word(cp); \
	if (tp == NULL) \
		tp = cp; \
	(n) = atoi(cp); \
     }

			NXTNUM(v);
			if (v < 0) {
				fprintf(stderr,
				    "line %d: %s: bad partition size\n",
				    lineno, cp);
				errors++;
			} else
				pp->p_size = v;
			NXTNUM(v);
			if (v < 0) {
				fprintf(stderr,
				    "line %d: %s: bad partition offset\n",
				    lineno, cp);
				errors++;
			} else
				pp->p_offset = v;
			cp = tp, tp = word(cp);
			cpp = fstypenames;
			for (; cpp < &fstypenames[FSMAXTYPES]; cpp++)
				if ((s = *cpp) && streq(s, cp)) {
					pp->p_fstype = cpp - fstypenames;
					goto gottype;
				}
			if (isdigit(*cp))
				v = atoi(cp);
			else
				v = FSMAXTYPES;
			if ((unsigned)v >= FSMAXTYPES) {
				fprintf(stderr, "line %d: %s %s\n", lineno,
				    "Warning, unknown filesystem type", cp);
				v = FS_UNUSED;
			}
			pp->p_fstype = v;
	gottype:

			switch (pp->p_fstype) {

			case FS_UNUSED:				/* XXX */
				NXTNUM(pp->p_fsize);
				if (pp->p_fsize == 0)
					break;
				NXTNUM(v);
				pp->p_frag = v / pp->p_fsize;
				break;

			case FS_BSDFFS:
				NXTNUM(pp->p_fsize);
				if (pp->p_fsize == 0)
					break;
				NXTNUM(v);
				pp->p_frag = v / pp->p_fsize;
				NXTNUM(pp->p_cpg);
				break;

			default:
				break;
			}
			continue;
		}
		fprintf(stderr, "line %d: %s: Unknown disklabel field\n",
		    lineno, cp);
		errors++;
	next:
		;
	}
	errors += checklabel(lp);
	return (errors == 0);
}

/*
 * Check disklabel for errors and fill in
 * derived fields according to supplied values.
 */
checklabel(lp)
	register struct disklabel *lp;
{
	register struct partition *pp;
	int i, errors = 0;
	char part;

	if (lp->d_secsize == 0) {
		fprintf(stderr, "sector size %d\n", lp->d_secsize);
		return (1);
	}
	if (lp->d_nsectors == 0) {
		fprintf(stderr, "sectors/track %d\n", lp->d_nsectors);
		return (1);
	}
	if (lp->d_ntracks == 0) {
		fprintf(stderr, "tracks/cylinder %d\n", lp->d_ntracks);
		return (1);
	}
	if  (lp->d_ncylinders == 0) {
		fprintf(stderr, "cylinders/unit %d\n", lp->d_ncylinders);
		errors++;
	}
	if (lp->d_rpm == 0)
		Warning("revolutions/minute %d", lp->d_rpm);
	if (lp->d_secpercyl == 0)
		lp->d_secpercyl = lp->d_nsectors * lp->d_ntracks;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = lp->d_secpercyl * lp->d_ncylinders;
	if (lp->d_bbsize == 0) {
		fprintf(stderr, "boot block size %d\n", lp->d_bbsize);
		errors++;
	} else if (lp->d_bbsize % lp->d_secsize)
		Warning("boot block size %% sector-size != 0");
	if (lp->d_sbsize == 0) {
		fprintf(stderr, "super block size %d\n", lp->d_sbsize);
		errors++;
	} else if (lp->d_sbsize % lp->d_secsize)
		Warning("super block size %% sector-size != 0");
	if (lp->d_npartitions > MAXPARTITIONS)
		Warning("number of partitions (%d) > MAXPARTITIONS (%d)",
		    lp->d_npartitions, MAXPARTITIONS);
	for (i = 0; i < lp->d_npartitions; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size == 0 && pp->p_offset != 0)
			Warning("partition %c: size 0, but offset %d",
			    part, pp->p_offset);
#ifdef notdef
		if (pp->p_size % lp->d_secpercyl)
			Warning("partition %c: size %% cylinder-size != 0",
			    part);
		if (pp->p_offset % lp->d_secpercyl)
			Warning("partition %c: offset %% cylinder-size != 0",
			    part);
#endif
		if (pp->p_offset > lp->d_secperunit) {
			fprintf(stderr,
			    "partition %c: offset past end of unit\n", part);
			errors++;
		}
		if (pp->p_offset + pp->p_size > lp->d_secperunit) {
			fprintf(stderr,
			    "partition %c: partition extends past end of unit\n",
			    part);
			errors++;
		}
	}
	for (; i < MAXPARTITIONS; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size || pp->p_offset)
			Warning("unused partition %c: size %d offset %d",
			    'a' + i, pp->p_size, pp->p_offset);
	}
	return (errors);
}

/*
 * If we are installing a boot program that doesn't fit in d_bbsize
 * we need to mark those partitions that the boot overflows into.
 * This allows newfs to prevent creation of a filesystem where it might
 * clobber bootstrap code.
 */
setbootflag(lp)
	register struct disklabel *lp;
{
	register struct partition *pp;
	int i, errors = 0;
	char part;
	u_long boffset;

	if (bootbuf == 0)
		return;
	boffset = bootsize / lp->d_secsize;
	for (i = 0; i < lp->d_npartitions; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size == 0)
			continue;
		if (boffset <= pp->p_offset) {
			if (pp->p_fstype == FS_BOOT)
				pp->p_fstype = FS_UNUSED;
		} else if (pp->p_fstype != FS_BOOT) {
			if (pp->p_fstype != FS_UNUSED) {
				fprintf(stderr,
					"boot overlaps used partition %c\n",
					part);
				errors++;
			} else {
				pp->p_fstype = FS_BOOT;
				Warning("boot overlaps partition %c, %s",
					part, "marked as FS_BOOT");
			}
		}
	}
	if (errors) {
		fprintf(stderr, "Cannot install boot program\n");
		exit(4);
	}
}

/*VARARGS1*/
Warning(fmt, a1, a2, a3, a4, a5)
	char *fmt;
{

	fprintf(stderr, "Warning, ");
	fprintf(stderr, fmt, a1, a2, a3, a4, a5);
	fprintf(stderr, "\n");
}

Perror(str)
	char *str;
{
	fputs("disklabel: ", stderr); perror(str);
	exit(4);
}

usage()
{
#if NUMBOOT > 0
	fprintf(stderr,
"%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n",
"usage: disklabel [-r] disk",
		"(to read label)",
"or disklabel -w [-r] disk type [ packid ]",
		"(to write label with existing boot program)",
"or disklabel -e [-r] disk",
		"(to edit label)",
"or disklabel -R [-r] disk protofile",
		"(to restore label with existing boot program)",
#if NUMBOOT > 1
"or disklabel -B [ -b boot1 [ -s boot2 ] ] disk [ type ]",
		"(to install boot program with existing label)",
"or disklabel -w -B [ -b boot1 [ -s boot2 ] ] disk type [ packid ]",
		"(to write label and boot program)",
"or disklabel -R -B [ -b boot1 [ -s boot2 ] ] disk protofile [ type ]",
		"(to restore label and boot program)",
#else
"or disklabel -B [ -b bootprog ] disk [ type ]",
		"(to install boot program with existing on-disk label)",
"or disklabel -w -B [ -b bootprog ] disk type [ packid ]",
		"(to write label and install boot program)",
"or disklabel -R -B [ -b bootprog ] disk protofile [ type ]",
		"(to restore label and install boot program)",
#endif
"or disklabel [-NW] disk",
		"(to write disable/enable label)");
#else
	fprintf(stderr, "%-43s%s\n%-43s%s\n%-43s%s\n%-43s%s\n%-43s%s\n",
"usage: disklabel [-r] disk", "(to read label)",
"or disklabel -w [-r] disk type [ packid ]", "(to write label)",
"or disklabel -e [-r] disk", "(to edit label)",
"or disklabel -R [-r] disk protofile", "(to restore label)",
"or disklabel [-NW] disk", "(to write disable/enable label)");
#endif
	exit(1);
}
