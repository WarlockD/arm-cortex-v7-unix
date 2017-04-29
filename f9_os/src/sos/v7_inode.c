#include "v7_internal.h"
#include "v7_inode.h"
#include "v7_bio.h"
#include "v7_user.h"
#include <fcntl.h>


struct cdevsw cdevsw[BDEV];
extern struct bdevsw bdevsw[BDEV];
typedef	struct fblk *FBLKP;
extern daddr_t rablock; // read ahead block in bio.c

#define FILEDESCOFFSET 4
struct mount mount[NMOUNT];
struct inode inode[NICINOD];	/* The inode table itself */
struct file file[NFILE];	/* The file table itself */
struct inode *mpxip=NULL;		/* mpx virtual inode */
static LIST_HEAD(,file) ffreelist;
static struct buf * balloc (dev_t dev);
static void bfree (dev_t dev, daddr_t bno);
static struct inode * ialloc (dev_t dev);
static void ifree (dev_t dev, ino_t ino);
static void  update (void);


void fsinit() {
	int i=FILEDESCOFFSET;// the file descriptors start at 4 so we can skip over
	// the standard ones used in newlib
	// fix this latter
	LIST_INIT(&ffreelist);
	LIST_INIT(&u.ofile); // just for grins
	foreach_array(f, file){
		f->f_desc = i++;
		LIST_INSERT_HEAD(&ffreelist, f, f_list);
	}
}
/*
 * prdev prints a warning message of the
 * form "mesg on dev x/y".
 * x and y are the major and minor parts of
 * the device argument.
 */
void  prdev (const char *str, dev_t dev)
{
	printk("%s on dev %u/%u\n", str, major(dev), minor(dev));
}

/*
 * deverr prints a diagnostic from
 * a device driver.
 * It prints the device, block number,
 * and an octal word (usually some error
 * status register) passed as argument.
 */
void deverror (register struct buf *bp, int o1, int o2)
{
	prdev("err", bp->b_dev);
	printk("bn=%D er=%o,%o\n", bp->b_blkno, o1, o2);
}


/*
 * Check that a block number is in the
 * range between the I list and the size
 * of the device.
 * This is used mainly to check that a
 * garbage file system has not been mounted.
 *
 * bad block on dev x/y -- not in range
 */
int badblock (struct filsys *fp, daddr_t bn, dev_t dev)
{
	if (bn < fp->s_isize || bn >= fp->s_fsize) {
		prdev("bad block", dev);
		return(1);
	}
	return(0);
}

/*
 * balloc will obtain the next available
 * kfree disk block from the kfree list of
 * the specified device.
 * The super block has up to NICFREE remembered
 * kfree blocks; the last of these is read to
 * obtain NICFREE more . . .
 *
 * no space on dev x/y -- when
 * the kfree list is exhausted.
 */
static struct buf * balloc (dev_t dev)
{
	daddr_t bno;
	struct filsys *fp;
	struct buf *bp;

	fp = getfs(dev);
	while(fp->s_flock)
		ksleep(&fp->s_flock, PINOD);
	do {
		if(fp->s_nfree <= 0)
			goto nospace;
		if (fp->s_nfree > NICFREE) {
			prdev("Bad kfree count", dev);
			goto nospace;
		}
		bno = fp->s_free[--fp->s_nfree];
		if(bno == 0)
			goto nospace;
	} while (badblock(fp, bno, dev));
	if(fp->s_nfree <= 0) {
		fp->s_flock++;
		bp = bread(dev, bno);
		if ((bp->b_flags&B_ERROR) == 0) {
			fp->s_nfree = ((FBLKP)(bp->b_un.b_addr))->df_nfree;
			memcpy(fp->s_free,((FBLKP)(bp->b_un.b_addr))->df_free, sizeof(fp->s_free));
		}
		brelse(bp);
		fp->s_flock = 0;
		kwakeup(&fp->s_flock);
		if (fp->s_nfree <=0)
			goto nospace;
	}
	bp = getblk(dev, bno);
	clrbuf(bp);
	fp->s_fmod = 1;
	return(bp);

nospace:
	fp->s_nfree = 0;
	prdev("no space", dev);
u.errno = ENOSPC;
	return(NULL);
}

/*
 * place the specified disk block
 * back on the kfree list of the
 * specified device.
 */
static void bfree (dev_t dev, daddr_t bno)
{
	struct filsys *fp;
	struct buf *bp;

	fp = getfs(dev);
	fp->s_fmod = 1;
	while(fp->s_flock)
		ksleep(&fp->s_flock, PINOD);
	if (badblock(fp, bno, dev))
		return;
	if(fp->s_nfree <= 0) {
		fp->s_nfree = 1;
		fp->s_free[0] = 0;
	}
	if(fp->s_nfree >= NICFREE) {
		fp->s_flock++;
		bp = getblk(dev, bno);
		((FBLKP)(bp->b_un.b_addr))->df_nfree = fp->s_nfree;
		memcpy(((FBLKP)(bp->b_un.b_addr))->df_free, fp->s_free, sizeof(fp->s_free));
		fp->s_nfree = 0;
		bwrite(bp);
		fp->s_flock = 0;
		kwakeup(&fp->s_flock);
	}
	fp->s_free[fp->s_nfree++] = bno;
	fp->s_fmod = 1;
}



/*
 * ballocate an unused I node
 * on the specified device.
 * Used with file creation.
 * The algorithm keeps up to
 * NICINOD spare I nodes in the
 * super block. When this runs out,
 * a linear search through the
 * I list is instituted to pick
 * up NICINOD more.
 */
static struct inode * ialloc (dev_t dev)
{
	register struct filsys *fp;
	register struct buf *bp;
	register struct inode *ip;
	int i;
	struct dinode *dp;
	ino_t ino;
	daddr_t adr;

