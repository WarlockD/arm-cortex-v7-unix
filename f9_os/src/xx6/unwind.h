#ifndef _UNWIND_H_
#define _UNWIND_H_



// define bewo to enable tracing
#define UPGRADE_ARM_STACK_UNWIND 1
#define UNW_DEBUG

#ifdef UPGRADE_ARM_STACK_UNWIND
#include <stdint.h>
#include <stdbool.h>
/** Example structure for holding unwind results.
 */
/***************************************************************************
 * Manifest Constants
 **************************************************************************/

/** The maximum number of instructions to interpet in a function.
 * Unwinding will be unconditionally stopped and UNWIND_EXHAUSTED returned
 * if more than this number of instructions are interpreted in a single
 * function without unwinding a stack frame.  This prevents infinite loops
 * or corrupted program memory from preventing unwinding from progressing.
 */
#define UNW_MAX_INSTR_COUNT 100

/** The size of the hash used to track reads and writes to memory.
 * This should be a prime value for efficiency.
 */
#define MEM_HASH_SIZE        31

/***************************************************************************
 * Type Definitions
 **************************************************************************/
typedef enum UnwResultTag
{
    /** Unwinding was successful and complete. */
    UNWIND_SUCCESS = 0,

    /** More than UNW_MAX_INSTR_COUNT instructions were interpreted. */
    UNWIND_EXHAUSTED,

    /** Unwinding stopped because the reporting func returned FALSE. */
    UNWIND_TRUNCATED,

    /** Read data was found to be inconsistent. */
    UNWIND_INCONSISTENT,

    /** Unsupported instruction or data found. */
    UNWIND_UNSUPPORTED,

    /** General failure. */
    UNWIND_FAILURE,

    /** Illegal instruction. */
    UNWIND_ILLEGAL_INSTR,

    /** Unwinding hit the reset vector. */
    UNWIND_RESET,

    /** Failed read for an instruction word. */
    UNWIND_IREAD_W_FAIL,

    /** Failed read for an instruction half-word. */
    UNWIND_IREAD_H_FAIL,

    /** Failed read for an instruction byte. */
    UNWIND_IREAD_B_FAIL,

    /** Failed read for a data word. */
    UNWIND_DREAD_W_FAIL,

    /** Failed read for a data half-word. */
    UNWIND_DREAD_H_FAIL,

    /** Failed read for a data byte. */
    UNWIND_DREAD_B_FAIL,

    /** Failed write for a data word. */
    UNWIND_DWRITE_W_FAIL
} UnwResult;

typedef enum
{
    /** Invalid value. */
    REG_VAL_INVALID      = 0x00,
    REG_VAL_FROM_STACK   = 0x01,
    REG_VAL_FROM_MEMORY  = 0x02,
    REG_VAL_FROM_CONST   = 0x04,
    REG_VAL_ARITHMETIC   = 0x80
} RegValOrigin;


/** Type for tracking information about a register.
 * This stores the register value, as well as other data that helps unwinding.
 */
typedef struct
{
    /** The value held in the register. */
    uint32_t              v;

    /** The origin of the register value.
     * This is used to track how the value in the register was loaded.
     */
    RegValOrigin       o;
} RegData;


/** Structure used to track reads and writes to memory.
 * This structure is used as a hash to store a small number of writes
 * to memory.
 */
typedef struct
{
    /** Memory contents. */
	uint32_t              v[MEM_HASH_SIZE];

    /** Address at which v[n] represents. */
	uint32_t              a[MEM_HASH_SIZE];

    /** Indicates whether the data in v[n] and a[n] is occupied.
     * Each bit represents one hash value.
     */
    uint8_t               used[(MEM_HASH_SIZE + 7) / 8];

    /** Indicates whether the data in v[n] is valid.
     * This allows a[n] to be set, but for v[n] to be marked as invalid.
     * Specifically this is needed for when an untracked register value
     * is written to memory.
     */
    uint8_t               tracked[(MEM_HASH_SIZE + 7) / 8];
} MemData;

/** Type for function pointer for result callback.
 * The function is passed two parameters, the first is a void * pointer,
 * and the second is the return address of the function.  The bottom bit
 * of the passed address indicates the execution mode; if it is set,
 * the execution mode at the return address is Thumb, otherwise it is
 * ARM.
 *
 * The return value of this function determines whether unwinding should
 * continue or not.  If TRUE is returned, unwinding will continue and the
 * report function maybe called again in future.  If FALSE is returned,
 * unwinding will stop with UnwindStart() returning UNWIND_TRUNCATED.
 */
typedef bool (*UnwindReportFunc)(void   *data, uint32_t   address);

/** Structure that holds memory callback function pointers.
 */
typedef struct UnwindCallbacksTag
{
    /** Report an unwind result. */
    UnwindReportFunc report;
    /** Read a 32 bit word from memory.
     * The memory address to be read is passed as \a address, and
     * \a *val is expected to be populated with the read value.
     * If the address cannot or should not be read, FALSE can be
     * returned to indicate that unwinding should stop.  If TRUE
     * is returned, \a *val is assumed to be valid and unwinding
     * will continue.
     */
    bool (*readW)(const uint32_t address, uint32_t *val);

    /** Read a 16 bit half-word from memory.
     * This function has the same usage as for readW, but is expected
     * to read only a 16 bit value.
     */
    bool (*readH)(const uint32_t address, uint16_t *val);

    /** Read a byte from memory.
     * This function has the same usage as for readW, but is expected
     * to read only an 8 bit value.
     */
    bool (*readB)(const uint32_t address, uint8_t  *val);

#if defined(UNW_DEBUG)
    /** Print a formatted line for debug. */
    int (*printf)(const char *format, ...);
#endif

}
UnwindCallbacks;

