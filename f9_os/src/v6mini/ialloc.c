#include "param.h"
//#include "systm.h"
#include "fs.h"
#include "conf.h"
#include "buf.h"
#include "inode.h"
#include "user.h"
struct inode inode[NINODE]; // refrence of all inodes

struct mount mount[NMOUNT]; // refrence of all inodes


/*
 * iinit is called once (from main)
 * very early in initialization.
 * It reads the root's super block
 * and initializes the current date
 * from the last modified date.
 *
 * panic: iinit -- cannot read the super
 * block. Usually because of an IO error.
 */
void iinit()
{
	struct buf *cp, *bp;

	bp = bread(rootdev, 1);
	cp = getblk(NODEV,0);
	if(u.u_error) kpanic("iinit");
	memcpy(cp->b_addr,bp->b_addr, BSIZE);
	brelse(bp);
	mount[0].m_bufp = cp;
	mount[0].m_dev = rootdev;
	struct filsys* fs = (struct filsys*)cp->b_addr;
	fs->s_flock = 0;
	fs->s_ilock = 0;
	fs->s_ronly = 0;
	ktimeset(fs->s_time[0] | fs->s_time[1] << 16);
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
static int badblock(struct filsys *fp, daddr_t bn, dev_t dev)
{
	if (bn < fp->s_isize+2 || bn >= fp->s_fsize) {
		kpanic("badblock");
		return(1);
	}
	return(0);
}
/*
 * alloc will obtain the next available
 * free disk block from the free list of
 * the specified device.
 * The super block has up to 100 remembered
 * free blocks; the last of these is read to
 * obtain 100 more . . .
 *
 * no space on dev x/y -- when
 * the free list is exhausted.
 */
struct buf * balloc(dev_t dev)
{
	daddr_t bno;
	struct buf *bp;
	//struct inode *ip;

	struct filsys* fp = getfs(dev);
	while(fp->s_flock)
		ksleep(&fp->s_flock, PINOD);
	do {
		if(fp->s_nfree <= 0)
			goto nospace;
		bno = fp->s_free[--fp->s_nfree];
		if(bno == 0)
			goto nospace;
	} while (badblock(fp, bno, dev));
	if(fp->s_nfree <= 0) {
		fp->s_flock++;
		bp = bread(dev, bno);
		union fsblk* b = (union fsblk*)bp->b_addr;
		memcpy(&fp->s_nfree, &b->free, sizeof(b->free));
#if 0
		uint16_t* ip = (uint16_t* )bp->b_addr;
		fp->s_nfree = *ip++;
		memcpy(fp->s_free, ip, 100);
		//bcopy(ip, fp->s_free, 100);
#endif
		brelse(bp);
		fp->s_flock = 0;
		kwakeup(&fp->s_flock);
	}
	bp = getblk(dev, bno);
	clrbuf(bp);
	fp->s_fmod = 1;
	return(bp);

nospace:
	fp->s_nfree = 0;
	u.u_error = ENOSPC;
	return (NULL);
}

/*
 * place the specified disk block
 * back on the free list of the
 * specified device.
 */
void bfree(dev_t dev,daddr_t bno)
{
	struct buf *bp;
	struct filsys* fp = getfs(dev);
	while(fp->s_flock)
		ksleep(&fp->s_flock, PINOD);
	if (badblock(fp, bno, dev))
		return;
	if(fp->s_nfree <= 0) {
		fp->s_nfree = 1;
		fp->s_free[0] = 0;
	}
	if(fp->s_nfree >= 100) {
		fp->s_flock++;
		bp = getblk(dev, bno);
		union fsblk* b = (union fsblk*)bp->b_addr;
		memcpy(&b->free, &fp->s_nfree, sizeof(b->free));
#if 0
		uint16_t* ip = bp->b_addr;
		*ip++ = fp->s_nfree;
		memcpy(ip, fp->s_free, sizeof(uint16_t)*100);
#endif
		fp->s_nfree = 0;
		bwrite(bp);
		fp->s_flock = 0;
		kwakeup(&fp->s_flock);
	}
	fp->s_free[fp->s_nfree++] = bno;
	fp->s_fmod = 1;
}



/*
 * Allocate an unused I node
 * on the specified device.
 * Used with file creation.
 * The algorithm keeps up to
 * 100 spare I nodes in the
 * super block. When this runs out,
 * a linear search through the
 * I list is instituted to pick
 * up 100 more.
 */
struct inode* ialloc(dev_t dev)
{
	struct buf *bp;
	ino_t ino;
	struct inode* ip;
	struct filsys* fp = getfs(dev);
	while(fp->s_ilock)
		ksleep(&fp->s_ilock, PINOD);
loop:
	if(fp->s_ninode > 0) {
		ino = fp->s_inode[--fp->s_ninode];
		ip = iget(dev, ino);
		if (ip==NULL)
			return(NULL);
		if(ip->i_mode == 0) {
			ip->i_nlink =0;
			ip->i_uid =0;
			ip->i_gid =0;
			ip->i_size =0;
			memset(&ip->i_addr, 0, sizeof(ip->i_addr));
			ip->i_nlink =0;
			ip->i_nlink =0;
			fp->s_fmod = 1;
			return (ip);
		}
		/*
		 * Inode was allocated after all.
		 * Look some more.
		 */
		iput(ip);
		goto loop;
	}
	// god this is slow
	// searches entire disk for free nodes and puts them on the list
	// now I see why xv6 went with bitmaps, shesh
	fp->s_ilock++;
	ino = 0;
	for(int i=0; i<fp->s_isize; i++) {
		bp = bread(dev, i+2);
		union fsblk* b = (union fsblk*)bp->b_addr;
		for(int j=0; j<COUNTOF(b->dinode); j++) {
			ino++;
			if(b->dinode[j].i_mode != 0) continue;
			for(int k=0; k<NINODE; k++){ // improve!
				if(dev==inode[k].i_dev && ino==inode[k].i_number)
					goto cont;
			}
			fp->s_inode[fp->s_ninode++] = ino;
			if(fp->s_ninode >= 100)
				break;
		cont:;
		}
		brelse(bp);
		if(fp->s_ninode >= 100)
			break;
	}
	fp->s_ilock = 0;
	kwakeup(&fp->s_ilock);
	if (fp->s_ninode > 0)
		goto loop;
	u.u_error = ENOSPC;
	return (NULL);
}

/*
 * Free the specified I node
 * on the specified device.
 * The algorithm stores up
 * to 100 I nodes in the super
 * block and throws away any more.
 */
void ifree(dev_t dev, ino_t ino)
{
	struct filsys* fp = getfs(dev);
	if(fp->s_ilock || fp->s_ninode >= 100) return;
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
 * in core free-block and i-node
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
struct filsys* getfs(dev_t dev)
{
	for(struct mount *p = &mount[0]; p < &mount[NMOUNT]; p++)
	if(p->m_bufp != NULL && p->m_dev == dev) {
		struct filsys* fp = (struct filsys*)p->m_bufp->b_addr;
		size_t n1 = fp->s_nfree;
		size_t n2 = fp->s_ninode;
		if(n1 > 100 || n2 > 100) {
			fp->s_nfree = 0;
			fp->s_ninode = 0;
		}
		return fp;
	}
	kpanic("getfs");
	return NULL;
}

/*
 * update is the internal name of
 * 'sync'. It goes through the disk
 * queues to initiate sandbagged IO;
 * goes through the I nodes to write
 * modified nodes; and it goes through
 * the mount table to initiate modified
 * super blocks.
 */
static int updlock = 0;
void iupdate()
{
	struct inode *ip;
	struct mount *mp;
	struct buf *bp;
	struct filsys *fp;
	if(updlock) return;
	updlock++;
	for(mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if(mp->m_bufp != NULL) {
			fp = (struct filsys *)mp->m_bufp->b_addr;
			if(fp->s_fmod==0 || fp->s_ilock!=0 ||
					fp->s_flock!=0 || fp->s_ronly!=0)
				continue;
			bp = getblk(mp->m_dev, 1);
			fp->s_fmod = 0;
			time_t t = ktimeget();
			fp->s_time[0] = t >> 16;
			fp->s_time[1] = t & 0xFFFF;
			memcpy(bp->b_addr, fp, sizeof(struct filsys));
			bwrite(bp);
		}
	for(ip = &inode[0]; ip < &inode[NINODE]; ip++)
		if((ip->i_flag&ILOCK) == 0) {
			ip->i_flag |= ILOCK;
			iupdat(ip);
			prele(ip);
		}
	updlock = 0;
	bflush(NODEV);
}