	fp = getfs(dev);
	while(fp->s_ilock)
		ksleep(&fp->s_ilock, PINOD);
loop:
	if(fp->s_ninode > 0) {
		ino = fp->s_inode[--fp->s_ninode];
		if (ino < ROOTINO)
			goto loop;
		ip = iget(dev, ino);
		if(ip == NULL)
			return(NULL);
		if(ip->i_mode == 0) {
			for (i=0; i<NADDR; i++)
				ip->i_un.i_addr[i] = 0;
			fp->s_fmod = 1;
			return(ip);
		}
		/*
		 * Inode was ballocated after all.
		 * Look some more.
		 */
		iput(ip);
		goto loop;
	}
	fp->s_ilock++;
	ino = 1;
	for(adr = SUPERB+1; adr < fp->s_isize; adr++) {
		bp = bread(dev, adr);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			ino += INOPB;
			continue;
		}
		dp = bp->b_un.b_dino;
		for(i=0; i<INOPB; i++) {
			if(dp->di_mode != 0)
				goto cont;
			for(ip = &inode[0]; ip < &inode[NINODE]; ip++)
			if(dev==ip->i_dev && ino==ip->i_number)
				goto cont;
			fp->s_inode[fp->s_ninode++] = ino;
			if(fp->s_ninode >= NICINOD)
				break;
		cont:
			ino++;
			dp++;
		}
		brelse(bp);
		if(fp->s_ninode >= NICINOD)
			break;
	}
	fp->s_ilock = 0;
	kwakeup(&fp->s_ilock);
	if(fp->s_ninode > 0)
		goto loop;
	prdev("Out of inodes", dev);
u.errno = ENOSPC;
	return(NULL);
}

/*
 * kfree the specified I node
 * on the specified device.
 * The algorithm stores up
 * to NICINOD I nodes in the super
 * block and throws away any more.
 */
static void ifree (dev_t dev, ino_t ino)
{
	register struct filsys *fp;

	fp = getfs(dev);
	if(fp->s_ilock)
		return;
	if(fp->s_ninode >= NICINOD)
		return;
	fp->s_inode[fp->s_ninode++] = ino;
	fp->s_fmod = 1;
}

/*
 * getfs maps a device number into
 * a pointer to the incore super
 * block.
 * The algorithm is a linear
 * search through the mount table.
 * A consistency check of the
 * in core kfree-block and i-node
 * counts.
 *
 * bad count on dev x/y -- the count
 *	check failed. At this point, all
 *	the counts are zeroed which will
 *	almost certainly lead to "no space"
 *	diagnostic
 * panic: no fs -- the device is not mounted.
 *	this "cannot happen"
 */
struct filsys * getfs (dev_t dev)
{
	struct mount *mp;
	struct filsys *fp;