/** Structure that is used to keep track of unwinding meta-data.
 * This data is passed between all the unwinding functions.
 */
typedef struct
{
    /** The register values and meta-data. */
    RegData regData[16];

    /** Memory tracking data. */
    MemData memData;

    /** Pointer to the callback functions */
    const UnwindCallbacks *cb;

    /** Pointer to pass to the report function. */
    const void *reportData;
}
UnwState;

/***************************************************************************
 *  Macros
 **************************************************************************/

#define M_IsOriginValid(v) (((v) & 0x7f) ? true : false)
#define M_Origin2Str(v)    ((v) ? "VALID" : "INVALID")


#if defined(UNW_DEBUG)
#define UnwPrintd1(a)               state->cb->printf(a)
#define UnwPrintd2(a,b)             state->cb->printf(a,b)
#define UnwPrintd3(a,b,c)           state->cb->printf(a,b,c)
#define UnwPrintd4(a,b,c,d)         state->cb->printf(a,b,c,d)
#define UnwPrintd5(a,b,c,d,e)       state->cb->printf(a,b,c,d,e)
#define UnwPrintd6(a,b,c,d,e,f)     state->cb->printf(a,b,c,d,e,f)
#define UnwPrintd7(a,b,c,d,e,f,g)   state->cb->printf(a,b,c,d,e,f,g)
#define UnwPrintd8(a,b,c,d,e,f,g,h) state->cb->printf(a,b,c,d,e,f,g,h)
#else
#define UnwPrintd1(a)
#define UnwPrintd2(a,b)
#define UnwPrintd3(a,b,c)
#define UnwPrintd4(a,b,c,d)
#define UnwPrintd5(a,b,c,d,e)
#define UnwPrintd6(a,b,c,d,e,f)
#define UnwPrintd7(a,b,c,d,e,f,g)
#define UnwPrintd8(a,b,c,d,e,f,g,h)
#endif

/***************************************************************************
 *  Function Prototypes
 **************************************************************************/

UnwResult UnwStartArm       (UnwState * const state);

UnwResult UnwStartThumb     (UnwState * const state);

void UnwInvalidateRegisterFile(RegData *regFile);

void UnwInitState           (UnwState * const       state,
                             const UnwindCallbacks *cb,
                             void                  *rptData,
                             uint32_t                  pcValue,
							 uint32_t                  spValue);

bool UnwReportRetAddr    (UnwState * const state, uint32_t addr);

bool UnwMemWriteRegister (UnwState * const      state,
                             const uint32_t           addr,
                             const RegData * const reg);

bool UnwMemReadRegister  (UnwState * const      state,
                             const uint32_t           addr,
                             RegData * const       reg);

void    UnwMemHashGC        (UnwState * const state);
bool UnwMemHashRead  (MemData * const memData,
                         uint32_t           addr,
						 uint32_t   * const data,
						 bool * const tracked);

bool UnwMemHashWrite (MemData * const memData,
						uint32_t           addr,
						uint32_t           val,
                        bool         valValid);

void    UnwMemHashGC    (UnwState * const state);
/***************************************************************************
 * Manifest Constants
 **************************************************************************/

/** \def UNW_DEBUG
 * If this define is set, additional information will be produced while
 * unwinding the stack to allow debug of the unwind module itself.
 */
/* #define UNW_DEBUG 1 */

/***************************************************************************
 * Type Definitions
 **************************************************************************/

/** Possible results for UnwindStart to return.
 */



/***************************************************************************
 *  Macros
 **************************************************************************/

/***************************************************************************
 *  Function Prototypes
 **************************************************************************/

/** Start unwinding the current stack.
 * This will unwind the stack starting at the PC value supplied to in the
 * link register (i.e. not a normal register) and the stack pointer value
 * supplied.
 */
//UnwResult UnwindStart(uint32_t spValue,const UnwindCallbacks *cb, void *data);

UnwResult UnwindStart(
		uint32_t spValue,
		const UnwindCallbacks *cb,
		void *data);
UnwResult UnwindStartRet(
		uint32_t spValue,uint32_t retValue,
		const UnwindCallbacks *cb,
        void                  *data);
/** Example structure for holding unwind results.
 */
typedef struct
{
    uint16_t frameCount;
    uint32_t address[32];
}
CliStack;

/***************************************************************************
 * Variables
 ***************************************************************************/

extern const UnwindCallbacks cliCallbacks;

#define UNWIND()                                                \
{                                                               \
    CliStack  results;                                          \
    Int8      t;                                                \
    UnwResult r;                                                \
    uint32_t current_sp = __asm("sp");                          \
    (results).frameCount = 0;                                   \
    r = UnwindStart(__current_sp(), &cliCallbacks, &results);   \
                                                                \
    for(t = 0; t < (results).frameCount; t++)                   \
    {                                                           \
        printf("%c: 0x%08x\n",                                  \
               (results.address[t] & 0x1) ? 'T' : 'A',          \
               results.address[t] & (~0x1));                    \
    }                                                           \
                                                                \
    printf("\nResult: %d\n", r);                                \
}

#endif


#endif
