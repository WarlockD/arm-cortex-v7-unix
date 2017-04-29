#include "param.h"
#include "systm.h"
#include "user.h"
//#include "reg.h"
#include "file.h"
#include "inode.h"
#include "sys.h"


/*
 * common code for read and write calls:
 * check permissions, set base, count, and offset,
 * and switch out to readi, writei, or pipe code.
 */
static int rdwr(int fdesc, void* data, size_t size, int m)
{
	struct file* fp;
	if((fp = getf(fdesc)) == NULL || (fp->f_flag&m) == 0) {
		u.u_error = EBADF;
		return -1;
	}
	u.u_base = data;
	u.u_count = size;
	u.u_offset = fp->f_offset;
	if(m==FREAD)
		readi(fp->f_inode);
	else
		writei(fp->f_inode);

	size_t rsize = size-u.u_count;
	fp->f_offset += rsize;
	return rsize;
}
/*
 * read system call
 */
int v6_read(int fdesc, void* data, size_t size)
{
	return rdwr( fdesc,data,size, FREAD);
}

/*
 * write system call
 */
int v6_write(int fdesc, void* data, size_t size)
{
	return rdwr( fdesc,data,size, FWRITE);
}
/*
 * common code for open and creat.
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
static int open1(struct inode* rip, int m, int trf)
{
	struct file *fp;

	if(trf != 2) {
		if(m&FREAD)
			iaccess(rip, IREAD);
		if(m&FWRITE) {
			iaccess(rip, IWRITE);
			if((rip->i_mode&IFMT) == IFDIR)
				u.u_error = EISDIR;
		}
	}
	if(u.u_error==0) {
		if(trf) itrunc(rip);
		prele(rip);
		if ((fp = falloc()) != NULL) {
			fp->f_flag = m&(FREAD|FWRITE);
			fp->f_inode = rip;
			openi(rip, m&FWRITE);
			if(u.u_error == 0) return fp->f_desc;
			u.u_ofile[fp->f_desc] = NULL;	// as well as atomic swap, got to use pointers man sigh
			fp->f_count--; // need to atomic decment this
		}
	}
	iput(rip);
	return -1;
}

/*
 * creat system call
 */
int creat(const char* filename, mode_t mode)
{
	u.u_dirp = filename; // find another way from this
	struct inode *ip = namei(1);
	if(ip == NULL) {
		if(u.u_error) return -1;
		ip = maknode(mode&07777);
		if (ip==NULL) return -1;
		return open1(ip, FWRITE, 2);
	} else
		return open1(ip, FWRITE, 1);
}
/*
 * open system call
 */
int v6_open(const char* filename, mode_t mode)
{
	u.u_dirp = filename; // find another way from this
	struct inode *ip= namei(0);
	if(ip == NULL) return -1;
	return open1(ip, ++mode, 0);
}




/*
 * close system call
 */
void  v6_close(int fdesc)
{
	struct file *fp = getf(fdesc);
	if(fp == NULL) return;
	u.u_ofile[fdesc] = NULL;
	closef(fp);
}

/*
 * seek system call
 */
off_t  v6_lseek(int fdesc,off_t offset, int rdir)
{

	struct file* fp = getf(fdesc);
	fp = getf(fdesc);
	if(fp == NULL) return -1;
	if(fp->f_flag&FPIPE) {
		u.u_error = ESPIPE;
		return -1;
	}
	off_t noff = 0;
	switch(rdir){
	case SEEK_SET:	/* set file offset to offset */
		noff = offset;
		break;
	case SEEK_CUR:	/* set file offset to current plus offset */
		noff = fp->f_offset + offset;
		break;
	case SEEK_END:	/* set file offset to EOF plus offset */
		noff = offset + fp->f_inode->i_size;
		break;
	}
	if(noff < 0)
		noff = 0;
	else
		if(noff > fp->f_inode->i_size)
			noff = fp->f_inode->i_size;
	fp->f_offset = noff;
	return noff;
}


int v6_link(const char* fname, const char* flink)
{
	struct inode* ip;
	struct inode* xp;

	u.u_dirp = fname;
	ip = namei(0);
	if(ip == NULL) return -1;
	if(ip->i_nlink >= 127) {
		u.u_error = EMLINK;
		goto out;
	}
	if((ip->i_mode&IFMT)==IFDIR && !suser())
		goto out;
	/*
	 * unlock to avoid possibly hanging the namei
	 */
	ip->i_flag &= ~ILOCK;
	u.u_dirp = flink;
	xp = namei(1);
	if(xp != NULL) {
		u.u_error = EEXIST;
		iput(xp);
	}
	if(u.u_error)
		goto out;
	if(u.u_pdir->i_dev != ip->i_dev) {
		iput(u.u_pdir);
		u.u_error = EXDEV;
		goto out;
	}
	wdir(ip);
	ip->i_nlink++;
	ip->i_flag |= IUPD;
	return 0;
out:
	iput(ip);
	return -1;
}

/*
 * mknod system call
 */
int v6_mknod(const char* fname, int mode, dev_t dev)
{
	struct inode* ip;
	u.u_dirp = fname;
	if(suser()) {
		ip = namei(1);
		if(ip != NULL) {
			u.u_error = EEXIST;
			goto out;
		}
	}
	if(u.u_error) return -1;
	ip = maknode(mode);
	if (ip==NULL) return -1;
	ip->i_addr[0] = dev;
	return 0;

out:
	iput(ip);
	return -1;
}

/*
 * sleep system call
 * not to be confused with the sleep internal routine.
 */
void v6_sleep(time_t ts)
{
	spl6(); // what is spl7 humm
	time_t t = ktimeget();
	time_t d  = t + ts;
	while((d-(t=ktimeget()))  > 0){
		if((tout-t) <=0 || (tout-d) >0 ) tout = d;
		ksleep((void*)tout,PSLEP); // humm intresting
	}
	spl0();
}