	for(mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
	if(mp->m_bufp != NULL && mp->m_dev == dev) {
		fp = mp->m_bufp->b_un.b_filsys;
		if(fp->s_nfree > NICFREE || fp->s_ninode > NICINOD) {
			prdev("bad count", dev);
			fp->s_nfree = 0;
			fp->s_ninode = 0;
		}
		return(fp);
	}
	panic("no fs");
	return(NULL);
}

atomic_flag updlock = ATOMIC_FLAG_INIT;

/*
 * update is the internal name of
 * 'sync'. It goes through the disk
 * queues to initiate sandbagged IO;
 * goes through the I nodes to write
 * modified nodes; and it goes through
 * the mount table to initiate modified
 * super blocks.
 */
static void  update (void)
{
	register struct inode *ip;
	register struct mount *mp;
	register struct buf *bp;
	struct filsys *fp;

	if(atomic_flag_test_and_set(&updlock)) return;

	for(mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if(mp->m_bufp != NULL) {
			fp = mp->m_bufp->b_un.b_filsys;
			if(fp->s_fmod==0 || fp->s_ilock!=0 ||
			   fp->s_flock!=0 || fp->s_ronly!=0)
				continue;
			bp = getblk(mp->m_dev, SUPERB);
			if (bp->b_flags & B_ERROR)
				continue;
			fp->s_fmod = 0;
			fp->s_time = current_time;
			memcpy(bp->b_un.b_addr,fp,BSIZE);
			bwrite(bp);
		}
	for(ip = &inode[0]; ip < &inode[NINODE]; ip++)
		if((ip->i_flag&ILOCK)==0 && ip->i_count) {
			ip->i_flag |= ILOCK;
			ip->i_count++;
			iupdat(ip, &current_time, &current_time);
			iput(ip);
		}
	atomic_flag_clear(&updlock);
	bflush(NODEV);
}
// moved from pipe.c cause its used in inode more often

/*
 * Lock a pipe.
 * If its already locked,
 * set the WANT bit and sleep.
 */
void plock (struct inode *ip)
{

	while(ip->i_flag&ILOCK) {
		ip->i_flag |= IWANT;
		ksleep(ip, PINOD);
	}
	ip->i_flag |= ILOCK;
}

/*
 * Unlock a pipe.
 * If WANT bit is on,
 * wakeup.
 * This routine is also used
 * to unlock inodes in general.
 */
void prele (struct inode *ip)
{

	ip->i_flag &= ~ILOCK;
	if(ip->i_flag&IWANT) {
		ip->i_flag &= ~IWANT;
		kwakeup(ip);
	}
}


static daddr_t d24_to_da(uint8_t* c) {
	return c[2] << 16 | c[1] << 8 | c[0];
}
static void iexpand (struct inode *ip, struct dinode *dp)
{
	ip->i_mode = dp->di_mode;
	ip->i_nlink = dp->di_nlink;
	ip->i_uid = dp->di_uid;
	ip->i_gid = dp->di_gid;
	ip->i_size = dp->di_size;
	caddr_t  p2 = (char *)dp->di_addr;
#if 1
	for(int i=0; i<NADDR; i++) {
		volatile daddr_t addr = d24_to_da((uint8_t* )p2);
		p2+=3;
		ip->i_un.i_addr[i] = addr;
	}
#else
	caddr_t p1 = (char *)ip->i_un.i_addr;
	for(int i=0; i<NADDR; i++) {
		*p1++ = *p2++;
		*p1++ = 0;
		*p1++ = *p2++;
		*p1++ = *p2++;
	}
#endif

}

/*
 * Look up an inode by device,inumber.
 * If it is in core (in the inode structure),
 * honor the locking protocol.
 * If it is not in core, read it in from the
 * specified device.
 * If the inode is mounted on, perform
 * the indicated indirection.
 * In all cases, a pointer to a locked
 * inode structure is returned.
 *
 * printf warning: no inodes -- if the inode
 *	structure is full
 * panic: no imt -- if the mounted file
 *	system is not in the mount table.
 *	"cannot happen"
 */
struct inode * iget (dev_t dev, ino_t ino)
{
	register struct inode *ip;
	register struct mount *mp;
	register struct inode *oip;
	register struct buf *bp;
	register struct dinode *dp;

loop:
	oip = NULL;
	for(ip = &inode[0]; ip < &inode[NINODE]; ip++) {
		if(ino == ip->i_number && dev == ip->i_dev) {
			if((ip->i_flag&ILOCK) != 0) {
				ip->i_flag |= IWANT;
				ksleep(ip, PINOD);
				goto loop;
			}
			if((ip->i_flag&IMOUNT) != 0) {
				for(mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
				if(mp->m_inodp == ip) {
					dev = mp->m_dev;
					ino = ROOTINO;
					goto loop;
				}
				panic("no imt");
			}
			ip->i_count++;
			ip->i_flag |= ILOCK;
			return(ip);
		}
		if(oip==NULL && ip->i_count==0)
			oip = ip;
	}
	ip = oip;
	if(ip == NULL) {
		printk("Inode table overflow\n");
	u.errno = ENFILE;
		return(NULL);
	}
	ip->i_dev = dev;
	ip->i_number = ino;
	ip->i_flag = ILOCK;
	ip->i_count++;
	ip->i_un.i_lastr = 0;
	bp = bread(dev, itod(ino));
	/*
	 * Check I/O errors
	 */
	if((bp->b_flags&B_ERROR) != 0) {
		brelse(bp);
		iput(ip);
		return(NULL);
	}
	dp = bp->b_un.b_dino;
	dp += itoo(ino);
	iexpand(ip, dp);
	brelse(bp);
	return(ip);
}



/*
 * Decrement reference count of
 * an inode structure.
 * On the last reference,
 * write the inode out and if necessary,
 * truncate and deallocate the file.
 */
void iput (struct inode *ip)
{

	if(ip->i_count == 1) {
		ip->i_flag |= ILOCK;
		if(ip->i_nlink <= 0) {
			itrunc(ip);
			ip->i_mode = 0;
			ip->i_flag |= IUPD|ICHG;
			ifree(ip->i_dev, ip->i_number);
		}
		time_t t = v7_time(NULL);
		iupdat(ip, &t, &t);
		prele(ip);
		ip->i_flag = 0;
		ip->i_number = 0;
	}
	ip->i_count--;
	prele(ip);
}

/*
 * Check accessed and update flags on
 * an inode structure.
 * If any are on, update the inode
 * with the current time.
 */
void iupdat (struct inode *ip, time_t *ta, time_t *tm)
{
	struct buf *bp;
	struct dinode *dp;
	register char *p1;
	char *p2;
	int i;

	if((ip->i_flag&(IUPD|IACC|ICHG)) != 0) {
		if(getfs(ip->i_dev)->s_ronly)
			return;
		bp = bread(ip->i_dev, itod(ip->i_number));
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return;
		}
		dp = bp->b_un.b_dino;
		dp += itoo(ip->i_number);
		dp->di_mode = ip->i_mode;
		dp->di_nlink = ip->i_nlink;
		dp->di_uid = ip->i_uid;
		dp->di_gid = ip->i_gid;
		dp->di_size = ip->i_size;
		p1 = (char *)dp->di_addr;
		p2 = (char *)ip->i_un.i_addr;
		for(i=0; i<NADDR; i++) {
			*p1++ = *p2++;
			if(*p2++ != 0 && (ip->i_mode&IFMT)!=IFMPC
			   && (ip->i_mode&IFMT)!=IFMPB)
				printk("iaddress > 2^24\n");
			*p1++ = *p2++;
			*p1++ = *p2++;
		}
		if(ip->i_flag&IACC)
			dp->di_atime = *ta;
		if(ip->i_flag&IUPD)
			dp->di_mtime = *tm;
		if(ip->i_flag&ICHG)
			dp->di_ctime = v7_time(NULL);
		ip->i_flag &= ~(IUPD|IACC|ICHG);
		bdwrite(bp);
	}
}
static void tloop (dev_t dev, daddr_t bn, int f1, int f2)
{
	int i;
	struct buf *bp;
	daddr_t *bap=NULL;
	daddr_t nb;

	bp = NULL;
	for(i=NINDIR-1; i>=0; i--) {
		if(bp == NULL) {
			bp = bread(dev, bn);
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				return;
			}
			bap = bp->b_un.b_daddr;
		}
		nb = bap[i];
		if(nb == (daddr_t)0)
			continue;
		if(f1) {
			brelse(bp);
			bp = NULL;
			tloop(dev, nb, f2, 0);
		} else
			bfree(dev, nb);
	}
	if(bp != NULL)
		brelse(bp);
	bfree(dev, bn);
}

/*
 * kfree all the disk blocks associated
 * with the specified inode structure.
 * The blocks of the file are removed
 * in reverse order. This FILO
 * algorithm will tend to maintain
 * a contiguous kfree list much longer
 * than FIFO.
 */
void itrunc (struct inode *ip)
{
	int i;
	dev_t dev;
	daddr_t bn;

	i = ip->i_mode & IFMT;
	if (i!=IFREG && i!=IFDIR)
		return;
	dev = ip->i_dev;
	for(i=NADDR-1; i>=0; i--) {
		bn = ip->i_un.i_addr[i];
		if(bn == (daddr_t)0)
			continue;
		ip->i_un.i_addr[i] = (daddr_t)0;
		switch(i) {

		default:
			bfree(dev, bn);
			break;

		case NADDR-3:
			tloop(dev, bn, 0, 0);
			break;

		case NADDR-2:
			tloop(dev, bn, 1, 0);
			break;

		case NADDR-1:
			tloop(dev, bn, 1, 1);
		}
	}
	ip->i_size = 0;
	ip->i_flag |= ICHG|IUPD;
}



/*
 * Bmap defines the structure of file system storage
 * by returning the physical block number on a device given the
 * inode and the logical block number in a file.
 * When convenient, it also leaves the physical
 * block number of the next block of the file in rablock
 * for use in read-ahead.
 */
static daddr_t bmap (struct inode *ip, daddr_t bn, int rwflg)
{
	int i=0;
	struct buf *bp, *nbp;
	int j, sh;
	daddr_t nb, *bap;
	dev_t dev;
	bap = NULL;
	if(bn < 0) {
	u.errno = EFBIG;
		return ((daddr_t)0);
	}
	dev = ip->i_dev;
	rablock = 0;

	/*
	 * blocks 0..NADDR-4 are direct blocks
	 */
	if(bn < NADDR-3) {
		i = bn;
		nb = ip->i_un.i_addr[i];
		if(nb == 0) {
			if(rwflg==B_READ || (bp = balloc(dev))==NULL)
				return((daddr_t)-1);
			nb = bp->b_blkno;
			bdwrite(bp);
			ip->i_un.i_addr[i] = nb;
			ip->i_flag |= IUPD|ICHG;
		}
		if(i < NADDR-4)
			rablock = ip->i_un.i_addr[i+1];
		return(nb);
	}

	/*
	 * addresses NADDR-3, NADDR-2, and NADDR-1
	 * have single, double, triple indirect blocks.
	 * the first step is to determine
	 * how many levels of indirection.
	 */
	sh = 0;
	nb = 1;
	bn -= NADDR-3;
	for(j=3; j>0; j--) {
		sh += NSHIFT;
		nb <<= NSHIFT;
		if(bn < nb)
			break;
		bn -= nb;
	}
	if(j == 0) {
	u.errno = EFBIG;
		return((daddr_t)0);
	}

	/*
	 * fetch the address from the inode
	 */
	nb = ip->i_un.i_addr[NADDR-j];
	if(nb == 0) {
		if(rwflg==B_READ || (bp = balloc(dev))==NULL)
			return ((daddr_t)-1);
		nb = bp->b_blkno;
		bdwrite(bp);
		ip->i_un.i_addr[NADDR-j] = nb;
		ip->i_flag |= IUPD|ICHG;
	}

	/*
	 * fetch through the indirect blocks
	 */
	for(; j<=3; j++) {
		bp = bread(dev, nb);
		if(bp->b_flags & B_ERROR) {
			brelse(bp);
			return((daddr_t)0);
		}
		bap = bp->b_un.b_daddr;
		sh -= NSHIFT;
		i = (bn>>sh) & NMASK;
		nb = bap[i];
		if(nb == 0) {
			if(rwflg==B_READ || (nbp = balloc(dev))==NULL) {
				brelse(bp);
				return((daddr_t)-1);
			}
			nb = nbp->b_blkno;
			bdwrite(nbp);
			bap[i] = nb;
			bdwrite(bp);
		} else
			brelse(bp);
	}

	/*
	 * calculate read-ahead.
	 */
	if(i < NINDIR-1)
		rablock = bap[i+1];
	return(nb);
}
/*
 * Move n bytes at byte location
 * &bp->b_un.b_addr[o] to/from (flag) the
 * user/kernel (u.segflg) area starting at u.base.
 * Update all the arguments by the number
 * of bytes moved.
 *
 * There are 2 algorithms,
 * if source address, dest address and count
 * are all even in a user copy,
 * then the machine language copyin/copyout
 * is called.
 * If not, its done byte-by-byte with
 * cpass and passc.
 */
static error_t iomove (void* base, void* cp, size_t n, int flag)
{
#if 1
	// don't worry about copying to diffrent memory spaces right now
	if(flag==B_WRITE)
		memcpy(cp,base,n);
	else
		memcpy(base,cp,n);
	return u.errno=0;
#else
	register t;

	if (n==0)
		return;
	if(u.u_segflg != 1 &&
	  (n&(NBPW-1)) == 0 &&
	  ((int)cp&(NBPW-1)) == 0 &&
	  ((int)u.u_base&(NBPW-1)) == 0) {
		if (flag==B_WRITE)
			if (u.u_segflg==0)
				t = copyin(u.u_base, (caddr_t)cp, n);
			else
				t = copyiin(u.u_base, (caddr_t)cp, n);
		else
			if (u.u_segflg==0)
				t = copyout((caddr_t)cp, u.u_base, n);
			else
				t = copyiout((caddr_t)cp, u.u_base, n);
		if (t) {
		u.errno = EFAULT;
			return;
		}
		u.u_base += n;
		u.u_offset += n;
		u.u_count -= n;
		return;
	}
	if (flag==B_WRITE) {
		do {
			if ((t = cpass()) < 0)
				return;
			*cp++ = t;
		} while (--n);
	} else
		do {
			if(passc(*cp++) < 0)
				return;
		} while (--n);
#endif
}



// geti

// rdwi.c - bread and butter.  modified to be detached from
// the user global
/*
 * Read the file corresponding to
 * the inode pointed at by the argument.
 * The actual read arguments are found
 * in the variables:
 *	u_base		core address for destination
 *	u_offset	byte offset in file
 *	u_count		number of bytes to read
 *	u_segflg	read to kernel/user/user I
 */
int readi(register struct inode *ip,void* base, off_t offset, size_t count)
{
	size_t scount = count;
	struct buf *bp;
	daddr_t lbn, bn;
	off_t diff;
	size_t on, n;
	int type;

	if(offset < 0) return u.errno =EINVAL;
	if(count ==0) return 0;
	u.errno = 0;
	ip->i_flag |= IACC;

	type = ip->i_mode&IFMT;
	if (type==IFCHR || type==IFMPC) {
		dev_t dev = (dev_t)ip->i_un.i_rdev;
		cdevsw[major(dev)].d_read(dev);
		return u.errno;
	}
	do {
		dev_t dev = ip->i_dev;
		lbn = bn = offset >> BSHIFT;
		on = offset & BMASK;
		n = min((size_t)(BSIZE-on), count);
		if (type!=IFBLK && type!=IFMPB) {
			diff = ip->i_size - offset;
			if(diff <= 0) return 0;

			if(diff < n)
				n = diff;
			bn = bmap(ip, bn, B_READ);
			if(u.errno) return u.errno;
			dev = ip->i_dev;
		} else
			rablock = bn+1;
		if ((long)bn<0) {
			bp = geteblk();
			clrbuf(bp);
		} else if (ip->i_un.i_lastr+1==lbn)
			bp = breada(dev, bn, rablock);
		else
			bp = bread(dev, bn);
		ip->i_un.i_lastr = lbn;
		n = min((unsigned)n, BSIZE-bp->b_resid);
		if (n!=0){
			iomove(base, bp->b_un.b_addr+on, n, B_READ);
			base += n;
			offset += n;
			count -= n;
		}
		brelse(bp);
	} while(u.errno==0 && count!=0 && n>0);
	return u.errno ? u.errno : (scount-count);
}

/*
 * Write the file corresponding to
 * the inode pointed at by the argument.
 * The actual write arguments are found
 * in the variables:
 *	u_base		core address for source
 *	u_offset	byte offset in file
 *	u_count		number of bytes to write
 *	u_segflg	write to kernel/user/user I
 */
int writei(struct inode *ip, const void* base, off_t offset, size_t count)
{
	size_t scount = count;
	struct buf *bp;
	dev_t dev;
	daddr_t bn;
	size_t n, on;
	int type;

	if(offset < 0) return u.errno = EINVAL;

	dev = (dev_t)ip->i_un.i_rdev;
	type = ip->i_mode&IFMT;
u.errno = 0;
	if (type==IFCHR || type==IFMPC) {
		ip->i_flag |= IUPD|ICHG;
		cdevsw[major(dev)].d_write(dev);
		return u.errno;
	}
	if (count == 0) return 0;

	do {
		bn = offset >> BSHIFT;
		on = offset & BMASK;
		n = min((size_t)(BSIZE-on), count);
		if (type!=IFBLK && type!=IFMPB) {
			bn = bmap(ip, bn, B_WRITE);
			if((long)bn<0) return 0;

			dev = ip->i_dev;
		}
		if(n == BSIZE)
			bp = getblk(dev, bn);
		else
			bp = bread(dev, bn);
		if(iomove((void*)base, bp->b_un.b_addr+on, n, B_WRITE))
			brelse(bp);
		else
			bdwrite(bp);
		if(offset > ip->i_size &&
		   (type==IFDIR || type==IFREG))
			ip->i_size = offset;
		ip->i_flag |= IUPD|ICHG;
	} while(u.errno==0 && count!=0);
	return u.errno ? u.errno : (scount-count);
}
// internal error access
/*
 * Check mode permission on inode pointer.
 * Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the
 * read-only status of the file
 * system is checked.
 * Also in WRITE, prototype text
 * segments cannot be written.
 * The mode is shifted to select
 * the owner/group/other fields.
 * The super user is granted all
 * permissions.
 */
static int iaccess (struct inode *ip, int m)
{
	if(m == IWRITE) {
		if(getfs(ip->i_dev)->s_ronly != 0) {
			u.errno = EROFS;
			return u.errno;
		}
		//if (ip->i_flag&ITEXT)		/* try to free text */
		//	xrele(ip);
		//if(ip->i_flag & ITEXT) {
		//	u.u_error = ETXTBSY;
		//	return(1);
		//}
	}
	if(u.u_uid == 0)
		return (0); //  super user
	if(u.u_uid != ip->i_uid) {
		m >>= 3;
		if(u.u_gid != ip->i_gid)
			m >>= 3;
	}
	if((ip->i_mode&m) != 0)
		return(0);
	u.errno = EACCES;
	return -u.errno;

}
// so... took the idea from xv6 on name searching
// again, the idea is to decouple the file system from the operating
// system itself

//PAGEBREAK!
// Directories

int namecmp(const char *s, struct buf* dbp)
{
  return strncmp(s, dbp->b_un.b_addr, dbp->b_size);
}


// Paths

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
static const char* skipelem(const char *path, struct buf* dbp)
{
  const char *s;
  int len;

  while(*path == '/') path++;
  if(*path == 0) return 0;
  s = path;
  while(*path != '/' && *path != 0) path++;
  len = path - s;
  if(len >= dbp->b_size)
    memmove(dbp->b_un.b_addr, s, dbp->b_size);
  else {
    memmove(dbp->b_un.b_addr, s, len);
    dbp->b_un.b_addr[len] = 0;
  }
  while(*path == '/')
    path++;
  return path;
}


struct nami_return {
	struct inode* ip;  	// inode of the file, NULL meains not found
	struct inode* dp;   // dir of the file
	off_t offset;	  	// offset of the direct of the file
	off_t eo;	  		// offset of the direct of the file
};

enum name_search_t {
	NAME_FOUND =0,
	NAME_ERROR = -1,
	NAME_NOTFOUND = -2,
};

static enum name_search_t namei_bksearch(const char* name, struct nami_return* a){
	if((a->dp->i_mode&IFMT) != IFDIR) {u.errno = ENOTDIR; return -1;} //u.errno = ENOTDIR;
	a->ip = NULL;
	a->offset = 0;
	a->eo = 0;
	// make sure its a dir and we have access
	iaccess(a->dp, IEXEC);
	if(u.errno) return NAME_ERROR; // we don't have permision
	struct buf *bp=NULL;
	while(a->offset <= a->dp->i_size) {
		bp = bread(a->dp->i_dev,
			bmap(a->dp, (daddr_t)(a->offset>>BSHIFT), B_READ));
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return NAME_ERROR; // block read error
		}
		struct dirent* de;
		for(de = &bp->b_un.b_dirent[0]; de < (bp->b_un.b_dirent+NDIRENT);
				de++,a->offset+=sizeof(struct dirent)) {
			if(de->d_ino == 0) {
				if(a->eo == 0) a->eo = a->offset;
				continue;
			}
			if(strncmp(name, de->d_name, sizeof(de->d_name))==0) {
				brelse(bp); // we have a name match
				if(!(name[0] == '.' && name[1]=='\0')) // stop a lock loop
					a->ip = iget(a->dp->i_dev, de->d_ino);
				return a->ip ? NAME_FOUND : NAME_ERROR;
			}
		}
		brelse(bp); // release the block
	}
	return NAME_NOTFOUND;
}

