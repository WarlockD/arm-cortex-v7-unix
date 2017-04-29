/*
 * os.cpp
 *
 *  Created on: Apr 18, 2017
 *      Author: warlo
 */

#include "os.h"

#include <os\printk.h>

namespace xv6 {
#if 0
enum class irq_prio {
	Critical = 0,
	Clock = 1,
	Tty = 2,
	Bio = 3,
	Normal = 15,
};
#endif
static const char* irq_proi_string[] = {
	//	[(int)irq_prio::Critical]  "Critical",

};
// sleep wakeup fuctions, fake ones are in the os.cpp
 void wakeup(void *p){
	 printk("Woke up pointer: %08X\n", (long)p);
 }
 void sleep(void* p, irq_prio pri){
	 printk("Sleeping  pointer: %08X, pri = %s\n", (long)p, irq_proi_string[(int)pri]);
	 assert(0); // have to die cause sleep not working yet
 }


}; /* namespace xv6 */
