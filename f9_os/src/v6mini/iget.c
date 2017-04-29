#include "param.h"
//#include "systm.h"
#include "fs.h"
#include "conf.h"
#include "buf.h"
#include "inode.h"
#include "user.h"

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
 * panic: no imt -- if the mounted file
 *	system is not in the mount table.
 *	"cannot happen"
 */
struct inode * iget(dev_t dev, ino_t ino)
{
	struct inode *p;
	struct inode *ip;

loop:
	ip = NULL;
	for(p = &inode[0]; p < &inode[NINODE]; p++) {
		if(dev==p->i_dev && ino==p->i_number) {
			if((p->i_flag&ILOCK) != 0) {
				p->i_flag |= IWANT;
				ksleep(p, PINOD);
				goto loop;
			}
			if((p->i_flag&IMOUNT) != 0) {
				for_each_array(mp, mount)
					if(mp->m_inodp == p) {
						ip = mp->m_inodp;
						dev = mp->m_dev;
						ino = ROOTINO;
						goto loop;
					}
				kpanic("iget");
			}
			p->i_count++;
			p->i_flag |= ILOCK;
			return(p);
		}
		if(ip==NULL && p->i_count==0)
			ip = p;
	}
	if((p=ip) == NULL) {
		u.u_error = ENFILE;
		return(NULL);
	}
	p->i_dev = dev;
	p->i_number = ino;
	p->i_flag = ILOCK;
	p->i_count++;
	struct buf* bp = bread(dev, itod(ino));
	// Check I/O errors
	if ((bp->b_flags&B_ERROR)!=0) {
		brelse(bp);
		iput(p);
		return(NULL);
	}
	struct dinode* di = (struct dinode*)(bp->b_addr + itoo(ino));
	p->i_mode = di->i_mode;
	p->i_nlink = di->i_nlink;
	p->i_uid = di->i_uid;
	p->i_gid = di->i_gid;
	p->i_size = di->i_size0 << 16 | di->i_size1;
	memcpy(p->i_addr, di->i_addr, sizeof(di->i_addr));
	brelse(bp);
	return(p);
}

/*
 * Decrement reference count of
 * an inode structure.
 * On the last reference,
 * write the inode out and if necessary,
 * truncate and deallocate the file.
 */
void iput(struct inode *rp)
{
	if(rp->i_count == 1) {
		rp->i_flag |= ILOCK;
		if(rp->i_nlink <= 0) {
			itrunc(rp);
			rp->i_mode = 0;
			ifree(rp->i_dev, rp->i_number);
		}
		iupdat(rp);
		prele(rp);
		rp->i_flag = 0;
		rp->i_number = 0;
	}
	rp->i_count--;
	prele(rp);
}

/*
 * Check accessed and update flags on
 * an inode structure.
 * If either is on, update the inode
 * with the corresponding dates
 * set to the argument tm.
 */
void iupdat(struct inode*rp)
{
	struct buf* bp;

	if((rp->i_flag&IUPD) != 0) {
		if(getfs(rp->i_dev)->s_ronly) return;
		ino_t ino = rp->i_number;
		bp = bread(rp->i_dev, itod(ino));
		struct dinode* di = (struct dinode*)(bp->b_addr + itoo(ino));

		di->i_mode = rp->i_mode;
		di->i_nlink = rp->i_nlink;
		di->i_uid = rp->i_uid;
		di->i_gid = rp->i_gid;
		di->i_size0 = rp->i_size >> 16;
		di->i_size1 = rp->i_size & 0xFFFF;
		memcpy(di->i_addr, rp->i_addr, sizeof(di->i_addr));
		if(rp->i_flag&IUPD) {
			time_t t = ktimeget();
			di->i_mtime[0] = t << 16;
			di->i_mtime[1]= t & 0xFFFF;
		}
		bwrite(bp);
	}
}

/*
 * Free all the disk blocks associated
 * with the specified inode structure.
 * The blocks of the file are removed
 * in reverse order. This FILO
 * algorithm will tend to maintain
 * a contiguous free list much longer
 * than FIFO.
 */
void itrunc(struct inode*rp)
{
	if((rp->i_mode&(IFCHR&IFBLK)) != 0)
		return;
	for(uint16_t* ip = &rp->i_addr[7]; ip >= &rp->i_addr[0]; ip--)
		if(*ip) {
		if((rp->i_mode&ILARG) != 0) {
			struct buf* bp = bread(rp->i_dev, *ip);
			union fsblk* b = (union fsblk*)bp->b_addr;
			for(uint16_t* cp = &b->indr[255]; cp >= &b->indr[0]; cp--)
				if(*cp) bfree(rp->i_dev, *cp);
			brelse(bp);
		}
		bfree(rp->i_dev, *ip);
		*ip = 0;
	}
	rp->i_mode &= ~ILARG;
	rp->i_size = 0;
	rp->i_flag |= IUPD;
}

/*
 * Make a new file.
 */
struct inode* maknode(mode_t mode)
{
	struct inode* ip= ialloc(u.u_pdir->i_dev);
	if (ip==NULL) return NULL;
	ip->i_flag |= IUPD;
	ip->i_mode = mode|IALLOC;
	ip->i_nlink = 1;
	ip->i_uid = u.u_uid;
	wdir(ip);
	return(ip);
}

/*
 * Write a directory entry with
 * parameters left as side effects
 * to a call to namei.
 */
void wdir(struct inode* ip,const char* name)
{
	u.u_dent.u_ino = ip->i_number;
	strncpy(u.u_dent.u_name, name,  DIRSIZ);
	u.u_count = DIRSIZ+2;
	u.u_base = (caddr_t)&u.u_dent;
	writei(u.u_pdir);
	iput(u.u_pdir);
}

void prele(struct inode* rp)
{
	rp->i_flag &= ~ILOCK;
	if(rp->i_flag&IWANT) {
		rp->i_flag &= ~IWANT;
		kwakeup(rp);
	}
}
