//******************************************************************************
//*
//*     FULLNAME:  Single-Chip Microcontroller Real-Time Operating System
//*
//*     NICKNAME:  scmRTOS
//*
//*     PURPOSE:  OS Kernel Header. Declarations And Definitions
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
//*****************************************************************************

#ifndef OS_KERNEL_H
#define OS_KERNEL_H

#include <stddef.h>
#include <stdint.h>
#include <scm\usrlib.h>
#include <stm32f7xx.h>
#include <os\tailq.hpp>
#include <os\hash.hpp>
#include "scmRTOS_defs.h"

//------------------------------------------------------------------------------

//==============================================================================
extern "C" __attribute__((no_return)) void os_start(uint32_t* sp);
extern "C" void os_context_switcher(uint32_t** Curr_SP, uint32_t* Next_SP);

    
//==============================================================================

//------------------------------------------------------------------------------
//
//
//     NAME       :   OS
//
//     PURPOSE    :   Namespace for all OS stuff
//
//     DESCRIPTION:   Includes:  Kernel,
//                               Processes,
//                               Mutexes,
//                               Event Flags,
//                               Arbitrary-type Channels,
//                               Messages
//
namespace OS
{

    class TBaseProcess;
    
    enum TProcessStartState
    {
        pssRunning,
        pssSuspended
    };
    using TProcessNumber = uint32_t;
    typedef uint_fast8_t TPriority;
  //  using TPriority = uint_fast8_t;
    // moved the deffs here just to make this a single file
	class TCritSect
	{
	public:
		TCritSect () : StatusReg(__get_PRIMASK()) { __disable_irq(); }
		~TCritSect() { __set_PRIMASK(StatusReg); }

	private:
		uint32_t StatusReg;
	};
#ifndef INLINE
#define  INLINE  __attribute__((__always_inline__)) inline
#endif

	__attribute__((__always_inline__)) inline void raise_context_switch() { *((volatile uint32_t*)0xE000ED04) |= 0x10000000; }

    class TKernelAgent;
    class TService;
    enum class ProcessState {
    	Zombie,
		Runable,
		Waiting,
    };
    class TBaseProcess
    {
        friend class TKernel;
        friend class TISRW;
        friend class TISRW_SS;
        friend class TKernelAgent;

        friend void run();

    public:
        TBaseProcess( stack_item_t * StackPoolEnd
                    , int pr
                    , void (*exec)()
                    , stack_item_t * aStackPool
                    , const char   * name = 0
                    );

    protected:
        void init_stack_frame( stack_item_t * StackPoolEnd
                             , void (*exec)() 
                             , stack_item_t * StackPool
                             );
    public:
        
        TPriority   priority() const { return Priority; }
        INLINE bool is_sleeping() const;
        INLINE bool is_suspended() const;

        INLINE void * waiting_for() const { return WaitingFor; }
               size_t       stack_size()  const { return StackSize; }
               size_t       stack_slack() const; 
               const char * name()        const { return Name; }
    #endif // scmRTOS_DEBUG_ENABLE

    
    public:

        //-----------------------------------------------------
        //
        //    Data members
        //
        TProcessNumber pid() const { return   ProcessNumber; }
    protected:


        stack_item_t *     StackPointer;
        TProcessNumber	   ProcessNumber;
        volatile timeout_t Timeout;
        const TPriority    Priority;
        ProcessState State;
        void           * volatile 	WaitingFor;
        const stack_item_t * const    StackPool;
        const size_t                  StackSize; // as number of stack_item_t items
        const char                  * Name;
        tailq::entry<TBaseProcess> allproc_link;
        tailq::entry<TBaseProcess> ready_link;
    public:
        using TProcessMap = tailq::head<TBaseProcess,&TBaseProcess::ready_link> ;
    protected:
        TProcessMap * ProcessMap;
        void map_insert(TProcessMap& map) {
        	if(ProcessMap!= nullptr) ProcessMap->remove(this);
        	ProcessMap = &map;
        	ProcessMap->push_front(this);
        }
        void map_remove() {
        	if(ProcessMap!= nullptr) {
        		ProcessMap->remove(this);
        		ProcessMap = nullptr;
        	}
        }
    };
    using TProcessMap = TBaseProcess::TProcessMap;
	/*
	 * System timer interrupt handler.
	 */
	void  system_timer_user_hook();
	void context_switch_user_hook();
	void idle_process_user_hook();
	void idle_process_target_hook();
    class TKernel
    {
        friend class TISRW;
        friend class TISRW_SS;
        friend class TBaseProcess;
        friend class TKernelAgent;
        friend class TService;
        friend void                 run();
        friend bool                 os_running();
        friend const TBaseProcess * get_proc(uint_fast8_t Prio);
        friend inline tick_count_t  get_tick_count();

