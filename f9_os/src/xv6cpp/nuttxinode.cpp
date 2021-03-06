/*
 * nuttxinode.cpp
 *
 *  Created on: Apr 24, 2017
 *      Author: warlo
 */

#include "nuttxinode.h"
#include "bitmap.h"
#include <cassert>
#include <cstring>

#define CONFIG_SEM_PREALLOCHOLDERS 20
#define getpid() (0) /* filler */
#define pid_t int16_t
namespace nuttx {
#define NO_HOLDER ((pid_t)-1);
	using alloc_table_t = bitops::bitmap_table_t<nuttx_inode, 64>;
	static alloc_table_t alloc_table;

	void* nuttx_inode::operator new(size_t size){
		assert(size < alloc_table_t::TYPE_SIZE);
		return alloc_table.alloc();
	}
	void nuttx_inode::operator delete(void *ptr){
		alloc_table.free(ptr);
	}
	struct inode_sem_s
	{
	  bitops::simple_semaphore   sem;     /* The semaphore */
	  pid_t   holder;  /* The current holder of the semaphore */
	  int16_t count;   /* Number of counts held */
	  inode_sem_s() : sem(1), holder(-1), count(0) {}
	  void take(){
		  pid_t me = getpid();
		  if(me == holder){
			  count++;
			  assert(count > 0);
		  } else {
			  sem.wait();
			  holder= me;
			  count = 1;
		  }
	  }
	  void give(){
		  assert(holder == getpid());
		  if(count >1) count--;
		  else {
			  holder = NO_HOLDER;
			  count =0;
			  sem.post();
		  }
	  }
	};
	static inode_sem_s g_inode_sem;
	static nuttx_inode  _g_rootnode("");
	static nuttx_inode *g_root_inode = &_g_rootnode;

	int nuttx_inode::dir_compare(const char *fname, const char *nname)
	{
	  if (!nname) return 1;
	  if (!fname)return -1;
	  for (;;)
	    {
	      if(*nname=='\0'){
	          if (dir_end(*fname))
	              return 0; /* Yes.. return match */
	          else
	              return 1;/* No... return find name > node name */
	      }
	      else if (dir_end(*fname)){ /* At end of find name? */
	          return -1; /* Yes... return find name < node name */
	        }
	      /* Check for non-matching characters */
	      else if (*fname > *nname)  return 1;
	      else if (*fname < *nname)  return -1;
	      else { fname++; nname++; }
	    }
	  return 0; // never gets here
	}
	const char *nuttx_inode::dir_nextname(const char *name)
	{
		while (*name && !dir_sep(*name)) name++;
		if (*name) name++; // skip over the '/'
		return name;
	}
	const char *nuttx_inode::dir_basename(const char *name)
	{
		const char *basename = nullptr;
		for (;;) {
			name = dir_nextname(name);
			if (name == nullptr || *name == '\0') break;
			basename = name;
		}
		return basename;
	}
	nuttx_inode *nuttx_inode::root_find(const char *path, const char **relpath){
		if (path == nullptr || path[0] == '\0' || !dir_sep(path[0]))  return nullptr;
		return g_root_inode->find(++path,relpath);
	}
	nuttx_inode *nuttx_inode::find(const char *path, const char **relpath,nuttx_inode** parent)
	{
		nuttx_inode * n;
		LIST_FOREACH(n, &_child, _peer) {
			if(dir_compare(path, n->_name)==0) {
				path = dir_nextname(path);
		          if (*path == '\0' || n->_flags == type::mountpt){
		        	  if(relpath) *relpath = path;
		        	  if(parent) *parent = this;
		        	  return n; // found
		          } else {
		        	  return n->find(path,relpath); // go up a level
		          }
			}
		}
		// not found
		if(relpath) *relpath = path;
		if(parent) *parent = this;
		return nullptr;
	}
	nuttx_inode::nuttx_inode(const char* name, type t) :  _crefs(1), _flags(t), _mode(0),  _child{nullptr} {
		::strncpy(_name, name, sizeof(_name)-1);
		_name[sizeof(_name)-1] = 0;
	}
	nuttx_inode::~nuttx_inode() {
		//release();
		assert(_crefs==0);
	}
	void nuttx_inode::release()
	{
		if (bitops::atomic_sub_return(1,&_crefs) <= 0){
			g_inode_sem.take();
			LIST_REMOVE(this,_peer); // we should be unlinked already but this just makes sure
			nuttx_inode*c,*n;
			LIST_FOREACH_SAFE(c, &_child, _peer,n)
				c->release(); // release all the children
			g_inode_sem.give();
			_crefs = 0;
			delete this;
		}
	}
	void nuttx_inode::link(nuttx_inode* n) {
		assert(n);
		n->addref();
		LIST_INSERT_HEAD(&_child,n,_peer);
	}
	nuttx_inode * nuttx_inode::link(const char* name, type t){
		const char* relpath;
		nuttx_inode *parent;
		nuttx_inode *n = find(name,&relpath,&parent);
		if(n) return n->_flags == t ? n : nullptr;  //exists so return exisiting one, null if wrong type
		n = new nuttx_inode(relpath,t); // already is 1
		LIST_INSERT_HEAD(&_child,n,_peer);
		return n;
	}
	nuttx_inode * nuttx_inode::unlink(const char* path){
		nuttx_inode *n = find(path,NULL);
		if(n) n->unlink();// carful refrence is sitll +1
		return n;
	}
	void nuttx_inode::unlink(){
		g_inode_sem.take();
		LIST_REMOVE(this,_peer);
		release();
		g_inode_sem.give();
	}

} /* namespace xv6 */
