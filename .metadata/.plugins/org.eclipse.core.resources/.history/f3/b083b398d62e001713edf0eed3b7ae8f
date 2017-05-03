#include <os/slist.hpp>
#include <sys\queue.h>
#include <os\printk.h>
#include <stm32f7xx.h>
#include <stm32746g_discovery.h>
using namespace sys;

struct test_entry {
	int value;
	slist_entry<test_entry> list1;
	slist_entry<test_entry> list2;

	constexpr test_entry() : value(0) {}
	constexpr test_entry(int i) : value(i) {}
};

//slist_head<test_entry> head1(head_field::field);
//slist_head<test_entry>  head2(&test_entry::list2);
slist_head<test_entry,&test_entry::list1> head1;
slist_head<test_entry,&test_entry::list2>  head2;

test_entry stuff[5] = { 1,2,3,4,5 };


extern "C" void try_list() {
	int count = 22;

	for(auto &a : stuff) {
		head1.insert_head(&a);
	//	slist_insert_head(head1,&a,&test_entry::list1);
	}
	test_entry* e=nullptr;
	for(auto& v : head1){
		if(v.value == 3) e = &v;
		printk("List num %d\r\n", v.value);
	}
	head1.remove(e);
	printk("removing 3\r\n");
	for(auto& v : head1){
		printk("List num %d\r\n", v.value);
	}
	printk("removing head\r\n");
	head1.remove_head();
	for(auto& v : head1){
		printk("List num %d\r\n", v.value);
	}
	for(auto &a : stuff) {
		a.value = count++;
		head1.remove(&a);
	//	slist_insert_head(head1,&a,&test_entry::list1);
	}
	printk("done!\r\n");
	while(1){
	//	printk("tick!\r\n");
	}
}