    private:
        uint_fast8_t           CurProcPriority;
        volatile TBaseProcess* CurProc;
        volatile TBaseProcess* SchedProc;
     //   volatile TProcessMap  ReadyProcessMap;
        volatile uint_fast8_t ISR_NestCount;

    private:
		struct TBaseProcess_hasher  {
			size_t operator()(TProcessNumber x) const { return hash::int_hasher<TProcessNumber>()(x); }
			size_t operator()(const TBaseProcess& x) const { return operator()(x.pid()); }
		};

		struct TBaseProcess_equals  {
			bool operator()(const TBaseProcess& a, const TBaseProcess& b) const { return a.pid() == b.pid(); }
			bool operator()(const TBaseProcess& a, TProcessNumber b) const { return a.pid() == b; }
		};
		using allproc_queue_t = tailq::head<TBaseProcess,&TBaseProcess::ready_link>;
      //  hash::table<TBaseProcess,allproc_queue_t, 64, TBaseProcess_hasher,TBaseProcess_equals>

		allproc_queue_t allproc;
        TProcessMap ready_queue;
        TProcessMap waiting_queue;

       // static TBaseProcess*  ProcessTable[PROCESS_COUNT];
        volatile uint_fast8_t SchedProcPriority;
        volatile tick_count_t SysTickCount;

    //-----------------------------------------------------------
    //
    //      Functions
    //
    	__attribute__((__always_inline__)) inline TBaseProcess* highest_priority(TProcessMap& map, size_t prio=0) {
			TBaseProcess* ret = nullptr;
    		for(auto& p : map)
    			if(p.priority() > prio) {
    				ret = &(p);
    				prio = p.priority();
    			}
    		return ret;
    	}
    	__attribute__((__always_inline__)) inline TBaseProcess* highest_priority(int prio=0) {
    		return highest_priority(ready_queue,prio);
    	}
    public:
        __attribute__((__always_inline__))
    	TKernel() : CurProcPriority( 0 )           // 'MAX_PROCESS_COUNT' means that OS not run yet
					, CurProc(nullptr)
					, SchedProc(nullptr)
             //        , ReadyProcessMap( (1ul << (PROCESS_COUNT)) - 1) // set all processes ready
                     , ISR_NestCount(0)
    {
    }
    
    private:
        INLINE static void register_process(TBaseProcess* const p);

               void sched();
        INLINE void scheduler() { if(ISR_NestCount) return; else  sched(); }
        INLINE void sched_isr();

        INLINE bool is_context_switch_done();
        INLINE void raise_context_switch() { OS::raise_context_switch(); }

