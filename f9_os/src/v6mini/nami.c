#include "param.h"
#include "systm.h"
#include "fs.h"
#include "file.h"
#include "conf.h"
#include "buf.h"
#include "inode.h"
#include "user.h"



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
  size_t len;

  while(*path == '/') path++;
  if(*path == 0) return 0;
  s = path;
  while(*path != '/' && *path != 0) path++;
  len = path - s;
  if(len >= BSIZE){
    memmove(dbp->b_addr, s, BSIZE-1);
  	  dbp->b_addr[BSIZE] = 0;
  } else {
    memmove(dbp->b_addr, s, len);
    dbp->b_addr[len] = 0;
  }
  while(*path == '/') path++;
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
typedef struct dirent dirblock_t[sizeof(struct dirent)/BSIZE];
static enum name_search_t namei_bksearch(const char* name, struct nami_return* a){
	if((a->dp->i_mode&IFMT) != IFDIR) {u.u_error = ENOTDIR; return -1;} //u.errno = ENOTDIR;
	a->ip = NULL;
	a->offset = 0;
	a->eo = 0;
	// make sure its a dir and we have access
	iaccess(a->dp, IEXEC);
	if(u.u_error ) return NAME_ERROR; // we don't have permision
	struct buf *bp=NULL;
	while(a->offset <= a->dp->i_size) {
		bp = bread(a->dp->i_dev, bmap(a->dp, u.u_offset>>BSHIFT));
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return NAME_ERROR; // block read error
		}
		dirblock_t* dea  = (dirblock_t*)bp->b_addr;
		for_each_array(de,*dea) {
			a->offset+=sizeof(struct dirent);
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
		if(u.u_error) return -1;
	}
	if(mode & (FWRITE | FCREAT |FTRUNC)){
		iaccess(ip,IWRITE);
		if(u.u_error) return -1;
	}
	return 0;
}

// return 1 for found
// it will create or delete depending on the mode
// check u.errno if something went wrong
struct inode* namex(int mode)
{
	struct nami_return a;
	struct buf* dbp;
	const char* path = u.u_dirp;
	char* name ;
	u.u_error = 0; // clear error
	if(path[0] == '\0'){
		u.u_error = ENOTDIR;
		return NULL;
	}
	dbp = getblk(NODEV,0); // used for name data and other buffer stuff
	name = dbp->b_addr;
	//memcpy(name,path,strlen(path));
	a.dp =  u.u_cdir; // current dir
	if(path[0] == '/') a.dp = u.rdir; // root dir
	iget(a.dp->i_dev, a.dp->i_number); // incrment and lock the node
	a.ip = NULL; // make sure ifile is clear in case of erro
	enum name_search_t r = NAME_FOUND;
	while(r == NAME_FOUND && u.u_error == 0 && (path=skipelem(path,dbp))) {
		iaccess(a.dp,IEXEC); // we can trasverse it
		if(u.u_error ) {
			iput(a.dp);
			break;
		}
		r = namei_bksearch(name,&a);
		if(r == NAME_FOUND) {
			if(path[0] == '\0') break; // found and no more parts
			if(((a.ip->i_mode & IFMT) == IFDIR)){
				if(a.dp != a.ip) iput(a.dp);
				a.dp = a.ip;
				a.ip = NULL;
				continue; // swap it
			} else u.u_error = ENOENT; // its not dir
		}
	} // this took some time to clean up the gotos
	if(u.u_error){
		if(a.dp) iput(a.dp);
		if(a.ip)iput(a.ip);
		return NULL;
	} else {
		u.u_offset = a.eo;
		u.u_pdir = a.dp;
		return a.ip;
	}
}




/*
 * Return the next character from the
 * user string pointed at by dirp.
 */
char uchar()
{
	int c = fubyte(u.u_dirp++);
	if(c == -1)
		u.u_error = EFAULT;
	return (c);
}

