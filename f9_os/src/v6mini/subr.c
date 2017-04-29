#include "param.h"
#include "systm.h"
#include "fs.h"
#include "conf.h"
#include "buf.h"
#include "inode.h"
#include "user.h"


/*
 * Bmap defines the structure of file system storage
 * by returning the physical block number on a device given the
 * inode and the logical block number in a file.
 */

daddr_t bmap(struct inode *ip, daddr_t bn)
{
	struct buf* bp;
	daddr_t nb;
	uint16_t* bap;
	int i;
	dev_t d = ip->i_dev;
	if(bn & 0174000) {
		u.u_error = EFBIG;
		return(0);
	}

	if((ip->i_mode&ILARG) == 0) {

		/*
		 * small file algorithm
		 */

		if((bn & ~7) != 0) {

			/*
			 * convert small to large
			 */

			if ((bp = balloc(d)) == NULL)
				return  0;
			 bap = (uint16_t*)bp->b_addr;
			for(i=0; i<8; i++) {
				*bap++ = ip->i_addr[i];
				ip->i_addr[i] = 0;
			}
			ip->i_addr[0] = bp->b_blkno;
			bdwrite(bp);
			ip->i_mode |= ILARG;
			goto large;
		}
		nb = ip->i_addr[bn];
		if(nb == 0 && (bp = balloc(d)) != NULL) {
			bdwrite(bp);
			nb = bp->b_blkno;
			ip->i_addr[bn] = nb;
			ip->i_flag |= IUPD;
		}
		return(nb);
	}

	/*
	 * large file algorithm
	 */

    large:
	i = bn>>8;
	if((nb=ip->i_addr[i]) == 0) {
		ip->i_flag |= IUPD;
		if ((bp = balloc(d)) == NULL)
			return 0;
		ip->i_addr[i] = bp->b_blkno;
	} else
		bp = bread(d, nb);
	bap = (uint16_t*)bp->b_addr;

	/*
	 * normal indirect fetch
	 */

	i = bn & 0377;
	struct buf* nbp;
	if((nb=bap[i]) == 0 && (nbp = balloc(d)) != NULL) {
		nb = nbp->b_blkno;
		bap[i] = nb;
		bdwrite(nbp);
		bdwrite(bp);
	} else
		brelse(bp);
	return nb;
}

/*
 * Pass back  c  to the user at his location u_base;
 * update u_base, u_count, and u_offset.  Return -1
 * on the last character of the user's read.
 * u_base is in the user address space unless u_segflg is set.
 */
int uio_put(int c, struct uio* ui) {
	if(subyte(ui->base, c) < 0) {
		u.u_error = EFAULT;
		return(-1);
	}
	ui->count--;
	ui->offset++;
	ui->base++;
	return(u.u_count == 0? -1: 0);
}

/*
 * Pick up and return the next character from the user's
 * write call at location u_base;
 * update u_base, u_count, and u_offset.  Return -1
 * when u_count is exhausted.  u_base is in the user's
 * address space unless u_segflg is set.
 */
int uio_get(struct uio* ui)
{
	int c;

	if(u.u_count == 0) return(-1);

	if((c=fubyte(ui->base)) < 0) {
		u.u_error = EFAULT;
		return(-1);
	}
	ui->count--;
	ui->offset++;
	ui->base++;
	return(c&0377);
}
// optimize latter
int uio_read(struct uio* ui, void* output, size_t count){
	if(u.u_count == 0) return(-1);
	if(count > ui->count) count = ui->count - count;
	memcpy(output, ui->base, count);
	ui->offset += count;
	ui->count -=count;
	ui->base += count;
	return count;
}

int uio_write(struct uio* ui, const void* input, size_t count){
	if(u.u_count == 0) return(-1);
	if(count > ui->count) count = ui->count - count;
	memcpy(ui->base, input, count);
	ui->offset += count;
	ui->count -=count;
	ui->base += count;
	return count;
}
/*
 * Routine which sets a user error; placed in
 * illegal entries in the bdevsw and cdevsw tables.
 */
void nodev() { u.u_error = ENODEV; }

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */
void nulldev() { }