        // be sure we have a lock before we run any of the readys
        INLINE void set_process_ready(TBaseProcess* pr) {
        	switch(pr->State){
        	case  ProcessState::Runable:
        		break; // return
        	case ProcessState::Waiting:
        	case ProcessState::Zombie:
        		pr->map_insert(ready_queue);
        		break;
        	default:
        		break; // already ready
        	}
        	pr->State = ProcessState::Runable;
        }
        INLINE void set_process_unready (TBaseProcess* pr,TProcessMap& waiting_map) {
        	switch(pr->State){
        	case  ProcessState::Waiting:
        		break; // return
        	case ProcessState::Runable:
        	case ProcessState::Zombie:
        		pr->map_insert(waiting_map);
        		break;
        	default:
        		break; // already ready
        	}
        	pr->State = ProcessState::Waiting;
        }
        INLINE void set_process_unready (TBaseProcess* pr) {
        	set_process_unready(pr,waiting_queue);
        }
        TBaseProcess* find_process(const TProcessNumber pid) {
        	TBaseProcess* p = nullptr;
        	for(auto& a : allproc){
        		if(a.pid() == pid) {
        			p = &(a);
        			break;
        		}
        	}
        	return p;
        }
        INLINE void set_process_ready (const TProcessNumber pid) {
        	//TBaseProcess* p = allproc.search(pid);
        	TBaseProcess* p = find_process(pid);
        	assert(p);
        	set_process_ready(p);
        }
        INLINE void set_process_unready(const TProcessNumber pid,TProcessMap& waiting_map) {
        	TBaseProcess* p = find_process(pid);
        	assert(p);
        	set_process_unready(p,waiting_map);
        }
        INLINE void set_process_unready(const TProcessNumber pid) {
        	TBaseProcess* p = find_process(pid);
        	assert(p);
        	set_process_unready(p,waiting_queue);
        }
        void wait(void* chan, timeout_t t=0);
        void sleep(timeout_t t=0);
        // carful with null, that will wake up all sleeping procs
        void wakeup(void* chan);
        void wakeup_force(TBaseProcess* pr);
        friend  void OS::wait(void* chan, timeout_t t) ;
        friend  void OS::sleep(timeout_t t) ;
        friend  void OS::wakeup(void* chan);
    public:
        INLINE void system_timer();
        INLINE stack_item_t* context_switch_hook(stack_item_t* sp);
    };  // End of TKernel class definition

    extern TKernel Kernel;

    //--------------------------------------------------------------------------
    //
    //  BaseProcess
    //
    //  Implements base class-type for application processes
    //
    //      DESCRIPTION:
    //
    //

    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    //
    //   process
    // 
    //   Implements template for application processes instantiation
    //
    //      DESCRIPTION:
    //
    //
    #if SEPARATE_RETURN_STACK == 0

        template<TPriority pr, size_t stk_size, TProcessStartState pss = pssRunning>
        class process : public TBaseProcess
        {
        public:
            INLINE process( const char * name_str = 0 );

            static void exec();

        #if scmRTOS_PROCESS_RESTART_ENABLE == 1
            INLINE void terminate();
        #endif
        
        private:
            stack_item_t Stack[stk_size/sizeof(stack_item_t)];
        };

        template<TPriority pr, size_t stk_size, TProcessStartState pss>
        OS::process<pr, stk_size, pss>::process( const char *
            name_str
            ) : TBaseProcess(&Stack[stk_size / sizeof(stack_item_t)]
                             , pr
                             , reinterpret_cast<void (*)()>(exec)
                             , Stack
                             , name_str
                             )
            
        {
            #if scmRTOS_SUSPENDED_PROCESS_ENABLE != 0
            if ( pss == pssSuspended )
                clr_prio_tag(SuspendedProcessMap, get_prio_tag(pr));
            #endif
        }

        #if scmRTOS_PROCESS_RESTART_ENABLE == 1
        template<TPriority pr, size_t stk_size, TProcessStartState pss>
        void OS::process<pr, stk_size, pss>::terminate()
        {
            TCritSect cs;

            reset_controls();
            init_stack_frame( &Stack[stk_size/sizeof(stack_item_t)]
                             , reinterpret_cast<void (*)()>(exec)
                          #if scmRTOS_DEBUG_ENABLE == 1
                              , Stack
                          #endif
                            );
        }
        #endif // scmRTOS_RESTART_ENABLE

        typedef OS::process<127, 256 > TIdleProc;

    #else  // SEPARATE_RETURN_STACK

        template<TPriority pr, size_t stk_size, size_t rstk_size, TProcessStartState pss = pssRunning>
        class process : public TBaseProcess
        {
        public:
            INLINE_PROCESS_CTOR process( const char * name_str = 0 );

