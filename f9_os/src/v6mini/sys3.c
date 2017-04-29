#include "buf.h"
#include "param.h"
#include "systm.h"
#include "user.h"
//#include "reg.h"
#include "file.h"
#include "fs.h"
#include "inode.h"
#include "sys.h"



/*
 * The basic routine for fstat and stat:
 * get the inode and pass appropriate parts back.
 */
static int stat1(struct inode* ip,struct stat* ub)
{
	iupdat(ip);
	struct buf* bp = bread(ip->i_dev, itod(ip->i_number));
	if(bp == NULL) return -1;
	struct dinode* di = (struct dinode*)(bp->b_addr + itoo(ip->i_number));
	ub->st_dev = (di->i_mode&(IFBLK|IFCHR)) ? di->i_addr[0] : ip->i_dev;
	ub->st_blksize = BSIZE;
	ub->st_gid = ip->i_gid;
	ub->st_uid = ip->i_uid;
	ub->st_mode = di->i_mode;
	ub->st_size = ip->i_size;
	ub->st_mtime = di->i_mtime[0] << 16 | di->i_mtime[1];
	ub->st_atime = di->i_atime[0] << 16 | di->i_atime[1];
	ub->st_nlink = ip->i_nlink;
	ub->st_ino = ip->i_number;
	ub->st_blocks = (ip->i_size+ BMASK) >> BSHIFT;
	brelse(bp);
	return 0;
}
/*
 * the fstat system call.
 */
int v6_fstat(int fdesc, struct stat* st)
{
	struct file* fp = getf(fdesc);
	if(fp == NULL) return -1;
	return stat1(fp->f_inode, st);
}

/*
 * the stat system call.
 */
int v6_stat(const char* fname, struct stat* st)
{
	u.u_dirp = fname;
	struct inode* ip = namei(0);
	if(ip == NULL) return -1;
	int r = stat1(ip, st);
	iput(ip);
	return r;
}


/*
 * the dup system call.
 */

int v6_dup(int fdesc)
{
	extern int ufalloc(struct file* fp); // fio.c
	struct file* fp = getf(fdesc);
	if(fp == NULL) return -1;
	int i;
	if ((i = ufalloc(fp)) < 0) return -1;
	u.u_ofile[i] = fp;
	fp->f_count++;
	return i;
}
/*
 * Common code for mount and umount.
 * Check that the user's argument is a reasonable
 * thing on which to mount, and return the device number if so.
 */
extern int nblkdev;
dev_t getmdev(const char* fname)
{
	dev_t d;
	struct inode* ip;
	u.u_dirp = fname;
	ip = namei(0);
	if(ip == NULL) return -1;
	if((ip->i_mode&IFMT) != IFBLK)
		u.u_error = ENXIO;
	d = ip->i_addr[0];
	if(major(d)>= nblkdev)
		u.u_error = ENXIO;
	iput(ip);
	return(d);
}
/*
 * the mount system call.
 */
int v6_mount(const char* root_dir, const char* device_name, int ro)
{
	struct inode *ip;
	struct buf* bp ;
	struct mount  *smp = NULL;
	dev_t d;
	d= getmdev(root_dir);
	if(u.u_error) return -1;
	u.u_dirp = device_name;
	if((ip = namei(0)) == NULL) return -1;
	if(ip->i_count!=1 || (ip->i_mode&(IFBLK&IFCHR))!=0){
		u.u_error = EBUSY;
		iput(ip);
		return -1;
	}

	for_each_array(mp,mount) {
		if(mp->m_bufp != NULL) {
			if(d == mp->m_dev){
				u.u_error = EBUSY;
				iput(ip);
				return -1;
			}
		} else
		if(smp == NULL) smp = mp;
	}
	if(smp == NULL){
		u.u_error = EBUSY;
		iput(ip);
		return -1;
	}
	bp  = bread(d, 1);
	if(u.u_error) {
		brelse(bp);
		u.u_error = EBUSY;
		iput(ip);
		return -1;
	}
	smp->m_inodp = ip;
	smp->m_dev = d;
	smp->m_bufp = getblk(NODEV,0);
	memcpy(smp->m_bufp->b_addr, bp->b_addr,sizeof(struct filsys));
	struct filsys* fp = (struct filsys*)smp->m_bufp->b_addr;
	fp->s_ilock = 0;
	fp->s_flock = 0;
	fp->s_ronly = ro & 1;
	brelse(bp);
	ip->i_flag |= IMOUNT;
	prele(ip);
	return 0;
}

/*
 * the umount system call.
 */
extern void iupdate();
int v6_umount(const char* root_dir)
{
	dev_t d;
	struct inode *ip;
	iupdate();
	d = getmdev(root_dir);
	if(u.u_error) return -1;
	for_each_array(mp,mount) {
		if(mp->m_bufp!=NULL && d==mp->m_dev){
			ip = mp->m_inodp;
			ip->i_flag &= ~IMOUNT;
			iput(ip);
			struct buf* bp = mp->m_bufp;
			mp->m_bufp = NULL;
			brelse(bp);
			return 0;
		}
	}
	u.u_error = EINVAL;
	return -1;
}