static int check_access(struct inode* ip, int mode) {
	if(mode & FREAD){
		iaccess(ip,IREAD);
		if(u.errno) return -1;
	}
	if(mode & (FWRITE | FCREAT |FTRUNC)){
		iaccess(ip,IWRITE);
		if(u.errno) return -1;
	}
	return 0;
}
static int namei_delete_entry(struct nami_return* a){
	if(a->ip == NULL){
		u.errno = ENOENT;
		return -1; // file no file to delete
	}
	if(a->ip->i_nlink > 1) {
		u.errno = EACCES; // to many links ot this file
		return -1; // file no file to delete
	}
	struct dirent de;
	de.d_ino = 0;
	if(writei(a->dp,&de, a->eo,sizeof(de.d_ino))){
		return -1;
	}
	itrunc(a->ip);
	a->ip=NULL;
	return 0;
}
static int namei_create_entry(const char* name, struct nami_return* a){
	if(a->ip != NULL){
		u.errno = EEXIST;
		return -1; // file exisits
	}
	struct dirent de;
	u.errno = 0;
	a->ip = ialloc(a->dp->i_dev);
	if(a->ip==NULL) return -1;
	de.d_ino = a->ip->i_number;
	strncpy(de.d_name,name,sizeof(de.d_name));
	if(writei(a->dp,&de, a->eo,sizeof(struct dirent))){
		iput(a->ip);
		a->ip=NULL;
		return -1;
	}
	return 0;
}
// return 1 for found
// it will create or delete depending on the mode
// check u.errno if something went wrong
struct inode* namei(const char* path, int mode)
{
	struct nami_return a;
	struct buf* dbp;
	u.errno = 0; // clear error
	if(path[0] == '\0'){
		u.errno = ENOTDIR;
		return NULL;
	}
	dbp = geteblk(); // used for name data and other buffer stuff
	char* name = dbp->b_un.b_addr;
	//memcpy(name,path,strlen(path));
	a.dp =  u.cdir; // current dir
	if(path[0] == '/') a.dp = u.rdir; // root dir
	iget(a.dp->i_dev, a.dp->i_number); // incrment and lock the node
	a.ip = NULL; // make sure ifile is clear in case of erro
	enum name_search_t r = NAME_NOTFOUND;
	while((path=skipelem(path,dbp))) {
		iaccess(a.dp,IEXEC); // we can trasverse it
		if(u.errno) {
			iput(a.dp);
			return NULL;
		}
		r = namei_bksearch(name,&a);
		if(path[0] == '\0') break; // found and done
		// more parts to the dir, so we need to swap
		if(a.dp != a.ip) iput(a.dp);
		// if we found the part and the part we found is a dir, keep going
		if(r == NAME_FOUND && ((a.ip->i_mode & IFMT) == IFDIR)){

			a.dp = a.ip;
			a.ip = NULL;
			continue; // swap it
		}else if(r == NAME_NOTFOUND)  u.errno = ENOENT;
		break;
	} // this took some time to clean up the gotos
	if(r != NAME_ERROR && !check_access(a.dp,mode)) {
		do {
			if((mode & FTRUNC) && namei_delete_entry(&a)) break;
			if((mode & FCREAT) && namei_create_entry(name,&a)) break;
		} while(0);
	}
	if(a.dp) iput(a.dp);
	if(u.errno && a.ip){ iput(a.ip); a.ip=NULL; }
	return a.ip;
}
/*
 * Allocate a user file descriptor
 * and a file structure.
 * Initialize the descriptor
 * to point at the file structure.
 *
 * no file -- if there are no available
 * 	file structures.
 */