            OS_PROCESS static void exec();

        #if scmRTOS_PROCESS_RESTART_ENABLE == 1
            INLINE void terminate();
        #endif

        private:
            stack_item_t Stack [stk_size/sizeof(stack_item_t)];
            stack_item_t RStack[rstk_size/sizeof(stack_item_t)];
        };

        template<TPriority pr, size_t stk_size, size_t rstk_size, TProcessStartState pss>
        process<pr, stk_size, rstk_size, pss>::process( const char *
            #if scmRTOS_DEBUG_ENABLE == 1
            name_str
            #endif
           ): TBaseProcess(&Stack[stk_size / sizeof(stack_item_t)]
                           , &RStack[rstk_size/sizeof(stack_item_t)]
                           , pr
                           , reinterpret_cast<void (*)()>(exec)
                      #if scmRTOS_DEBUG_ENABLE == 1
                           , Stack
                           , RStack
                           , name_str
                      #endif
                           )
        {
            #if scmRTOS_SUSPENDED_PROCESS_ENABLE != 0
            if ( pss == pssSuspended )
                clr_prio_tag(SuspendedProcessMap, get_prio_tag(pr));
            #endif
        }
        
        #if scmRTOS_PROCESS_RESTART_ENABLE == 1
        template<TPriority pr, size_t stk_size, size_t rstk_size, TProcessStartState pss>
        void OS::process<pr, stk_size, rstk_size, pss>::terminate()
        {
            TCritSect cs;
            
            reset_controls();
            init_stack_frame( &Stack[stk_size/sizeof(stack_item_t)]
                            , &RStack[rstk_size/sizeof(stack_item_t)]
                            , reinterpret_cast<void (*)()>(exec)
                        #if scmRTOS_DEBUG_ENABLE == 1
                            , Stack
                            , RStack
                        #endif
                            );
        }
        #endif // scmRTOS_RESTART_ENABLE

        typedef OS::process< OS::prIDLE
                           , scmRTOS_IDLE_PROCESS_DATA_STACK_SIZE
                           , scmRTOS_IDLE_PROCESS_RETURN_STACK_SIZE> TIdleProc;

    #endif    // SEPARATE_RETURN_STACK
    //--------------------------------------------------------------------------


    extern TIdleProc IdleProc;
        
    //--------------------------------------------------------------------------
    //
    //   TKernelAgent
    // 
    //   Grants access to some RTOS kernel internals for services implementation
    //
    //      DESCRIPTION:
    //
    //
    class TKernelAgent
    {
        INLINE static TBaseProcess * cur_proc()                        { return const_cast<TBaseProcess *>(Kernel.CurProc); }

    protected:
        TKernelAgent() { }
        INLINE static void wait(void* chan, timeout_t t=0) { Kernel.wait(chan,t);             }
        INLINE static void sleep(timeout_t t=0) { Kernel.sleep(t);             }
        // carful with null, that will wake up all sleeping procs
        INLINE static void wakeup(void* chan) { Kernel.wakeup(chan);             }
        INLINE static void wakeup_force(TBaseProcess* pr);
        INLINE static uint_fast8_t const   & cur_proc_priority()       { return Kernel.CurProcPriority;  }
       // INLINE static volatile TProcessMap & ready_process_map()       { return Kernel.ReadyProcessMap;  }
        INLINE static volatile timeout_t   & cur_proc_timeout()        { return cur_proc()->Timeout;     }
        INLINE static void reschedule()                                { Kernel.scheduler();             }

        INLINE static void set_process_ready   (const TProcessNumber pr) { Kernel.set_process_ready(pr);   }
        INLINE static void set_process_unready (const TProcessNumber pr) { Kernel.set_process_unready(pr); }
        INLINE static void set_process_ready   (TBaseProcess& pr) { Kernel.set_process_ready(&pr);   }
        INLINE static void set_process_unready (TBaseProcess&  pr) { Kernel.set_process_unready(&pr); }
        INLINE static void * volatile & cur_proc_waiting_for()     { return cur_proc()->WaitingFor;  }

