/*
 * vnode.h
 *
 *  Created on: Apr 24, 2017
 *      Author: warlo
 */

#ifndef XV6CPP_VNODE_H_
#define XV6CPP_VNODE_H_

#include "list.h"
#include "os.h"
namespace xv6 {

/* flags */
	class vnode {
		static constexpr vnode* INVALID = (vnode*)-1;
	public:
		static constexpr size_t DIRSIZE = 20;
		inline static constexpr bool isdirch(char c) { return  c == '/' || c == '\\'; }
		inline static constexpr bool eodir(char c) { return c == '\0' || isdirch(c); }
		inline static const char *nextname(const char *name)
		{
			while (eodir(*name)) name++;
			if(*name) name++;
			return name;
		}
		int compare(const char *fname)
		{
			const char* nname = _name;
			if(!nname) return 1;
			if(!fname) return -1;
			for(;;){
				if(*nname == '\0') {
					if(eodir(*fname)) return 0; //match
					else return 1; // fname > nname
				} else if(eodir(*fname)) return -1; // fname < name
				else if(*fname > *nname) return 1;
				else if(*fname < *nname) return -1;
				else { fname++; nname++; }
			}
			return -2; // compiler, shouldn't get here
		}
		vnode *search(const char **path,vnode **peer, vnode **parent, const char **relpath)
		{
		  const char   *name  = *path; /* Skip over leading '/' */
		  if(isdirch(*name)) name++;
		  vnode *node  = this;
		  vnode *left  = NULL;
		  vnode *above = NULL;

		  while (node)
		    {
		      int result = compare(name);

		      /* Case 1:  The name is less than the name of the node.
		       * Since the names are ordered, these means that there
		       * is no peer node with this name and that there can be
		       * no match in the fileystem.
		       */

		      if (result < 0)
		        {
		          node = nullptr;
		          break;
		        }

		      /* Case 2: the name is greater than the name of the node.
		       * In this case, the name may still be in the list to the
		       * "right"
		       */

		      else if (result > 0)
		        {
		          left = node;
		          node = node->vnext();
		        }

		      /* The names match */

		      else
		        {
		          /* Now there are three more possibilities:
		           *   (1) This is the node that we are looking for or,
		           *   (2) The node we are looking for is "below" this one.
		           *   (3) This node is a mountpoint and will absorb all request
		           *       below this one
		           */

		          name = nextname(name);
		          if (*name == '\0') // || INODE_IS_MOUNTPT(node))
		            {
		              /* Either (1) we are at the end of the path, so this must be the
		               * node we are looking for or else (2) this node is a mountpoint
		               * and will handle the remaining part of the pathname
		               */

		              if (relpath) *relpath = name;
		              break;
		            }
		          else
		            {
		              /* More to go, keep looking at the next level "down" */

		              above = node;
		              left  = nullptr;
		              node = node->_vhead;
		            }
		        }
		    }

		  /* node is null.  This can happen in one of four cases:
		   * With node = NULL
		   *   (1) We went left past the final peer:  The new node
		   *       name is larger than any existing node name at
		   *       that level.
		   *   (2) We broke out in the middle of the list of peers
		   *       because the name was not found in the ordered
		   *       list.
		   *   (3) We went down past the final parent:  The new node
		   *       name is "deeper" than anything that we currently
		   *       have in the tree.
		   * with node != NULL
		   *   (4) When the node matching the full path is found
		   */

		  if (peer)
		    {
		      *peer = left;
		    }

		  if (parent)
		    {
		      *parent = above;
		    }

		  *path = name;
		  return node;
		}

		enum TYPE {
			VFREE =0,
			VLINK, // indirect link, we basicly use head from here
			VDIR,
			VBLKDRV,
			VCHRDRV,
		};
		size_t nlinks() const { return _nlinks; }
		vnode(const char* name) : _vhead(nullptr), _vnext(INVALID), _vprev(&_vnext), _flags(VFREE), _nlinks(0), _name(name) {}
		// these two functions handle the indirect linking of the node
		//
		bool empty() const { return _flags != VLINK ? _vhead == nullptr : _vhead->empty(); }
		void link(vnode* to) {
			unlink(); // free this node
			++to->_nlinks;
			_vhead = to;
			_flags = VLINK;
		}
		void search(const char* dir) {

		}
		void unlink() {
			switch(_flags) {
			case VLINK:
				--_vhead->_nlinks;
				_vhead = nullptr;
				break;
			case VDIR:
				for(vnode *n, *c = _vhead ; c && ((n=c->_vnext),1); c = n)
						c->remove();
				break;
			default: break; // all others
			}
			_flags = VFREE;
		}
		// after this link is detected and works from here
		void insert_head(vnode* vn) {
			if(_flags != VLINK) {
				if((vn->_vnext = _vhead) != nullptr)
					_vhead->_vprev = &vn->_vnext;
				_vhead = vn;
				vn->_vprev = &_vhead;
				vn->_nlinks++;
			} else _vhead->insert_head(vn);
		}
		void insert_after(vnode* elm) {
			if(_flags != VLINK) {
				if((elm->_vnext = _vnext) != nullptr)
						_vprev = &elm->_vnext;
				_vnext = elm;
				elm->_vprev = &_vnext;
				elm->_nlinks++;
			}	_vhead->insert_after(elm);
		}
		void insert_before(vnode* elm) {
			if(_flags != VLINK) {
				elm->_vprev = _vprev;
				elm->_vnext = this;
				*_vprev = elm;
				_vprev = &elm->_vnext;
				elm->_nlinks++;
			}_vhead->insert_before(elm);
		}
		vnode* vnext() { return _flags != VLINK ? _vnext : _vhead->_vnext; }
		// prev is tricky to do without some magic number
		// work on it latter cause forward trasversal is all we really need
		// right now
		//vnode* vprev() {}

		void remove() {
			if(_vnext != nullptr)
				_vnext->_vprev = _vprev;
			*_vprev = _vnext;
			--_nlinks;
		}
		bool is_directory() const { return _vhead !=nullptr; }
		vnode* move_up() { return _vhead; }// move up a directory
		bool equals(const char* name) const { return false; } // ::strncmp(_name,name,16) == 0; }
	protected:
		vnode* _vhead;
		vnode* _vnext;
		vnode** _vprev;// 12 here
		TYPE _flags;		  // 16 sized here
		int _nlinks;
		const char* _name;
	};

} /* namespace xv6 */

#endif /* XV6CPP_VNODE_H_ */