struct file * falloc()
{
	struct file *fp = NULL;
	if(LIST_EMPTY(&ffreelist)) {
	u.errno = ENFILE;
	} else {
		fp = LIST_FIRST(&ffreelist);
		LIST_REMOVE(fp,f_list);
		LIST_INSERT_HEAD(&u.ofile, fp, f_list);
		fp->f_count++;
		fp->f_un.f_offset = 0;
	}
	return fp;
}
void ffree(struct file* fp) {
	if(fp->f_count==1){
		LIST_REMOVE(fp,f_list);
		LIST_INSERT_HEAD(&ffreelist, fp, f_list);
	}
	--fp->f_count;
}
/*
 * Convert a user supplied
 * file descriptor into a pointer
 * to a file structure.
 * Only task is to check range
 * of the descriptor.
 */
struct file * getf(int f)
{
	struct file *fp;
	f-= FILEDESCOFFSET;
	if(0 <= f && f < NFILE) {
		fp = &file[f];
		if(fp->f_count >0) return fp; // if its not in use don't return it
	}
u.errno = EBADF;
	return(NULL);
}

/*
 * Internal form of close.
 * Decrement reference count on
 * file structure.
 * Also make sure the pipe protocol
 * does not constipate.
 *
 * Decrement reference count on the inode following
 * removal to the referencing file structure.
 * Call device handler on last close.
 */