/*
 * Convert a pathname into a pointer to
 * an inode. Note that the inode is locked.
 *
 * flag = 0 if name is sought
 *	1 if name is to be created
 *	2 if name is to be deleted
 */
struct inode *namei(int flag)
{
	return namex(flag);
#if 0
	struct inode *dp;
	struct buf* bp;
	int c;
	caddr_t cp;
	off_t eo;

	/*
	 * If name starts with '/' start from
	 * root; otherwise start from current dir.
	 */
	u.u_error =0;
	dp = u.u_cdir;
	if((c=uchar()) == '/')
		dp = rootdir;
	iget(dp->i_dev, dp->i_number);
	while(c == '/')
		c = uchar();
	if(c == '\0' && flag != 0) {
		u.u_error = ENOENT;
		goto out;
	}

cloop:
	/*
	 * Here dp contains pointer
	 * to last component matched.
	 */

	if(u.u_error)
		goto out;
	if(c == '\0')
		return(dp);

	/*
	 * If there is another component,
	 * dp must be a directory and
	 * must have x permission.
	 */

	if((dp->i_mode&IFMT) != IFDIR) {
		u.u_error = ENOTDIR;
		goto out;
	}
	if(iaccess(dp, IEXEC))
		goto out;

	/*
	 * Gather up name into
	 * users' dir buffer.
	 */

	cp = &u.u_dbuf[0];
	while(c!='/' && c!='\0' && u.u_error==0) {
		if(cp < &u.u_dbuf[DIRSIZ])
			*cp++ = c;
		c = uchar();
	}
	while(cp < &u.u_dbuf[DIRSIZ])
		*cp++ = '\0';
	while(c == '/')
		c = uchar();
	if(u.u_error)
		goto out;

	/*
	 * Set up to search a directory.
	 */

	u.u_offset = 0;
	eo = 0;
	u.u_count = dp->i_size / (DIRSIZ+2);
	bp = NULL;

eloop:

	/*
	 * If at the end of the directory,
	 * the search failed. Report what
	 * is appropriate as per flag.
	 */

	if(u.u_count == 0) {
		if(bp != NULL)
			brelse(bp);
		if(flag==1 && c=='\0') {
			if(iaccess(dp, IWRITE))
				goto out;
			u.u_pdir = dp;
			if(eo)
				u.u_offset = eo-DIRSIZ-2;
			else
				dp->i_flag |= IUPD;
			return(NULL);
		}
		u.u_error = ENOENT;
		goto out;
	}

	/*
	 * If offset is on a block boundary,
	 * read the next directory block.
	 * Release previous if it exists.
	 */

	if((u.u_offset&0777) == 0) {
		if(bp != NULL)
			brelse(bp);
		bp = bread(dp->i_dev, bmap(dp, u.u_offset/BSIZE));
	}

	/*
	 * Note first empty directory slot
	 * in eo for possible creat.
	 * String compare the directory entry
	 * and the current component.
	 * If they do not match, go back to eloop.
	 */
	memcpy( &u.u_dent,bp->b_addr + u.u_offset,(DIRSIZ+2) );
	u.u_offset += DIRSIZ+2;
	u.u_count--;
	if(u.u_dent.u_ino == 0) {
		if(eo == 0)
			eo = u.u_offset;
		goto eloop;
	}
	for(cp = &u.u_dbuf[0]; cp < &u.u_dbuf[DIRSIZ]; cp++)
		if(*cp != cp[u.u_dent.u_name - u.u_dbuf])
			goto eloop;

	/*
	 * Here a component matched in a directory.
	 * If there is more pathname, go back to
	 * cloop, otherwise return.
	 */

	if(bp != NULL)
		brelse(bp);
	if(flag==2 && c=='\0') {
		if(iaccess(dp, IWRITE))
			goto out;
		return(dp);
	}
	ino_t dev = dp->i_dev;
	iput(dp);
	dp = iget(dev, u.u_dent.u_ino);
	if(dp == NULL)
		return(NULL);
	goto cloop;

out:
	iput(dp);
	return(NULL);
#endif
}


