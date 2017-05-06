#include <os/slist.hpp>
#include <os/hash.hpp>
#include <os/stailq.hpp>
#include <sys\queue.h>
#include <os\printk.h>
#include <stm32f7xx.h>
#include <stm32746g_discovery.h>
#include <os/devices/stm32f7_uart.hpp>

os::device::stm32f7xx::stm32f7_uart<1> dbg_uart(os::device::uart_trasfer_type::isr);

extern "C" void USART1_IRQHandler(){
	dbg_uart.isr_handler();
}
struct test_entry {
	int value;
	list_entry<test_entry> link;
	slist_entry<test_entry> list1;
	slist_entry<test_entry> list2;
	stailq_entry<test_entry> stailq1;
	constexpr test_entry() : value(0) {}
	constexpr test_entry(int i) : value(i) {}
};

//slist_head<test_entry> head1(head_field::field);
//slist_head<test_entry>  head2(&test_entry::list2);
slist_head<test_entry,&test_entry::list1> head1;
slist_head<test_entry,&test_entry::list2>  head2;
stailq_head<test_entry,&test_entry::stailq1>  head3;


struct test_entry_equals {
	constexpr bool operator()(const test_entry&l, const test_entry&r) const {
		return l.value == r.value;
	}
	constexpr bool operator()(const test_entry&l, const int value) const {
		return l.value == value;
	}
};

struct test_entry_hasher {

	constexpr size_t operator()(int x) const {
	    x = ((x >> 16) ^ x) * 0x45d9f3b;
	    x = ((x >> 16) ^ x) * 0x45d9f3b;
	    x = (x >> 16) ^ x;
	    return x;
	}
	constexpr size_t operator()(const test_entry&l) const {
		return (l.value);
	}
};


hash_list<test_entry,&test_entry::link,16, test_entry_hasher,test_entry_equals> hlist;
#include <cstdlib>
test_entry stuff[50];
void setup_stuff(bool random=false){
	int pos =0;
	for(auto &a : stuff) {
		a.value  = random ? rand() : pos;
		 pos++;
	}
}
template<typename T>
void print_stuff(const char* message, T&& list) {
	int pos=0;
	printk("%s\r\n",message);
	for(auto& v : list){
		printk("    %2d = %d\r\n",pos++, v.value);
	}
	printk("\r\n");
}
template<typename T>
void print_buckets(const char* message, T& hash) {
	test_entry_hasher hasher;
	printk("%s\r\n",message);
	int pos = 0;
	for(size_t i=0; i < T::BUCKET_COUNT;i++) {
		auto bucket = hash.dbg_bucket(i);
		if(bucket == nullptr || bucket->empty()) continue;
		pos=0;
		printk("[%3d]: ", i);
		for(auto& v : *bucket) printk("%d\t", v.value);
		printk("\r\n");
	}

}

void hash_uinttest() {
	test_entry* e=nullptr;
	setup_stuff();
	for(auto &a : stuff) {
	//	a.value = rand();
		hlist.insert(&a);
		if(a.value == 3) e = &a;
	//	slist_insert_head(head1,&a,&test_entry::list1);
	}
	print_buckets("hash Start",hlist);

	printk("done!\r\n");
	//print_stuff("hash Start",hlist);
	while(1);
}
void tailq_unittest() {
	test_entry* e=nullptr;
	for(auto &a : stuff) {
		head3.insert_tail(&a);
		if(a.value == 3) e = &a;
	//	slist_insert_head(head1,&a,&test_entry::list1);
	}
	print_stuff("tailq Start",head3);
	head3.remove(e);
	print_stuff("tailq removing 3",head3);
	head3.remove_head();
	print_stuff("tailq  remove_head",head3);
	while(1);

}
#include <string.h>
extern "C" int pk_uartputc(int c) {
	uint8_t data = c;
	dbg_uart.write(&data, 1);
	return c & 0xFF;
}
void test_term(const char* str) {
	dbg_uart.write(str,strlen(str));

}
extern "C" void scmrtos_test_start();
extern "C" void try_list() {
	//dbg_uart.open(os::open_mode::write,nullptr);
	dbg_uart.debug_enable();
	test_term("TERM TEST!\r\n");
	printk_setup(pk_uartputc, NULL, SERIAL_OPTIONS);
	scmrtos_test_start();
	//hash_uinttest();
	//tailq_unittest();


	test_entry* e=nullptr;
	for(auto &a : stuff) {
		head1.insert_head(&a);
		if(a.value == 3) e = &a;
	//	slist_insert_head(head1,&a,&test_entry::list1);
	}
	print_stuff("Start",head1);
	head1.remove(e);
	print_stuff("removing 3",head1);
	head1.remove_head();
	print_stuff("remove_head",head1);

	printk("done!\r\n");
	while(1){
	//	printk("tick!\r\n");
	}
}

