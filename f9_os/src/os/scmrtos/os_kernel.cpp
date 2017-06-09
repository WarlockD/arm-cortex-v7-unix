//******************************************************************************
//*
//*     FULLNAME:  Single-Chip Microcontroller Real-Time Operating System
//*
//*     NICKNAME:  scmRTOS
//*
//*     PURPOSE:  OS Kernel Source
//*
//*     Version: v5.1.0
//*
//*
//*     Copyright (c) 2003-2016, scmRTOS Team
//*
//*     Permission is hereby granted, free of charge, to any person 
//*     obtaining  a copy of this software and associated documentation 
//*     files (the "Software"), to deal in the Software without restriction, 
//*     including without limitation the rights to use, copy, modify, merge, 
//*     publish, distribute, sublicense, and/or sell copies of the Software, 
//*     and to permit persons to whom the Software is furnished to do so, 
//*     subject to the following conditions:
//*
//*     The above copyright notice and this permission notice shall be included 
//*     in all copies or substantial portions of the Software.
//*
//*     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
//*     EXPRESS  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
//*     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
//*     IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
//*     CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
//*     TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH 
//*     THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//*
//*     =================================================================
//*     Project sources: https://github.com/scmrtos/scmrtos
//*     Documentation:   https://github.com/scmrtos/scmrtos/wiki/Documentation
//*     Wiki:            https://github.com/scmrtos/scmrtos/wiki
//*     Sample projects: https://github.com/scmrtos/scmrtos-sample-projects
//*     =================================================================
//*
//******************************************************************************


#include <scm\scmRTOS.h>
#include <scm\os_kernel.h>

OS::TKernel OS::Kernel;
__attribute__((weak)) void OS::system_timer_user_hook(){}
__attribute__((weak)) void OS::context_switch_user_hook(){}
__attribute__((weak)) void OS::idle_process_user_hook(){}
__attribute__((weak)) void OS::idle_process_target_hook(){}
using namespace OS;







//TBaseProcess * TKernel::ProcessTable[scmRTOS_PROCESS_COUNT + 1];

//------------------------------------------------------------------------------
//
//    TKernel functions
//

//------------------------------------------------------------------------------
void TKernel::sched()
{
    TBaseProcess* NextProc = highest_priority(CurProcPriority);
    if(NextProc != CurProc)
    {
        SchedProc = NextProc;
        raise_context_switch();
        if(__get_IPSR() == 0){ // in thread mode so we got to wait
            do
            {
                __WFI();
            }
            while(NextProc != SchedProc); // until context switch done
        }

    }
}
//------------------------------------------------------------------------------
stack_item_t* os_context_switch_hook(stack_item_t* sp) { return Kernel.context_switch_hook(sp); }
//------------------------------------------------------------------------------

TBaseProcess::TBaseProcess( stack_item_t * StackPoolEnd
                          , int pr
                          , void (*exec)()
                          , stack_item_t * aStackPool
                          , const char   * name_str
                          ) : Timeout(0)
                            , Priority(pr)
                      #if scmRTOS_DEBUG_ENABLE == 1
                            , WaitingFor(0)
                            , StackPool(aStackPool)
                            , StackSize(StackPoolEnd - aStackPool)
                            , Name(name_str)
                      #endif 
                      #if scmRTOS_PROCESS_RESTART_ENABLE == 1
                            , WaitingProcessMap(0)
                      #endif

{
    TKernel::register_process(this);
    init_stack_frame( StackPoolEnd
                    , exec
              //  #if scmRTOS_DEBUG_ENABLE == 1
                    , aStackPool
            //    #endif
                    );
}
void TKernel::wait(void* chan, timeout_t timeout){
	auto current = CurProc;
    if(current->State != ProcessState::Waiting) {
        TCritSect cs;
        if(current->State != ProcessState::Waiting) {
        	Kernel.set_process_unready(const_cast<TBaseProcess*>(current));
        	current->WaitingFor = chan;
        	current->Timeout = timeout;
			Kernel.scheduler();
        }
    }
}
void TKernel::sleep(timeout_t timeout){ wait(nullptr,timeout); }

void TKernel::wakeup(void* chan){
	// make this a hash lookup latter
    TCritSect cs;
    for(auto it = waiting_queue.begin(); it != waiting_queue.end();) {
    	if(it->WaitingFor == chan) {
    		TBaseProcess* p = &(*it);
    		it = ready_queue.erase(it);
    		ready_queue.push_back(p);
        	p->State = ProcessState::Runable;
        	p->Timeout = 0;
    	} else ++it;
    }
}


//------------------------------------------------------------------------------
//
//
//   Idle Process
//
//
namespace OS
{
    TIdleProc IdleProc("Idle");
}

namespace OS
{
	__attribute((weak)) void idle_process_user_hook() {}
	__attribute((weak)) void idle_process_target_hook() {}
    template<> void TIdleProc::exec()
    {
        for(;;)
        {
        	OS::idle_process_user_hook();
        	OS::idle_process_target_hook();
            __WFI();
        }
    }
}
//------------------------------------------------------------------------------

size_t TBaseProcess::stack_slack() const
{
     size_t slack = 0;
     const stack_item_t * Stack = StackPool;
     while (*Stack++ == 0x43214321)
         slack++;
     return slack;
}
stack_item_t* TKernel::context_switch_hook(stack_item_t* sp)
{
    // no need for crit, PendSV is set at the highest priority
	CurProc->StackPointer = sp;
	sp = SchedProc->StackPointer;
    context_switch_user_hook();
    CurProc = SchedProc;
    return sp;
}

//------------------------------------------------------------------------------
#if scmRTOS_PROCESS_RESTART_ENABLE == 1
void TBaseProcess::reset_controls()
{
    Kernel.set_process_unready(this->Priority);
    if(WaitingProcessMap)
    {
        clr_prio_tag( *WaitingProcessMap, get_prio_tag(Priority) );  // remove current process from service's process map
        WaitingProcessMap = 0;
    }
    Timeout    = 0;
#if scmRTOS_DEBUG_ENABLE == 1
    WaitingFor = 0;
#endif
}
#endif  // scmRTOS_PROCESS_RESTART_ENABLE
//------------------------------------------------------------------------------





