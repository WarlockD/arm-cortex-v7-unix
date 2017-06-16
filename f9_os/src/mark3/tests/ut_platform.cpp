/*===========================================================================
     _____        _____        _____        _____
 ___|    _|__  __|_    |__  __|__   |__  __| __  |__  ______
|    \  /  | ||    \      ||     |     ||  |/ /     ||___   |
|     \/   | ||     \     ||     \     ||     \     ||___   |
|__/\__/|__|_||__|\__\  __||__|\__\  __||__|\__\  __||______|
    |_____|      |_____|      |_____|      |_____|

--[Mark3 Realtime Platform]--------------------------------------------------

Copyright (c) 2012-2016 Funkenstein Software Consulting, all rights reserved.
See license.txt for more information
===========================================================================*/

//---------------------------------------------------------------------------

#include "mark3.h"
#include <os\printk.hpp>

#include "unit_test.h"
#include "ut_platform.h"
#include "memutil.h"



extern "C"
{
//void __cxa_pure_virtual(void) {}
}

//---------------------------------------------------------------------------
// Global objects
static Thread AppThread;			//!< Main "application" thread
static K_WORD aucAppStack[STACK_SIZE_APP];

static Driver* debug_out = nullptr;



//---------------------------------------------------------------------------
#if !KERNEL_USE_IDLE_FUNC
static Thread IdleThread;			//!< Idle thread - runs when app can't
static uint8_t aucIdleStack[STACK_SIZE_IDLE];
#endif

//---------------------------------------------------------------------------
//static uint8_t aucTxBuffer[UART_SIZE_TX];
//static uint8_t aucRxBuffer[UART_SIZE_RX];

//---------------------------------------------------------------------------
static void AppEntry(void);
static void IdleEntry(void);

//---------------------------------------------------------------------------
void MyUnitTest::PrintTestResult()
{
    char acTemp[6];
    int iLen;

    PrintString("Test ");
    PrintString(GetName());
    PrintString(": ");
    iLen = MemUtil::StringLength(GetName());
    if (iLen >= 32)
    {
        iLen = 32;
    }
    for (int i = 0; i < 32 - iLen; i++)
    {
        PrintString(".");
    }
    if (GetPassed() == GetTotal())
    {
        PrintString("(PASS)[");
    }
    else
    {
        PrintString("(FAIL)[");
    }
    MemUtil::DecimalToString(GetPassed(), (char*)acTemp);
    PrintString((const char*)acTemp);
    PrintString("/");
    MemUtil::DecimalToString(GetTotal(), (char*)acTemp);
    PrintString((const char*)acTemp);
    PrintString("]\n");
}

typedef void (*FuncPtr)(void);
//---------------------------------------------------------------------------
void run_tests()
{
    MyTestCase *pstTestCase;
    pstTestCase = astTestCases;

    while (pstTestCase->pclTestCase)
    {
        pstTestCase->pfTestFunc();
        pstTestCase++;
    }
    PrintString("--DONE--\n");
    Thread::Sleep(100);

    FuncPtr pfReset = 0;
    pfReset();
}

//---------------------------------------------------------------------------
void init_tests()
{
    MyTestCase *pstTestCase;
    pstTestCase = astTestCases;

    while (pstTestCase->pclTestCase)
    {
        pstTestCase->pclTestCase->SetName(pstTestCase->szName);
        pstTestCase++;
    }
}

//---------------------------------------------------------------------------
void PrintString(const char *szStr_)
{
    char *szTemp = (char*)szStr_;
    while (*szTemp)
    {
        while( 1 != debug_out->Write( 1, (uint8_t*)szTemp ) ) { /* Do nothing */ }
        szTemp++;
    }
}

//---------------------------------------------------------------------------
void AppEntry(void)
{
    {
    	debug_out = DriverList::FindByPath("/dev/tty");
    	assert(debug_out);

       // my_uart->Control( CMD_SET_BUFFERS, aucRxBuffer, UART_SIZE_RX, aucTxBuffer, UART_SIZE_TX);
    	debug_out->Open();

        init_tests();
    }

    while(1)
    {
        run_tests();
    }
}

//---------------------------------------------------------------------------
void IdleEntry(void)
{
#if !KERNEL_USE_IDLE_FUNC
    while(1)
    {
#endif

#if !KERNEL_USE_IDLE_FUNC
    }
#endif

}

//---------------------------------------------------------------------------
int ut_main(void)
{
    AppThread.Init(	aucAppStack,		//!< Pointer to the stack
                    STACK_SIZE_APP,		//!< Size of the stack
                    1,					//!< Thread priority
                    (ThreadEntry_t)AppEntry,	//!< Entry function
                    (void*)&AppThread );//!< Entry function argument

    AppThread.Start();					//!< Schedule the threads

#if KERNEL_USE_IDLE_FUNC
    Kernel::SetIdleFunc(IdleEntry);
#else
    IdleThread.Init( (uint32_t*)aucIdleStack,		//!< Pointer to the stack
                     STACK_SIZE_IDLE,	//!< Size of the stack
                     0,					//!< Thread priority
                     (ThreadEntry_t)IdleEntry	//!< Entry function
                     );			//!< Entry function argument

    IdleThread.Start();
#endif



    Kernel::Start();					//!< Start the kernel!
}
