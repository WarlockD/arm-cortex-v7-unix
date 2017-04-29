#include "param.h"
#include "inode.h"
#include "user.h"
#include "buf.h"
#include "conf.h"
#include "systm.h"

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
/*
 * Read the file corresponding to
 * the inode pointed at by the argument.
 * The actual read arguments are found
 * in the variables:
 *	u_base		core address for destination
 *	u_offset	byte offset in file
 *	u_count		number of bytes to read
 */
void readi( struct inode *ip)
{
	struct buf* bp;
	daddr_t lbn, bn;
	off_t on;
	size_t n;
	dev_t dev;
	if(u.u_count == 0) return;
	switch(ip->i_mode&IFMT){
	case IFCHR:
		dev =ip->i_addr[0];
		cdevsw[major(dev)].d_read(dev);
		break;
	case IFBLK:
		dev =ip->i_addr[0];
		while(u.u_error==0 && u.u_count>0){
			lbn  =bn = u.u_offset << BSHIFT;
			on = u.u_offset & BMASK;
			n = min(BSIZE-on, u.u_count);
			bp = bread(dev, bn);
			iomove(bp, on, n, B_READ);
			brelse(bp);
		}
		break;
	default: // disk system
		dev =ip->i_dev ;
		while(u.u_error==0 && u.u_count>0){
			lbn  =bn = u.u_offset << BSHIFT;
			on = u.u_offset & BMASK;
			n = min(BSIZE-on, u.u_count);

			off_t od =  ip->i_size-u.u_offset;
			if(od<=0) return; // return error? offset incorrect
			n = min(n, od);
			if ((bn = bmap(ip, lbn)) == 0) return;

			bp = bread(dev, bn);
			iomove(bp, on, n, B_READ);
			brelse(bp);
		}
		break;
	}
}

/*
 * Write the file corresponding to
 * the inode pointed at by the argument.
 * The actual write arguments are found
 * in the variables:
 *	u_base		core address for source
 *	u_offset	byte offset in file
 *	u_count		number of bytes to write
 */
void writei( struct inode *ip)
{
	struct buf* bp;
	daddr_t lbn, bn;
	off_t on;
	size_t n;
	dev_t dev;
	if(u.u_count == 0) return;
	ip->i_flag |= IUPD;
	switch(ip->i_mode&IFMT){
		case IFCHR:
			dev =ip->i_addr[0];
			cdevsw[major(dev)].d_write(dev);
			break;
		case IFBLK:
			dev =ip->i_addr[0];
			while(u.u_error==0 && u.u_count>0){
				lbn  =bn = u.u_offset << BSHIFT;
				on = u.u_offset & BMASK;
				n = min(BSIZE-on, u.u_count);
				bp = (n == BSIZE) ? getblk(dev, bn) : bread(dev, bn);
				iomove(bp, on, n, B_WRITE);
				if(u.u_error != 0)					brelse(bp);
				else if ((u.u_offset & BMASK)==0) 	bawrite(bp);
				else 								bdwrite(bp); // cacheing?
			}
			break;
		default:
			dev =ip->i_dev ;
			while(u.u_error==0 && u.u_count>0){
				lbn  =bn = u.u_offset << BSHIFT;
				on = u.u_offset & BMASK;
				n = min(BSIZE-on, u.u_count);
				if ((bn = bmap(ip, lbn)) == 0) return;
				bp = (n == BSIZE) ? getblk(dev, bn) : bread(dev, bn);
				iomove(bp, on, n, B_WRITE);
				if(u.u_error != 0)					brelse(bp);
				else if ((u.u_offset & BMASK)==0) 	bawrite(bp);
				else 								bdwrite(bp); // cacheing?
				if(u.u_offset > ip->i_size) ip->i_size = u.u_offset;
			}
			break;
	}
}



/*
 * Move 'an' bytes at byte location
 * &bp->b_addr[o] to/from (flag) the
 * user/kernel  area starting at u.base.
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
void iomove(struct buf* bp, off_t o,size_t an,int flag)
{
	caddr_t cp = bp->b_addr + o;
	size_t n = an;
	// we cut out the easy stuff and just do memcpy
	// change this over to dma latter
	if (flag==B_WRITE)
		memcpy(cp, u.u_base,n);
	else
		memcpy(u.u_base, cp, n);
	u.u_base += n;
	u.u_offset+= n;
	u.u_count -= n;

#if 0
	if(((n | cp | u.u_base)&01)==0) {
		if (flag==B_WRITE)
			cp = copyin(u.u_base, cp, n);
		else
			cp = copyout(cp, u.u_base, n);
		if (cp) {
			u.u_error = EFAULT;
			return;
		}
		u.u_base =+ n;
		dpadd(u.u_offset, n);
		u.u_count =- n;
		return;
	}
	if (flag==B_WRITE) {
		while(n--) {
			if ((t = cpass()) < 0)
				return;
			*cp++ = t;
		}
	} else
		while (n--)
			if(passc(*cp++) < 0)
				return;
#endif
}