void closef(struct file * fp)
{
	register struct inode *ip;
	int flag, mode;
	dev_t dev;
	bdevsw_fuc_dev cfunc;
	struct chan *cp;

	if(fp == NULL) return;
	if (fp->f_count > 1) {
		fp->f_count--;
		return;
	}
	ip = fp->f_inode;
	flag = fp->f_flag;
	cp = fp->f_un.f_chan;
	dev = (dev_t)ip->i_un.i_rdev;
	mode = ip->i_mode;

	plock(ip);
	ffree(fp); // free the file
	if(flag & FPIPE) {
		ip->i_mode &= ~(IREAD|IWRITE);
		kwakeup(ip+1);
		kwakeup(ip+2);
	}
	iput(ip);

	switch(mode&IFMT) {

	case IFCHR:
	case IFMPC:
		cfunc = cdevsw[major(dev)].d_close;
		break;

	case IFBLK:
	case IFMPB:
		cfunc = bdevsw[major(dev)].d_close;
		break;
	default:
		return;
	}

//	if ((flag & FMP) == 0)
		for(fp=file; fp < &file[NFILE]; fp++)
			if (fp->f_count && fp->f_inode==ip)
				return;
	cfunc(dev, flag, cp);
}
/*
 * openi called to allow handler
 * of special files to initialize and
 * validate before actual IO.
 */
