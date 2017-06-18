#include <assert.h>
#include <stm32746g_discovery.h>
#include <stm32746g_discovery_qspi.h>
#include <stm32746g_discovery_sdram.h>
#include <pin_macros.h>
#include <string.h>
#include <stdio.h>

#include <os\printk.h>
#include <sys\time.h>

#include "f9\thread.hpp"

#include <os\printk.hpp>
#if 0
#include "scm\scmRTOS.h"
#include "scm\os_serial.hpp"
#endif

#include "kerneltypes.h"
#include "kernel.h"
#include "thread.h"
#include "ksemaphore.h"
#include "kernelprofile.h"
#include "profile.h"
#include "kerneltimer.h"
#include "driver.h"
#include "memutil.h"
#include "tracebuffer.h"

#include "mark3/tests/ut_platform.h"
#include "mark3/tests/ut_thread.cpp"

//------------------------------------
int ut_main(void);

void LoggerCallback(uint16_t *pu16Data_, uint16_t u16Len_, bool bPingPong_)
{
	(void)bPingPong_;
    CS_ENTER();
    writek( (uint8_t*)pu16Data_,u16Len_);
    CS_EXIT();
}

extern "C" void scmrtos_test_start()
{
	printk("starting os!\n");
    TraceBuffer::SetCallback( LoggerCallback );
	ut_main();
}


