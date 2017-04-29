#include "param.h"
#include "systm.h"
#include "user.h"
#include "proc.h"
#include "buf.h"
#include "file.h"
//#include "reg.h"
#include "inode.h"
#include "sys.h"
/*
 * exec system call.
 */
void v6_exec()
{
#if 0
	int ap, na, nc;
	int ds;
	int  c;
	struct buf *bp;
	struct inode* ip;
	char *cp;

	/*
	 * pick up file names
	 * and check various modes
	 * for execute permission
	 */

	ip = namei(0);
	if(ip == NULL)
		return;
	bp = getblk(NODEV,0);
	if(iaccess(ip, IEXEC))
		goto bad;
	if((ip->i_mode & IFMT) != 0 ||
	   (ip->i_mode & (IEXEC|(IEXEC>>3)|(IEXEC>>6))) == 0) {
		u.u_error = EACCES;
		goto bad;
	}

	/*
	 * pack up arguments into
	 * allocated disk buffer
	 */

	cp = bp->b_addr;
	na = 0;
	nc = 0;
	while((ap = fuword(u.u_arg[1]))!=0) {
		na++;
		if(ap == -1)
			goto bad;
		u.u_arg[1] =+ 2;
		for(;;) {
			c = fubyte(ap++);
			if(c == -1)
				goto bad;
			*cp++ = c;
			nc++;
			if(nc > 510) {
				u.u_error = E2BIG;
				goto bad;
			}
			if(c == 0)
				break;
		}
	}
	if((nc&1) != 0) {
		*cp++ = 0;
		nc++;
	}

	/*
	 * read in first 8 bytes
	 * of file for segment
	 * sizes:
	 * w0 = 407
	 * w1 = text size
	 * w2 = data size
	 * w3 = bss size
	 */

	u.u_base = &u.u_arg[0];
	u.u_count = 8;
	u.u_offset = 0;
	readi(ip);
	if(u.u_error)
		goto bad;
	if(u.u_arg[0] == 0407) {
		u.u_arg[2] =+ u.u_arg[1];
	} else {
		u.u_error = ENOEXEC;
		goto bad;
	}

	/*
	 * find text and data sizes
	 * try them out for possible
	 * exceed of max sizes
	 */

	ds = ((u.u_arg[2]+u.u_arg[3]+63)>>6) & 01777;
	if(ds + SSIZE > UCORE)
		goto bad;

	/*
	 * allocate and clear core
	 * at this point, committed
	 * to the new image
	 */

	cp = TOPSYS;
	while(cp < TOPUSR)
		*cp++ = 0;

	/*
	 * read in data segment
	 */

	u.u_base = TOPSYS;
	u.u_offset[1] = 020;
	u.u_count = u.u_arg[2];
	u.u_dsize = ds;
	readi(ip);

	/*
	 * initialize stack segment
	 */

	u.u_ssize = SSIZE;
	cp = bp->b_addr;
	ap = TOPUSR - nc - na*2 - 4;
	//u.u_ar0[R6] = ap;
	suword(ap, na);
	c = TOPUSR - nc;
	while(na--) {
		suword(ap=+2, c);
		do
			subyte(c++, *cp);
		while(*cp++);
	}
	suword(ap+2, -1);

	/*
	 * set SUID/SGID protections
	 */

	if(ip->i_mode&ISUID)
		if(u.u_uid != 0) {
			u.u_uid = ip->i_uid;
			u.u_procp->p_uid = ip->i_uid;
		}

	/*
	 * clear sigs, regs and return
	 */

	c = ip;
	for(ip = &u.u_signal[0]; ip < &u.u_signal[NSIG]; ip++)
		if((*ip & 1) == 0)
			*ip = 0;
	for(cp = &regloc[0]; cp < &regloc[6];)
		u.u_ar0[*cp++] = 0;
//	u.u_ar0[R7] = TOPSYS;
	for(ip = &u.u_fsav[0]; ip < &u.u_fsav[25];)
		*ip++ = 0;
	ip = c;

bad:
	iput(ip);
	brelse(bp);
#endif
}

/*
 * exit system call:
 * pass back caller's r0
 */
void v6_rexit()
{

	//u.u_arg[0] = u.u_ar0[R0] << 8;
	v6_exit();
}

/*
 * Release resources.
 * Save u. area for parent to look at.
 * Enter zombie state.
 * Wake up parent and init processes,
 * and dispose of children.
 */
