/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)lfs_extern.h	7.7 (Berkeley) 01/10/92
 */

struct fid;
struct mount;
struct nameidata;
struct proc;
struct statfs;
struct timeval;
struct uio;

__BEGIN_DECLS
u_long	 cksum __P((void *, size_t));				/* XXX */
int	 lfs_balloc __P((struct vnode *, u_long, daddr_t, struct buf **));
int	 lfs_blkatoff __P((struct vnode *, off_t, char **, struct buf **));
int	 lfs_bmap __P((VNODE *, daddr_t, VNODE **, daddr_t *));
int	 lfs_bmaparray __P((VNODE *, daddr_t, daddr_t *, INDIR *, int *));
int	 lfs_bwrite __P((BUF *));
int	 lfs_fhtovp __P((struct mount *, struct fid *, int, struct vnode **));
int	 lfs_fsync
	     __P((struct vnode *, int, struct ucred *, int, struct proc *));
DINODE	*lfs_ifind __P((struct lfs *, ino_t, void *));
int	 lfs_inactive __P((VNODE *, struct proc *));
int	 lfs_init __P((void));
daddr_t	 lfs_itod __P((struct lfs *, ino_t));
int	 lfs_makeinode __P((int, struct nameidata *, struct inode **));
int	 lfs_mount
	     __P((MOUNT *, char *, caddr_t, struct nameidata *, struct proc *));
int	 lfs_mountroot __P((void));
int	 lfs_read __P((struct vnode *, struct uio *, int, struct ucred *));
int	 lfs_root __P((struct mount *, struct vnode **));
int	 lfs_segwrite __P((MOUNT *, int));
int	 lfs_statfs __P((MOUNT *, struct statfs *, struct proc *));
int	 lfs_sync __P((MOUNT *, int));
int	 lfs_truncate __P((VNODE *, u_long, int));
int	 lfs_unmount __P((MOUNT *, int, struct proc *));
int	 lfs_update
	     __P((struct vnode *, struct timeval *, struct timeval *, int));
int	 lfs_valloc __P((VNODE *, int, UCRED *, VNODE **));
int	 lfs_vcreate __P((MOUNT *, ino_t, VNODE **));
void	 lfs_vfree __P((VNODE *, ino_t, int));
int	 lfs_vget __P((struct mount *, ino_t, VNODE **));
int	 lfs_vptofh __P((struct vnode *, struct fid *));
int	 lfs_write __P((struct vnode *, struct uio *, int, struct ucred *));
#ifdef DEBUG
void	lfs_dump_dinode __P((DINODE *));
void	lfs_dump_super __P((struct lfs *));
int	lfs_umountdebug __P((struct mount *));
int	lfs_vinvalbuf __P((struct vnode *));
#endif
__END_DECLS
extern struct vnodeops lfs_vnodeops, lfs_specops;
#ifdef FIFO
extern struct vnodeops lfs_fifoops;
#define LFS_FIFOOPS &lfs_fifoops
#else
#define LFS_FIFOOPS NULL
#endif
