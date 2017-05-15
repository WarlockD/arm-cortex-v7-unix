#include "fs.hpp"
#include <os\bitmap.hpp>

#include <cstring>

namespace fs {
	using inode_bitmap_type = bitops::bitmap_alloc_t<fs::inode>;
	inode_bitmap_type g_nodes;

	 inode* inode::g_root = nullptr;

	size_t dirlen(const char* str) {
		size_t siz =0;
		while(*str && !is_sep_char(*str)) { siz++; str++; }
		return siz;
	}
	int dircmp(const char* a, const char* b) {
		if(!a) return -1;
		if(!b) return 1;
		for(;;) {
			if(*b == '\0'){
				if(*a == '\0' || is_sep_char(*a)) return 0; // match
				else return 1;
			} else if(*b == '\0' || is_sep_char(*a)) return -1;
			else if(*a > *b) return -1;
			else if(*b < *a) return 1;
			else { ++a; ++b; }
		}
		return -1; // never gets here
	}
	const char* skip_sep(const char* str) {
		while(*str && is_sep_char(*str)) str++;
		return str;
	}
	void* inode::operator new(std::size_t sz){
		//assert(sz <= inode_bitmap_type::ELEMENT_SIZE);
		return g_nodes.alloc(sz);
	}
    // custom placement delete
    void inode::operator delete(void* ptr)
    {
    	g_nodes.free(ptr);
    }

    inode::inode(const char* name) : i_peer(nullptr), i_child(nullptr), i_crefs(0),i_flags(0), i_mode(0), i_private(0) {
    	::strncpy(i_name,name,31);
    }
	void inode::insert(inode *node,inode *peer,inode *parent){
		  /* If peer is non-null, then new node simply goes to the right
		   * of that peer node.
		   */

		  if (peer)
		    {
		      node->i_peer = peer->i_peer;
		      peer->i_peer = node;
		    }

		  /* If parent is non-null, then it must go at the head of its
		   * list of children.
		   */

		  else if (parent)
		    {
			  node->i_peer    = parent->i_child;
		      parent->i_child = node;
		    }

		  /* Otherwise, this must be the new root_inode */

		  else
		    {
			  node->i_peer = g_root;
		      g_root = node;
		    }
	}
	inode* inode::find(const char *path, const char **relpath){
	  struct inode *node;

	  if (path == nullptr || path[0] == '\0' || is_sep_char(path[0])) return nullptr;
	  addref(); // make sure we arn't collected
	  /* Find the node matching the path.  If found, increment the count of
	   * references on the node.
	   */
	  node = search(&path, nullptr, nullptr, relpath);
	  if (node) node->addref();
	  subref();

	  return node;
	}
	inode *inode::search(const char **path,inode **peer,inode **parent,const char **relpath){
		dir_string name  = (*path + 1); /* Skip over leading '/' */
		inode *node  = this;
		inode *left  = nullptr;
		inode *above = nullptr;

	while (node){
		int result = name.compare(node->name());
		/* Case 1:  The name is less than the name of the node.
		* Since the names are ordered, these means that there
		* is no peer node with this name and that there can be
		* no match in the fileystem.
		*/
		if (result < 0){
			node = nullptr;
			break;
		}
		/* Case 2: the name is greater than the name of the node.
		* In this case, the name may still be in the list to the
		* "right"
		*/
		else if (result > 0){
			left = node;
			node = node->i_peer;
		} else { /* The names match */
			/* Now there are three more possibilities:
			*   (1) This is the node that we are looking for or,
			*   (2) The node we are looking for is "below" this one.
			*   (3) This node is a mountpoint and will absorb all request
			*       below this one
			*/

			name = name.nextname();
			if (name.empty() || node->is_mountpt()){
				/* Either (1) we are at the end of the path, so this must be the
				* node we are looking for or else (2) this node is a mountpoint
				* and will handle the remaining part of the pathname
				*/

				if (relpath) *relpath = name;

				break;
			}
			else{
				/* More to go, keep looking at the next level "down" */

				above = node;
				left  = nullptr;
				node = node->i_child;
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

	if (peer) *peer = left;
	if (parent) *parent = above;
	*path = name.c_str();
	return node;
	}


};
