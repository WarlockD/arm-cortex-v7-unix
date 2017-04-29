#include "param.h"
#include "user.h"
#include "fs.h"
#include "file.h"
#include "conf.h"
#include "inode.h"
#include "atomic.h"
///#include "reg.h"
struct file file[NFILE]; // refrence of all files
#define file_ref_dec(FP) 		atomic_decr_u8(&(FP)->f_count,1)
#define file_ref_inc(FP) 		atomic_incr_u8(&(FP)->f_count,1)
#define file_ref(FP) 			atomic_incr_u8(&(FP)->f_count,0)
/*
 * Convert a user supplied
 * file descriptor into a pointer
 * to a file structure.
 * Only task is to check range
 * of the descriptor.
 */
struct file * getf(int rf)
{
	if(rf<0 || rf>=NOFILE || u.u_ofile[rf] == NULL){
		u.u_error = EBADF;
		return(NULL);
	}
	return u.u_ofile[rf];
}

/*
 * Internal form of close.
 * Decrement reference count on
 * file structure and call closei
 * on last closef.
 * Also make sure the pipe protocol
 * does not constipate.
 */
void closef(struct file * fp)
{
	if(file_ref_dec(fp) == 0) closei(fp->f_inode, fp->f_flag&FWRITE);
}

/*
 * Decrement reference count on an
 * inode due to the removal of a
 * referencing file structure.
 * On the last closei, switchout
 * to the close entry point of special
 * device handler.
 * Note that the handler gets called
 * on every open and only on the last
 * close.
 */
void closei(struct inode* rip, int rw)
{
	dev_t dev= rip->i_addr[0];
	if(rip->i_count <= 1){
		switch(rip->i_mode&IFMT) {
		case IFCHR:
			if(major(dev) >= nchrdev){u.u_error = ENXIO; return; }
			cdevsw[major(dev)].d_close(dev, rw);
			break;
		case IFBLK:
			if(major(dev) >= nblkdev){u.u_error = ENXIO; return; }
			bdevsw[major(dev)].d_close(dev, rw);
			break;
		}
	}
	iput(rip);
}

/*
 * openi called to allow handler
 * of special files to initialize and
 * validate before actual IO.
 * Called on all sorts of opens
 * and also on mount.
 */
void openi(struct inode* rip, int rw)
{
	dev_t dev= rip->i_addr[0];
	switch(rip->i_mode&IFMT) {
	case IFCHR:
		if(major(dev) >= nchrdev){u.u_error = ENXIO; return; }
		bdevsw[major(dev)].d_open(dev, rw);
		break;
	case IFBLK:
		if(major(dev) >= nblkdev){u.u_error = ENXIO; return; }
		bdevsw[major(dev)].d_open(dev, rw);
		break;
	}
}

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
int iaccess(struct inode* ip, int m)
{
	if(m == IWRITE) {
		if(getfs(ip->i_dev)->s_ronly != 0) {
			u.u_error = EROFS;
			return 1;
		}
	}
	if(u.u_uid == 0) return(0);
	if(u.u_uid != ip->i_uid) m >>= 6;
	if((ip->i_mode&m) != 0) return(0);
	u.u_error = EACCES;
	return 1;
}

/*
 * Look up a pathname and test if
 * the resultant inode is owned by the
 * current user.
 * If not, try for super-user.
 * If permission is granted,
 * return inode pointer.
 */
struct inode * owner()
{
	struct inode *ip;
	if ((ip = namei(0)) == NULL) return NULL;
	if(u.u_uid == ip->i_uid || suser()) return ip;
	iput(ip);
	return NULL;
}

/*
 * Test if the current user is the
 * super user.
 */
int suser()
{
	if(u.u_uid == 0) return(1);
	u.u_error = EPERM;
	return(0);
}
// trying to solve the lopping file issue
// That is if we get a file desc,
/*
 * Allocate a user file descriptor.
 */
int ufalloc(struct file* fp){
	struct file* nullfp = NULL;		// we have a free file struct
	for (int i=0; i<NOFILE; i++){
		// find a file desc
		if(atomic_cas_obj(&u.u_ofile[i], &nullfp, fp)) {
			return i;
		}
	}
	u.u_error = ENFILE;
	return -1;
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
struct file* falloc()
{
	for_each_array(fp,file) { // find a free file struct
		if (file_ref_inc(fp)==1) {
			int i = ufalloc(fp);
			if(i <0) {
				file_ref_dec(fp);
				return NULL;
			}
			fp->f_desc = i; // first file desc
			fp->f_offset = 0;
			return fp;
		}
		file_ref_dec(fp);
	}
	u.u_error = ENFILE;
	return NULL;
}
