#include <os\list.hpp>
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
};

slist_head<test_entry> head1(&test_entry::list1);
slist_head<test_entry>  head2(&test_entry::list2);

test_entry stuff[20];


extern "C" void try_list() {
	int count = 22;
#if 0
	for(auto &a : stuff) {
		SLIST_INSERT_HEAD(&head2,&a,list2);
	}
#endif
	for(auto &a : stuff) {
		a.value = count++;
		head1.insert_head(&a);
	//	slist_insert_head(head1,&a,&test_entry::list1);
	}
	for(auto& v : head1){
		printk("List num %d\r\n", v.value);
	}
	printk("done!\r\n");
	while(1){
		HAL_Delay(200);
		printk("tick!\r\n");
	}
}

