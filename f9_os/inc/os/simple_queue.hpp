#ifndef OS_SIMPLE_QUEUE_HPP_

#define OS_SIMPLE_QUEUE_HPP_


#include <cstdint>
#include <cstddef>
#include <functional>
#include <type_traits>


namespace os {
	struct sq_entry_t
	{
		sq_entry_t *flink;
		sq_entry_t * next() { return flink; }
	};

	struct dq_entry_t
	{
		dq_entry_t *flink;
		dq_entry_t *blink;
		dq_entry_t * next() { return flink; }
		dq_entry_t * prev() { return blink; }
	};
	struct sq_queue_t
	{
	  sq_entry_t *head;
	  sq_entry_t *tail;
	  sq_queue_t() : head(nullptr), tail(nullptr) {}

	  bool empty() const { return head == nullptr; }
	  sq_entry_t * peek() { return head; }
	  void addfirst(sq_entry_t* node){
		  node->flink = head;
		  if (!head)tail = node;
		  head = node;
	  }
	  void addlast(sq_entry_t* node){
		  node->flink = nullptr;
		  if (!head){
		      head = node;
		      tail = node;
		 }else{
		      tail->flink = node;
		      tail        = node;
		 }
	  }
	  void addafter(sq_entry_t *prev,  sq_entry_t *node){
		  if (!head || prev == tail){
		      addlast(node);
		  } else {
		      node->flink = prev->flink;
		      node->flink = node;
		  }
	  }
	  void cat(sq_queue_t& other){
		  /* If queue2 is empty, then just move queue1 to queue2 */

		  if (empty()) {
			  std::swap(*this,other);
		   }else if (!other.empty()) {
		      tail->flink = other.head;
		      tail = other.tail;
		      other = sq_queue_t();
		    }
	  }
	  sq_entry_t *remafter(sq_entry_t *node){
		  sq_entry_t *ret = node->flink;
		  if(head && ret)
		    {
		      if (tail == ret)
		        {
		          tail = node;
		          node->flink = nullptr;
		        }
		      else
		        {
		          node->flink = ret->flink;
		        }

		      ret->flink = nullptr;
		    }

		  return ret;
	  }
	  void remove(sq_entry_t *node){
		  if (head && node)
		    {
		      if (node == head)
		        {
		          head = node->flink;
		          if (node == tail)
		            {
		              tail = nullptr;
		            }
		        }
		      else
		        {
		          sq_entry_t *prev;

		          for (prev = (sq_entry_t *)head;
		               prev && prev->flink != node;
		               prev = prev->flink);

		          if (prev) remafter(prev);
		        }
		    }
	  }
	  sq_entry_t *remlast(){
		  sq_entry_t *ret = tail;

		   if (ret)
		     {
		       if (head == tail)
		         {
		           head = nullptr;
		           tail = nullptr;
		         }
		       else
		         {
		           sq_entry_t *prev;
		           for (prev = head;
		               prev && prev->flink != ret;
		               prev = prev->flink);

		           if (prev)
		             {
		               prev->flink = nullptr;
		               tail = prev;
		             }
		         }

		       ret->flink = nullptr;
		     }

		   return ret;
	  }
	  sq_entry_t *remfirst(){
		  sq_entry_t *ret = head;
		  if (ret) {
			  head = ret->flink;
			  if (!head) tail = nullptr;
			  ret->flink = nullptr;
		 }

		  return ret;
	  }
	  size_t count() const{
		  size_t count;
		  for (sq_entry_t *node = head, count = 0;
		       node != nullptr;
		       node = node->flink, count++);
		  return count;
	  }
	};

	struct dq_queue_t
	{
	  dq_entry_t *head;
	  dq_entry_t *tail;
	  dq_queue_t() : head(nullptr), tail(nullptr) {}
	  bool empty() const { return head == nullptr; }
	  sq_entry_t * peek() { return head; }
	  void addfirst(dq_entry_t* node);
	  void addlast(dq_entry_t* node);
	  void addafter(dq_entry_t *prev,  dq_entry_t *node);
	  void addbefore(dq_entry_t *next,  dq_entry_t *node);
	  void cat(dq_queue_t* other);
	  size_t count() const;
	};

}














#endif