void v6_exit()
{
#if 0
	int *q, a;
	struct proc *p;

	p = u.u_procp;
	p->p_clktim = 0;
	for(q = &u.u_signal[0]; q < &u.u_signal[NSIG];)
		*q++ = 1;
	for(q = &u.u_ofile[0]; q < &u.u_ofile[NOFILE]; q++)
		if(a = *q) {
			*q = NULL;
			closef(a);
		}
	iput(u.u_cdir);
	p = getblk(swapdev, p->p_pid*SWPSIZ+SWPLO);
	bcopy(&u, p->b_addr, 256);
	bwrite(p);
	q = u.u_procp;
	q->p_stat = SZOMB;

loop:
	for(p = &proc[0]; p < &proc[NPROC]; p++)
	if(q->p_ppid == p->p_pid) {
		wakeup(p);
		for(p = &proc[0]; p < &proc[NPROC]; p++)
		if(q->p_pid == p->p_ppid) {
			p->p_ppid  = 0;
		}
		runrun++;
		swtch();
		/* no return */
	}
	q->p_ppid = 0;
	goto loop;
#endif
}

/*
 * Wait system call.
 * Search for a terminated (zombie) child,
 * finally lay it to rest, and collect its status.
 * Look also for stopped (traced) children,
 * and pass back status from them.
 */
void r6_wait()
{
	u.u_error = ECHILD;
#if 0

	struct file* f;
	struct buf* *bp;
	register struct proc *p;

	f = 0;

loop:
	for_each_array(p,proc) {
	//for(p = &proc[0]; p < &proc[NPROC]; p++)
	if(p->p_ppid == u.u_procp->p_pid) {
		f++;
		if(p->p_stat == SZOMB) {
			u.u_ar0[R0] = p->p_pid;
			bp = bread(swapdev, p->p_pid*SWPSIZ+SWPLO);
			p->p_stat = NULL;
			p->p_pid = 0;
			p->p_ppid = 0;
			p->p_sig = 0;
			p->p_pgrp = 0;
			p->p_flag = 0;
			p = bp->b_addr;
			u.u_cstime[0] =+ p->u_cstime[0];
			dpadd(u.u_cstime, p->u_cstime[1]);
			dpadd(u.u_cstime, p->u_stime);
			u.u_cutime[0] =+ p->u_cutime[0];
			dpadd(u.u_cutime, p->u_cutime[1]);
			dpadd(u.u_cutime, p->u_utime);
			u.u_ar0[R1] = p->u_arg[0];
			brelse(bp);
			return;
		}
	}
	if(f) {
		runrun++;
		sleep(u.u_procp, PWAIT);
		goto loop;
	}
	u.u_error = ECHILD;
#endif
}

/*
 * fork system call.
 */
int v6_fork()
{
#if 1
	u.u_error = EAGAIN;
	return 0;
#else
	register struct proc *p1, *p2;

	p1 = u.u_procp;
	for(p2 = &proc[0]; p2 < &proc[NPROC]; p2++)
		if(p2->p_stat == NULL)
			goto found;
	u.u_error = EAGAIN;
	goto out;

found:
	if(newproc()) {
		u.u_ar0[R0] = p1->p_pid;
		u.u_cstime[0] = 0;
		u.u_cstime[1] = 0;
		u.u_stime = 0;
		u.u_cutime[0] = 0;
		u.u_cutime[1] = 0;
		u.u_utime = 0;
		return;
	}
	u.u_ar0[R0] = p2->p_pid;

out:
	u.u_ar0[R7] =+ 2;
#endif
}

/*
 * break system call.
 *  -- bad planning: "break" is a dirty word in C.
 */
int v6_sbreak(int size)
{
	//int a, n, d;
return -1;
	/*
	 * set n to new data size
	 * set d to new-old
	 * set n to new total size
	 */
#if 0
#if 1
	n = (((size-TOPSYS+63)>>6) & 01777);
	if(n < 0) n = 0;
	d = n - u.u_dsize;
	n =+ USIZE+u.u_ssize;
	if(n > UCORE) {
		u.u_error = E2BIG;
		return;
	}
	u.u_dsize =+ d;
#else
	n = (((u.u_arg[0]-TOPSYS+63)>>6) & 01777);
	if(n < 0)
		n = 0;
	d = n - u.u_dsize;
	n =+ USIZE+u.u_ssize;
	if(n > UCORE) {
		u.u_error = E2BIG;
		return;
	}
	u.u_dsize =+ d;
#endif
#endif
}