void  openi (struct inode *ip, int rw)
{
	dev_t dev;
	register unsigned int maj;

	dev = (dev_t)ip->i_un.i_rdev;
	maj = major(dev);
	switch(ip->i_mode&IFMT) {

	case IFCHR:
	case IFMPC:
		//if(maj >= nchrdev)
		//	goto bad;
		cdevsw[maj].d_open(dev, rw);
		break;

	case IFBLK:
	case IFMPB:
		//if(maj >= nblkdev)
		//	goto bad;
		bdevsw[maj].d_open(dev, rw);
	}
	return;

//bad:
//	u.u_error = ENXIO;
}
/*
 * Max allowable buffering per pipe.
 * This is also the max size of the
 * file created to implement the pipe.
 * If this size is bigger than 5120,
 * pipes will be implemented with large
 * files, which is probably not good.
 */
#define	PIPSIZ	4096

/*
 * The sys-pipe entry.
 * Allocate an inode on the root device.
 * Allocate 2 file structures.
 * Put it all together with flags.
 */
int pipe (int filedes[2]) // int* wf_pipe, int* rf_pipe) // returns fill disc
{
	extern int pipedev;
	struct inode *ip;
	struct file *rf, *wf;

	if((ip = ialloc(pipedev)) == NULL) return -1;
	if((rf = falloc()) == NULL) {
		iput(ip);
		return -1;
	}
	if((wf = falloc()) == NULL) {
		ffree(rf);
		iput(ip);
		return -1;
	}
	filedes[0] = rf->f_desc;
	filedes[1]= wf->f_desc;
	wf->f_flag = FWRITE|FPIPE;
	wf->f_inode = ip;
	rf->f_flag = FREAD|FPIPE;
	rf->f_inode = ip;
	ip->i_count = 2;
	ip->i_mode = IFREG;
	ip->i_flag = IACC|IUPD|ICHG;
	return 0;
}

/*
 * Read call directed to a pipe.
 */
int readp (struct file *fp, void* base,  size_t count,off_t offset)
{
	struct inode *ip;
	ip = fp->f_inode;
	do {
		plock(ip);
		// If nothing in the pipe, wait.
		if (ip->i_size == 0) {
			/*
			 * If there are not both reader and
			 * writer active, return without
			 * satisfying read.
			 */
			prele(ip);
			if(ip->i_count < 2) return -1;
			ip->i_mode |= IREAD;
			ksleep(ip+2, PPIPE);
			continue;
		}
		// Read and return
		readi(ip,base,offset,count);
		offset+=count;
		count-=count;
		base +=count;
		/*
		 * If reader has caught up with writer, reset
		 * offset and size to 0.
		 */
		if (offset == ip->i_size) {
			fp->f_un.f_offset = 0;
			ip->i_size = 0;
			if(ip->i_mode & IWRITE) {
				ip->i_mode &= ~IWRITE;
				kwakeup(ip+1);
			}
		}
		prele(ip);
	} while(0);
	return u.errno ? -1 : offset;
}

/*
 * Write call directed to a pipe.
 */
int writep (struct file *fp, void* base, size_t count,off_t offset)
{
	struct inode *ip;
	size_t c = count;
	ip = fp->f_inode;
	int ret = 0;
	while(c>0) {
		plock(ip);
		/*
		 * If there are not both read and
		 * write sides of the pipe active,
		 * return error and signal too.
		 */
		if(ip->i_count < 2) {
			prele(ip);
			u.errno = EPIPE;
			ret = -u.errno ;
			signal(SIGPIPE, NULL);
			break;
		}
		/*
		 * If the pipe is full,
		 * wait for reads to deplete
		 * and truncate it.
		 */
		if(ip->i_size >= PIPSIZ) {
			ip->i_mode |= IWRITE;
			prele(ip);
			ksleep(ip+1, PPIPE);
			continue;
		}

		/*
		 * Write what is possible and
		 * loop back.
		 * If writing less than PIPSIZ, it always goes.
		 * One can therefore get a file > PIPSIZ if write
		 * sizes do not divide PIPSIZ.
		 */

		offset = ip->i_size;
		count = min((unsigned)c, (unsigned)PIPSIZ);
		c -= count;
		writei(ip,base,offset,c);
		offset+=count;
		count-=count;
		base +=count;

		prele(ip);
		if(ip->i_mode&IREAD) {
			ip->i_mode &= ~IREAD;
			kwakeup((caddr_t)ip+2);
		}
	}
	return ret;
}