    #if scmRTOS_PROCESS_RESTART_ENABLE == 1
        INLINE static volatile TProcessMap * & cur_proc_waiting_map()  { return cur_proc()->WaitingProcessMap; }
    #endif
    };
    
    //--------------------------------------------------------------------------
    //
    //       Miscellaneous
    //
    //
    INLINE __attribute__((__noreturn__))  void run();
    INLINE bool os_running();
    //INLINE const TBaseProcess * get_proc(uint_fast8_t Prio) { return Kernel.CurProc; }
    INLINE void wait(void* chan, timeout_t t = 0) { Kernel.wait(chan,t); }
    INLINE void sleep(timeout_t t = 0) { Kernel.sleep(t); }
    INLINE void wakeup(void* chan) { Kernel.wakeup(chan); }
    
    //--------------------------------------------------------------------------

    INLINE tick_count_t get_tick_count() { TCritSect cs; return Kernel.SysTickCount; }


	class TISRW
	{
	public:
		__attribute__((__always_inline__)) inline  TISRW()  { ISR_Enter(); }
		__attribute__((__always_inline__)) inline  ~TISRW() { ISR_Exit();  }

	private:
	    //-----------------------------------------------------
		__attribute__((__always_inline__)) inline void ISR_Enter()
	    {
	        TCritSect cs;
	        Kernel.ISR_NestCount++;
	    }
	    //-----------------------------------------------------
		__attribute__((__always_inline__)) inline void ISR_Exit()
	    {
	        TCritSect cs;
	        if(--Kernel.ISR_NestCount) return;
	        Kernel.sched_isr();
	    }
	    //-----------------------------------------------------
	};



	__attribute__((__always_inline__))  void system_timer_isr()
	{
		// OS::TISRW ISR; // no need as its intregrated into Kernel.system_timer

	    Kernel.system_timer();
	}
}

//------------------------------------------------------------------------------

#include "os_services.h"
//#include "scmRTOS_extensions.h"

//------------------------------------------------------------------------------
//
//   Register Process
// 
//   Places pointer to process in kernel's process table
//
void OS::TKernel::register_process(OS::TBaseProcess*  p)
{
	TCritSect crit; // this is critical in case we have hardware screwing around
	Kernel.set_process_ready(p);
}
//------------------------------------------------------------------------------
//
//    System Timer Implementation
// 
//    Performs process's timeouts checking and
//    moving processes to ready-to-run state
//
void OS::TKernel::system_timer()
{
	TCritSect crit; // this is critical in case we have hardware screwing around
    system_timer_user_hook();
    SysTickCount++;
    bool reschedual = false;
    for(auto&p : ready_queue) {
    	if(p.Timeout > 0)
    			if(--p.Timeout == 0) {
    			set_process_ready(&p);
            	reschedual = true;
    		}
    }
    if(reschedual) sched(); // do a rescedual
}

//------------------------------------------------------------------------------
#ifndef CONTEXT_SWITCH_HOOK_CRIT_SECT
#define CONTEXT_SWITCH_HOOK_CRIT_SECT()
#endif



//-----------------------------------------------------------------------------
bool OS::TBaseProcess::is_sleeping() const
{
    TCritSect cs;
    return this->Timeout != 0 && State == ProcessState::Waiting;
}
//-----------------------------------------------------------------------------
bool OS::TBaseProcess::is_suspended() const
{
    TCritSect cs;
    return this->Timeout == 0   && State == ProcessState::Waiting;
}

//-----------------------------------------------------------------------------
INLINE void OS::run()
{
	TBaseProcess* top = Kernel.highest_priority(0);
	Kernel.CurProc = top;
	Kernel.CurProcPriority = top->priority();
	stack_item_t *sp = Kernel.CurProc->StackPointer;
    os_start(sp);
}
//-----------------------------------------------------------------------------
INLINE bool OS::os_running()
{
    return Kernel.CurProc != nullptr;
}
//-----------------------------------------------------------------------------

#include "os_services.h"

#endif // OS_KERNEL_H
