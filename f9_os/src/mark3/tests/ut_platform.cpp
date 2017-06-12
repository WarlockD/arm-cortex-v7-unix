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

class trace_driver : public Driver {
public:
    void Init() { putsk("trace_driver: Init\r\n"); }
    uint8_t Open()  { putsk("trace_driver: Open\r\n"); return 0; }
    uint8_t Close()	{ putsk("trace_driver: Close\r\n"); return 0; }
    virtual ~trace_driver() {}
    uint16_t Read( uint16_t u16Bytes_, uint8_t *pu8Data_) {
    	(void)u16Bytes_;(void)pu8Data_;
    	//auto sz = trace_write(pu8Data_, u16Bytes_);
    	return 0;
    }
    uint16_t Write( uint16_t u16Bytes_, uint8_t *pu8Data_){
    	writek(pu8Data_, u16Bytes_);
    	return u16Bytes_;
    }
    virtual uint16_t Control( uint16_t u16Event_,
                                    void *pvDataIn_,
                                    uint16_t u16SizeIn_,
                                    void *pvDataOut_,
                                    uint16_t u16SizeOut_ ) {
    	// does NOTHING
    	(void)u16Event_;(void)pvDataIn_;(void)u16SizeIn_;(void)pvDataOut_;(void)u16SizeOut_;
    	return 0;
    }

};
static trace_driver clUART; 	//!< UART device driver object


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
        while( 1 != clUART.Write( 1, (uint8_t*)szTemp ) ) { /* Do nothing */ }
        szTemp++;
    }
}

//---------------------------------------------------------------------------
void AppEntry(void)
{
    {
        Driver *my_uart = DriverList::FindByPath("/dev/tty");

       // my_uart->Control( CMD_SET_BUFFERS, aucRxBuffer, UART_SIZE_RX, aucTxBuffer, UART_SIZE_TX);
        my_uart->Open();

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
    Kernel::Init();						//!< MUST be before other kernel ops

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

    clUART.SetName("/dev/tty");			//!< Add the serial driver
    clUART.Init();

    DriverList::Add( &clUART );

    Kernel::Start();					//!< Start the kernel!
}