/*
 * common code for read and write calls:
 * check permissions, set base, count, and offset,
 * and switch out to readi, writei, or pipe code.
 */
int rdwr(int fdes, caddr_t cbuf, size_t count, int mode)
{
	struct file *fp;
	struct inode *ip;
	off_t offset;
	int ret;
	fp = getf(fdes);
	if(fp == NULL || (fp->f_flag&mode) == 0) return -EBADF;
	offset=fp->f_un.f_offset;
	if((fp->f_flag&FPIPE) != 0) {
		if(mode == FREAD)
			ret=readp(fp,cbuf,offset, count);
		else
			ret=writep(fp,cbuf,offset, count);
	} else {
		ip = fp->f_inode;
	//	if (fp->f_flag&FMP)
	///		offset = 0;
	//	else
		if((ip->i_mode&(IFCHR&IFBLK)) == 0)
			plock(ip);
		if(mode == FREAD)
			ret=readi(ip,cbuf,offset,count);
		else
			ret=writei(ip,cbuf,offset,count);
		if((ip->i_mode&(IFCHR&IFBLK)) == 0)
			prele(ip);
		if (ret > 0)// && (fp->f_flag&FMP) == 0)
			fp->f_un.f_offset += ret;//-u.u_count;
	}
	return ret;
}


// ok the sys calls go here now, the defines, the bread and butter
/*
 * read system call
 */
int v7_read(int fdesc, caddr_t data, size_t count)
{
	return rdwr(fdesc, data, count, FREAD);
}

/*
 * write system call
 */
int v7_write(int fdesc, caddr_t data, size_t count)
{
	return rdwr(fdesc, data, count, FWRITE);
}
/*
 * close system call
 */
void v7_close (int	fdesc)
{
	register struct file *fp;
	fp = getf(fdesc);
	if(!fp) {
	u.errno = EBADF;
		return;
	}
	ffree(fp);
	closef(fp);
}

/*
 * seek system call
 */
off_t  v7_seek (int	fdesc, off_t	off, int	sbase)
{
	off_t ret = -1;
	do {
		struct file * fp = getf(fdesc);
		if(!fp) {
		u.errno = EBADF;
			break;
		}
		if(fp->f_flag&(FPIPE)){//|FMP)) {
		u.errno = ESPIPE;
			return -1;
		}
		switch(sbase) {
			case SEEK_SET:
				fp->f_un.f_offset = off;
				break;
			case SEEK_CUR:
				fp->f_un.f_offset += off;
				break;
			case SEEK_END:
				fp->f_un.f_offset = off +  fp->f_inode->i_size;
				break;
			}
		ret=fp->f_un.f_offset;
	} while(0);
	return ret;
}


int v7_open (const char	*fname, int mode,...) {
	struct file *fp;
	if(mode & O_TRUNC) mode |= O_CREAT; // make sure if we are using O_TRUNC we are also doing create
	mode++;
	struct inode* ip = namei(fname, mode);
	// chekc for O_TRUNC
	if(ip && u.errno==0){// we already know if we have access
		prele(ip);
		if((fp = falloc())!=NULL) {
			fp->f_flag = mode&(FREAD|FWRITE);
			fp->f_inode = ip;
			openi(ip, mode&FWRITE);
			if(u.errno == 0) return fp->f_desc;
			ffree(fp);
		}
	}
	if(ip) iput(ip);
	return -1;
}
/*
 * The basic routine for fstat and stat:
 * get the inode and pass appropriate parts back.
 */
static int stat1 (struct inode *ip, struct stat *ub, off_t pipeadj)
{
	struct dinode *dp;
	struct buf *bp;
	struct stat ds;
	time_t t = v7_time(NULL);
	iupdat(ip, &t, &t);
	if(u.errno) return -1;
	/*
	 * first copy from inode table
	 */
	ds.st_dev = ip->i_dev;
	ds.st_ino = ip->i_number;
	ds.st_mode = ip->i_mode;
	ds.st_nlink = ip->i_nlink;
	ds.st_uid = ip->i_uid;
	ds.st_gid = ip->i_gid;
	ds.st_rdev = (dev_t)ip->i_un.i_rdev;
	ds.st_size = ip->i_size - pipeadj;
	/*
	 * next the dates in the disk
	 */
	bp = bread(ip->i_dev, itod(ip->i_number));
	if(u.errno) return -1;
	dp = bp->b_un.b_dino;
	dp += itoo(ip->i_number);
	ds.st_atime = dp->di_atime;
	ds.st_mtime = dp->di_mtime;
	ds.st_ctime = dp->di_ctime;
	brelse(bp);
	if(u.errno) return -1;
	memcpy(ub,&ds,sizeof(ds));
	return 0;
	//if (copyout((caddr_t)&ds, (caddr_t)ub, sizeof(ds)) < 0)
	//	u.u_error = EFAULT;
}
/*
 * the fstat system call.
 */
int v7_stat (int fdes, struct stat *sb)
{
	struct file *fp;
	fp = getf(fdes);
	if(fp == NULL) return -1;
	return stat1(fp->f_inode, sb, fp->f_flag&FPIPE? fp->f_un.f_offset: 0);

}

/*
 * the stat system call.
 */
int v7_fstat (const char *fname, struct stat *sb)
{
	struct inode* ip = namei(fname, 0);
	if(ip == NULL) return -1;
	stat1(ip, sb, (off_t)0);
	iput(ip);
	return 0;
}

