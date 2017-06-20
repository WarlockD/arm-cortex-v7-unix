/***************************************************************************
 * ARM Stack Unwinder, Michael.McTernan.2001@cs.bris.ac.uk
 *
 * This program is PUBLIC DOMAIN.
 * This means that there is no copyright and anyone is able to take a copy
 * for free and use it as they wish, with or without modifications, and in
 * any context, commercially or otherwise. The only limitation is that I
 * don't guarantee that the software is fit for any purpose or accept any
 * liability for it's use or misuse - this software is without warranty.
 ***************************************************************************
 * File Description: Abstract interpretation for Thumb mode.
 **************************************************************************/

#define MODULE_NAME "UNWARM_THUMB"

/***************************************************************************
 * Include Files
 **************************************************************************/

#include "unwind.h"
#if defined(UPGRADE_ARM_STACK_UNWIND)
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#define M_IsIdxUsed(a, v) (((a)[v >> 3] & (1 << (v & 0x7))) ? true : false)

#define M_SetIdxUsed(a, v) ((a)[v >> 3] |= (1 << (v & 0x7)))

#define M_ClrIdxUsed(a, v) ((a)[v >> 3] &= ~(1 << (v & 0x7)))

/***************************************************************************
 * Local Functions
 **************************************************************************/

/** Search the memory hash to see if an entry is stored in the hash already.
 * This will search the hash and either return the index where the item is
 * stored, or -1 if the item was not found.
 */
static int16_t memHashIndex(MemData * const memData,
                                const uint32_t     addr)
{
    const uint16_t v = addr % MEM_HASH_SIZE;
    uint16_t       s = v;

    do
    {
        /* Check if the element is occupied */
        if(M_IsIdxUsed(memData->used, s))
        {
            /* Check if it is occupied with the sought data */
            if(memData->a[s] == addr)
            {
                return s;
            }
        }
        else
        {
            /* Item is free, this is where the item should be stored */
            return s;
        }

        /* Search the next entry */
        s++;
        if(s > MEM_HASH_SIZE)
        {
            s = 0;
        }
    }
    while(s != v);

    /* Search failed, hash is full and the address not stored */
    return -1;
}



/***************************************************************************
 * Global Functions
 **************************************************************************/

bool UnwMemHashRead(MemData * const memData,
                       uint32_t           addr,
                       uint32_t   * const data,
                       bool * const tracked)
{
    int16_t i = memHashIndex(memData, addr);

    if(i >= 0 && M_IsIdxUsed(memData->used, i) && memData->a[i] == addr)
    {
        *data    = memData->v[i];
        *tracked = M_IsIdxUsed(memData->tracked, i);
        return true;
    }
    else
    {
        /* Address not found in the hash */
        return false;
    }
}

bool UnwMemHashWrite(MemData * const memData,
                        uint32_t           addr,
                        uint32_t           val,
                        bool         valValid)
{
    int16_t i = memHashIndex(memData, addr);

    if(i < 0)
    {
        /* Hash full */
        return false;
    }
    else
    {
        /* Store the item */
        memData->a[i] = addr;
        M_SetIdxUsed(memData->used, i);

        if(valValid)
        {
            memData->v[i] = val;
            M_SetIdxUsed(memData->tracked, i);
        }
        else
        {
#if defined(UNW_DEBUG)
            memData->v[i] = 0xdeadbeef;
#endif
            M_ClrIdxUsed(memData->tracked, i);
        }

        return true;
    }
}


void UnwMemHashGC(UnwState * const state)
{
    const uint32_t minValidAddr = state->regData[13].v;
    MemData * const memData  = &state->memData;
    uint16_t       t;

    for(t = 0; t < MEM_HASH_SIZE; t++)
    {
        if(M_IsIdxUsed(memData->used, t) && (memData->a[t] < minValidAddr))
        {
            UnwPrintd3("MemHashGC: Free elem %d, addr 0x%08x\n",
                       t, memData->a[t]);

            M_ClrIdxUsed(memData->used, t);
        }
    }
}
UnwResult UnwindStart(uint32_t spValue,
                      const UnwindCallbacks *cb,
                      void                  *data){
    uint32_t    retAddr;
    UnwState state;

    retAddr = __return_address(0);
    /* Initialise the unwinding state */
    UnwInitState(&state, cb, data, retAddr, spValue);
#if 1
    return UnwStartThumb(&state);
#else


    /* Check the Thumb bit */
    if(retAddr & 0x1)
    {
        return UnwStartThumb(&state);
    }
    else
    {
       return UnwStartArm(&state);
    }
#endif
}
UnwResult UnwindStartRet(uint32_t spValue,uint32_t retAddr,
                      const UnwindCallbacks *cb,
                      void                  *data)
{
    UnwState state;
#if 0

#if !defined(SIM_CLIENT)
    retAddr = __return_address(0);
#else
    retAddr = 0x0000a894;
   spValue = 0x7ff7edf8;
#endif
#endif
    /* Initialise the unwinding state */
    UnwInitState(&state, cb, data, retAddr, spValue);
#if 1
    return UnwStartThumb(&state);
#else


    /* Check the Thumb bit */
    if(retAddr & 0x1)
    {
        return UnwStartThumb(&state);
    }
    else
    {
       return UnwStartArm(&state);
    }
#endif
}

#if defined(UNW_DEBUG)
/** Printf wrapper.
 * This is used such that alternative outputs for any output can be selected
 * by modification of this wrapper function.
 */
void UnwPrintf(const char *format, ...)
{
    va_list args;

    va_start( args, format );
    vtrace_printf(format, args );
    va_end(args);
}
#endif

/** Invalidate all general purpose registers.
 */
void UnwInvalidateRegisterFile(RegData *regFile)
{
    uint8_t t = 0;

    do
    {
        regFile[t].o = REG_VAL_INVALID;
        t++;
    }
    while(t < 13);

}


/** Initialise the data used for unwinding.
 */
void UnwInitState(UnwState * const       state,   /**< Pointer to structure to fill. */
                  const UnwindCallbacks *cb,      /**< Callbacks. */
                  void                  *rptData, /**< Data to pass to report function. */
                  uint32_t                  pcValue, /**< PC at which to start unwinding. */
                  uint32_t                  spValue) /**< SP at which to start unwinding. */
{
    UnwInvalidateRegisterFile(state->regData);

    /* Store the pointer to the callbacks */
    state->cb = cb;
    state->reportData = rptData;

    /* Setup the SP and PC */
    state->regData[13].v = spValue;
    state->regData[13].o = REG_VAL_FROM_CONST;
    state->regData[15].v = pcValue;
    state->regData[15].o = REG_VAL_FROM_CONST;

    UnwPrintd3("\nInitial: PC=0x%08x SP=0x%08x\n", pcValue, spValue);

    /* Invalidate all memory addresses */
    memset(state->memData.used, 0, sizeof(state->memData.used));
}


/** Call the report function to indicate some return address.
 * This returns the value of the report function, which if true
 * indicates that unwinding may continue.
 */
bool UnwReportRetAddr(UnwState * const state, uint32_t addr)
{
    /* Cast away const from reportData.
     *  The const is only to prevent the unw module modifying the data.
     */
    return state->cb->report((void *)state->reportData, addr);
}


/** Write some register to memory.
 * This will store some register and meta data onto the virtual stack.
 * The address for the write
 * \param state [in/out]  The unwinding state.
 * \param wAddr [in]  The address at which to write the data.
 * \param reg   [in]  The register to store.
 * \return true if the write was successful, false otherwise.
 */
bool UnwMemWriteRegister(UnwState * const      state,
                            const uint32_t           addr,
                            const RegData * const reg)
{
    return UnwMemHashWrite(&state->memData,
                           addr,
                           reg->v,
                           M_IsOriginValid(reg->o));
}

/** Read a register from memory.
 * This will read a register from memory, and setup the meta data.
 * If the register has been previously written to memory using
 * UnwMemWriteRegister, the local hash will be used to return the
 * value while respecting whether the data was valid or not.  If the
 * register was previously written and was invalid at that point,
 * REG_VAL_INVALID will be returned in *reg.
 * \param state [in]  The unwinding state.
 * \param addr  [in]  The address to read.
 * \param reg   [out] The result, containing the data value and the origin
 *                     which will be REG_VAL_FROM_MEMORY, or REG_VAL_INVALID.
 * \return true if the address could be read and *reg has been filled in.
 *         false is the data could not be read.
 */
bool UnwMemReadRegister(UnwState * const      state,
                           const uint32_t           addr,
                           RegData * const       reg)
{
    bool tracked;

    /* Check if the value can be found in the hash */
    if(UnwMemHashRead(&state->memData, addr, &reg->v, &tracked))
    {
        reg->o = tracked ? REG_VAL_FROM_MEMORY : REG_VAL_INVALID;
        return true;
    }
    /* Not in the hash, so read from real memory */
    else if(state->cb->readW(addr, &reg->v))
    {
        reg->o = REG_VAL_FROM_MEMORY;
        return true;
    }
    /* Not in the hash, and failed to read from memory */
    else
    {
        return false;
    }
}






/***************************************************************************
 * Manifest Constants
 **************************************************************************/


/***************************************************************************
 * Type Definitions
 **************************************************************************/


/***************************************************************************
 * Variables
 **************************************************************************/


/***************************************************************************
 * Macros
 **************************************************************************/


/***************************************************************************
 * Local Functions
 **************************************************************************/

/** Sign extend an 11 bit value.
 * This function simply inspects bit 11 of the input \a value, and if
 * set, the top 5 bits are set to give a 2's compliment signed value.
 * \param value   The value to sign extend.
 * \return The signed-11 bit value stored in a 16bit data type.
 */
static int16_t signExtend11(uint16_t value)
{
    if(value & 0x400)
    {
        value |= 0xf800;
    }

    return value;
}


/***************************************************************************
 * Global Functions
 **************************************************************************/


UnwResult UnwStartThumb(UnwState * const state)
{
    bool  found = false;
    uint16_t    t = UNW_MAX_INSTR_COUNT;

    do
    {
        uint16_t instr;

        /* Attempt to read the instruction */
        if(!state->cb->readH(state->regData[15].v & (~0x1), &instr))
        {
            return UNWIND_IREAD_H_FAIL;
        }

        UnwPrintd4("T %x %x %04x:",
                   state->regData[13].v, state->regData[15].v, instr);

        /* Check that the PC is still on Thumb alignment */
        if(!(state->regData[15].v & 0x1))
        {
            UnwPrintd1("\nError: PC misalignment\n");
            return UNWIND_INCONSISTENT;
        }

        /* Check that the SP and PC have not been invalidated */
        if(!M_IsOriginValid(state->regData[13].o) || !M_IsOriginValid(state->regData[15].o))
        {
            UnwPrintd1("\nError: PC or SP invalidated\n");
            return UNWIND_INCONSISTENT;
        }

        /* Format 1: Move shifted register
         *  LSL Rd, Rs, #Offset5
         *  LSR Rd, Rs, #Offset5
         *  ASR Rd, Rs, #Offset5
         */
        if((instr & 0xe000) == 0x0000 && (instr & 0x1800) != 0x1800)
        {
            bool signExtend;
            uint8_t    op      = (instr & 0x1800) >> 11;
            uint8_t    offset5 = (instr & 0x07c0) >>  6;
            uint8_t    rs      = (instr & 0x0038) >>  3;
            uint8_t    rd      = (instr & 0x0007);

            switch(op)
            {
                case 0: /* LSL */
                    UnwPrintd6("LSL r%d, r%d, #%d\t; r%d %s", rd, rs, offset5, rs, M_Origin2Str(state->regData[rs].o));
                    state->regData[rd].v = state->regData[rs].v << offset5;
                    state->regData[rd].o = state->regData[rs].o;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    break;

                case 1: /* LSR */
                    UnwPrintd6("LSR r%d, r%d, #%d\t; r%d %s", rd, rs, offset5, rs, M_Origin2Str(state->regData[rs].o));
                    state->regData[rd].v = state->regData[rs].v >> offset5;
                    state->regData[rd].o = state->regData[rs].o;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    break;

                case 2: /* ASR */
                    UnwPrintd6("ASL r%d, r%d, #%d\t; r%d %s", rd, rs, offset5, rs, M_Origin2Str(state->regData[rs].o));

                    signExtend = (state->regData[rs].v & 0x8000) ? true : false;
                    state->regData[rd].v = state->regData[rs].v >> offset5;
                    if(signExtend)
                    {
                        state->regData[rd].v |= 0xffffffff << (32 - offset5);
                    }
                    state->regData[rd].o = state->regData[rs].o;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    break;
            }
        }
        /* Format 2: add/subtract
         *  ADD Rd, Rs, Rn
         *  ADD Rd, Rs, #Offset3
         *  SUB Rd, Rs, Rn
         *  SUB Rd, Rs, #Offset3
         */
        else if((instr & 0xf800) == 0x1800)
        {
            bool I  = (instr & 0x0400) ? true : false;
            bool op = (instr & 0x0200) ? true : false;
            uint8_t    rn = (instr & 0x01c0) >> 6;
            uint8_t    rs = (instr & 0x0038) >> 3;
            uint8_t    rd = (instr & 0x0007);

            /* Print decoding */
            UnwPrintd6("%s r%d, r%d, %c%d\t;",
                       op ? "SUB" : "ADD",
                       rd, rs,
                       I ? '#' : 'r',
                       rn);
            UnwPrintd5("r%d %s, r%d %s",
                       rd, M_Origin2Str(state->regData[rd].o),
                       rs, M_Origin2Str(state->regData[rs].o));
            if(!I)
            {
                UnwPrintd3(", r%d %s", rn, M_Origin2Str(state->regData[rn].o));

                /* Perform calculation */
                if(op)
                {
                    state->regData[rd].v = state->regData[rs].v - state->regData[rn].v;
                }
                else
                {
                    state->regData[rd].v = state->regData[rs].v + state->regData[rn].v;
                }

                /* Propagate the origin */
                if(M_IsOriginValid(state->regData[rs].v) &&
                   M_IsOriginValid(state->regData[rn].v))
                {
                    state->regData[rd].o = state->regData[rs].o;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                }
                else
                {
                    state->regData[rd].o = REG_VAL_INVALID;
                }
            }
            else
            {
                /* Perform calculation */
                if(op)
                {
                    state->regData[rd].v = state->regData[rs].v - rn;
                }
                else
                {
                    state->regData[rd].v = state->regData[rs].v + rn;
                }

                /* Propagate the origin */
                state->regData[rd].o = state->regData[rs].o;
                state->regData[rd].o |= REG_VAL_ARITHMETIC;
            }
        }
        /* Format 3: move/compare/add/subtract immediate
         *  MOV Rd, #Offset8
         *  CMP Rd, #Offset8
         *  ADD Rd, #Offset8
         *  SUB Rd, #Offset8
         */
        else if((instr & 0xe000) == 0x2000)
        {
            uint8_t    op      = (instr & 0x1800) >> 11;
            uint8_t    rd      = (instr & 0x0700) >>  8;
            uint8_t    offset8 = (instr & 0x00ff);

            switch(op)
            {
                case 0: /* MOV */
                    UnwPrintd3("MOV r%d, #0x%x", rd, offset8);
                    state->regData[rd].v = offset8;
                    state->regData[rd].o = REG_VAL_FROM_CONST;
                    break;

                case 1: /* CMP */
                    /* Irrelevant to unwinding */
                    UnwPrintd1("CMP ???");
                    break;

                case 2: /* ADD */
                    UnwPrintd5("ADD r%d, #0x%x\t; r%d %s",
                               rd, offset8, rd, M_Origin2Str(state->regData[rd].o));
                    state->regData[rd].v += offset8;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    break;

                case 3: /* SUB */
                    UnwPrintd5("SUB r%d, #0x%d\t; r%d %s",
                               rd, offset8, rd, M_Origin2Str(state->regData[rd].o));
                    state->regData[rd].v += offset8;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    break;
            }
        }
        /* Format 4: ALU operations
         *  AND Rd, Rs
         *  EOR Rd, Rs
         *  LSL Rd, Rs
         *  LSR Rd, Rs
         *  ASR Rd, Rs
         *  ADC Rd, Rs
         *  SBC Rd, Rs
         *  ROR Rd, Rs
         *  TST Rd, Rs
         *  NEG Rd, Rs
         *  CMP Rd, Rs
         *  CMN Rd, Rs
         *  ORR Rd, Rs
         *  MUL Rd, Rs
         *  BIC Rd, Rs
         *  MVN Rd, Rs
         */
        else if((instr & 0xfc00) == 0x4000)
        {
            uint8_t op = (instr & 0x03c0) >> 6;
            uint8_t rs = (instr & 0x0038) >> 3;
            uint8_t rd = (instr & 0x0007);
#if defined(UNW_DEBUG)
            static const char * const mnu[16] =
            { "AND", "EOR", "LSL", "LSR",
              "ASR", "ADC", "SBC", "ROR",
              "TST", "NEG", "CMP", "CMN",
              "ORR", "MUL", "BIC", "MVN" };
#endif
            /* Print the mnemonic and registers */
            switch(op)
            {
                case 0: /* AND */
                case 1: /* EOR */
                case 2: /* LSL */
                case 3: /* LSR */
                case 4: /* ASR */
                case 7: /* ROR */
                case 9: /* NEG */
                case 12: /* ORR */
                case 13: /* MUL */
                case 15: /* MVN */
                    UnwPrintd8("%s r%d ,r%d\t; r%d %s, r%d %s",
                               mnu[op],
                               rd, rs,
                               rd, M_Origin2Str(state->regData[rd].o),
                               rs, M_Origin2Str(state->regData[rs].o));
                    break;

                case 5: /* ADC */
                case 6: /* SBC */
                    UnwPrintd4("%s r%d, r%d", mnu[op], rd, rs);
                    break;

                case 8: /* TST */
                case 10: /* CMP */
                case 11: /* CMN */
                    /* Irrelevant to unwinding */
                    UnwPrintd2("%s ???", mnu[op]);
                    break;

                case 14: /* BIC */
                    UnwPrintd5("r%d ,r%d\t; r%d %s",
                                rd, rs,
                                rs, M_Origin2Str(state->regData[rs].o));
                    state->regData[rd].v &= !state->regData[rs].v;
                    break;
            }


            /* Perform operation */
            switch(op)
            {
                case 0: /* AND */
                    state->regData[rd].v &= state->regData[rs].v;
                    break;

                case 1: /* EOR */
                    state->regData[rd].v ^= state->regData[rs].v;
                    break;

                case 2: /* LSL */
                    state->regData[rd].v <<= state->regData[rs].v;
                    break;

                case 3: /* LSR */
                    state->regData[rd].v >>= state->regData[rs].v;
                    break;

                case 4: /* ASR */
                    if(state->regData[rd].v & 0x80000000)
                    {
                        state->regData[rd].v >>= state->regData[rs].v;
                        state->regData[rd].v |= 0xffffffff << (32 - state->regData[rs].v);
                    }
                    else
                    {
                        state->regData[rd].v >>= state->regData[rs].v;
                    }

                    break;

                case 5: /* ADC */
                case 6: /* SBC */
                case 8: /* TST */
                case 10: /* CMP */
                case 11: /* CMN */
                    break;
                case 7: /* ROR */
                    state->regData[rd].v = (state->regData[rd].v >> state->regData[rs].v) |
                                    (state->regData[rd].v << (32 - state->regData[rs].v));
                    break;

                case 9: /* NEG */
                    state->regData[rd].v = -state->regData[rs].v;
                    break;

                case 12: /* ORR */
                    state->regData[rd].v |= state->regData[rs].v;
                    break;

                case 13: /* MUL */
                    state->regData[rd].v *= state->regData[rs].v;
                    break;

                case 14: /* BIC */
                    state->regData[rd].v &= !state->regData[rs].v;
                    break;

                case 15: /* MVN */
                    state->regData[rd].v = !state->regData[rs].v;
                    break;
            }

            /* Propagate data origins */
            switch(op)
            {
                case 0: /* AND */
                case 1: /* EOR */
                case 2: /* LSL */
                case 3: /* LSR */
                case 4: /* ASR */
                case 7: /* ROR */
                case 12: /* ORR */
                case 13: /* MUL */
                case 14: /* BIC */
                    if(M_IsOriginValid(state->regData[rs].o) && M_IsOriginValid(state->regData[rs].o))
                    {
                        state->regData[rd].o = state->regData[rs].o;
                        state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    }
                    else
                    {
                        state->regData[rd].o = REG_VAL_INVALID;
                    }
                    break;

                case 5: /* ADC */
                case 6: /* SBC */
                    /* C-bit not tracked */
                    state->regData[rd].o = REG_VAL_INVALID;
                    break;

                case 8: /* TST */
                case 10: /* CMP */
                case 11: /* CMN */
                    /* Nothing propagated */
                    break;

                case 9: /* NEG */
                case 15: /* MVN */
                    state->regData[rd].o = state->regData[rs].o;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    break;

            }

        }
        /* Format 5: Hi register operations/branch exchange
         *  ADD Rd, Hs
         *  ADD Hd, Rs
         *  ADD Hd, Hs
         */
        else if((instr & 0xfc00) == 0x4400)
        {
            uint8_t    op  = (instr & 0x0300) >> 8;
            bool h1  = (instr & 0x0080) ? true: false;
            bool h2  = (instr & 0x0040) ? true: false;
            uint8_t    rhs = (instr & 0x0038) >> 3;
            uint8_t    rhd = (instr & 0x0007);

            /* Adjust the register numbers */
            if(h2) rhs += 8;
            if(h1) rhd += 8;

            if(op != 3 && !h1 && !h2)
            {
                UnwPrintd1("\nError: h1 or h2 must be set for ADD, CMP or MOV\n");
                return UNWIND_ILLEGAL_INSTR;
            }

            switch(op)
            {
                case 0: /* ADD */
                    UnwPrintd5("ADD r%d, r%d\t; r%d %s",
                               rhd, rhs, rhs, M_Origin2Str(state->regData[rhs].o));
                    state->regData[rhd].v += state->regData[rhs].v;
                    state->regData[rhd].o =  state->regData[rhs].o;
                    state->regData[rhd].o |= REG_VAL_ARITHMETIC;
                    break;

                case 1: /* CMP */
                    /* Irrelevant to unwinding */
                    UnwPrintd1("CMP ???");
                    break;

                case 2: /* MOV */
                    UnwPrintd5("MOV r%d, r%d\t; r%d %s",
                               rhd, rhs, rhd, M_Origin2Str(state->regData[rhs].o));
                    state->regData[rhd].v += state->regData[rhs].v;
                    state->regData[rhd].o  = state->regData[rhd].o;
                    break;

                case 3: /* BX */
                    UnwPrintd4("BX r%d\t; r%d %s\n",
                               rhs, rhs, M_Origin2Str(state->regData[rhs].o));

                    /* Only follow BX if the data was from the stack */
                    if(state->regData[rhs].o == REG_VAL_FROM_STACK)
                    {
                        UnwPrintd2(" Return PC=0x%x\n", state->regData[rhs].v & (~0x1));

                        /* Report the return address, including mode bit */
                        if(!UnwReportRetAddr(state, state->regData[rhs].v))
                        {
                            return UNWIND_TRUNCATED;
                        }

                        /* Update the PC */
                        state->regData[15].v = state->regData[rhs].v;

                        /* Determine the new mode */
                        if(state->regData[rhs].v & 0x1)
                        {
                            /* Branching to THUMB */

                            /* Account for the auto-increment which isn't needed */
                            state->regData[15].v -= 2;
                        }
                        else
                        {
                            /* Branch to ARM */
                        	assert(0);
                        	return 0;
                            return UnwStartArm(state);
                        }
                    }
                    else
                    {
                        UnwPrintd4("\nError: BX to invalid register: r%d = 0x%x (%s)\n",
                                   rhs, state->regData[rhs].o, M_Origin2Str(state->regData[rhs].o));
                        return UNWIND_FAILURE;
                    }
            }
        }
        /* Format 9: PC-relative load
         *  LDR Rd,[PC, #imm]
         */
        else if((instr & 0xf800) == 0x4800)
        {
            uint8_t  rd    = (instr & 0x0700) >> 8;
            uint8_t  word8 = (instr & 0x00ff);
            uint32_t address;

            /* Compute load address, adding a word to account for prefetch */
            address = (state->regData[15].v & (~0x3)) + 4 + (word8 << 2);

            UnwPrintd3("LDR r%d, 0x%08x", rd, address);

            if(!UnwMemReadRegister(state, address, &state->regData[rd]))
            {
                return UNWIND_DREAD_W_FAIL;
            }
        }
        /* Format 13: add offset to Stack Pointer
         *  ADD sp,#+imm
         *  ADD sp,#-imm
         */
        else if((instr & 0xff00) == 0xB000)
        {
            uint8_t value = (instr & 0x7f) * 4;

            /* Check the negative bit */
            if((instr & 0x80) != 0)
            {
                UnwPrintd2("SUB sp,#0x%x", value);
                state->regData[13].v -= value;
            }
            else
            {
                UnwPrintd2("ADD sp,#0x%x", value);
                state->regData[13].v += value;
            }
        }
        /* Format 14: push/pop registers
         *  PUSH {Rlist}
         *  PUSH {Rlist, LR}
         *  POP {Rlist}
         *  POP {Rlist, PC}
         */
        else if((instr & 0xf600) == 0xb400)
        {
            bool  L     = (instr & 0x0800) ? true : false;
            bool  R     = (instr & 0x0100) ? true : false;
            uint8_t     rList = (instr & 0x00ff);

            if(L)
            {
                uint8_t r;

                /* Load from memory: POP */
                UnwPrintd2("POP {Rlist%s}\n", R ? ", PC" : "");

                for(r = 0; r < 8; r++)
                {
                    if(rList & (0x1 << r))
                    {
                        /* Read the word */
                        if(!UnwMemReadRegister(state, state->regData[13].v, &state->regData[r]))
                        {
                            return UNWIND_DREAD_W_FAIL;
                        }

                        /* Alter the origin to be from the stack if it was valid */
                        if(M_IsOriginValid(state->regData[r].o))
                        {
                            state->regData[r].o = REG_VAL_FROM_STACK;
                        }

                        state->regData[13].v += 4;

                        UnwPrintd3("  r%d = 0x%08x\n", r, state->regData[r].v);
                    }
                }

                /* Check if the PC is to be popped */
                if(R)
                {
                    /* Get the return address */
                    if(!UnwMemReadRegister(state, state->regData[13].v, &state->regData[15]))
                    {
                        return UNWIND_DREAD_W_FAIL;
                    }

                    /* Alter the origin to be from the stack if it was valid */
                    if(!M_IsOriginValid(state->regData[15].o))
                    {
                        /* Return address is not valid */
                        UnwPrintd1("PC popped with invalid address\n");
                        return UNWIND_FAILURE;
                    }
                    else
                    {
                        /* The bottom bit should have been set to indicate that
                         *  the caller was from Thumb.  This would allow return
                         *  by BX for interworking APCS.
                         */
                        if((state->regData[15].v & 0x1) == 0)
                        {
                            UnwPrintd2("Warning: Return address not to Thumb: 0x%08x\n",
                                       state->regData[15].v);

                            /* Pop into the PC will not switch mode */
                            return UNWIND_INCONSISTENT;
                        }

                        /* Store the return address */
                        if(!UnwReportRetAddr(state, state->regData[15].v))
                        {
                            return UNWIND_TRUNCATED;
                        }

                        /* Now have the return address */
                        UnwPrintd2(" Return PC=%x\n", state->regData[15].v);

                        /* Update the pc */
                        state->regData[13].v += 4;

                        /* Compensate for the auto-increment, which isn't needed here */
                        state->regData[15].v -= 2;
                    }
                }

            }
            else
            {
                int8_t r;

                /* Store to memory: PUSH */
                UnwPrintd2("PUSH {Rlist%s}", R ? ", LR" : "");

                /* Check if the LR is to be pushed */
                if(R)
                {
                    UnwPrintd3("\n  lr = 0x%08x\t; %s",
                               state->regData[14].v, M_Origin2Str(state->regData[14].o));

                    state->regData[13].v -= 4;

                    /* Write the register value to memory */
                    if(!UnwMemWriteRegister(state, state->regData[13].v, &state->regData[14]))
                    {
                        return UNWIND_DWRITE_W_FAIL;
                    }
                }

                for(r = 7; r >= 0; r--)
                {
                    if(rList & (0x1 << r))
                    {
                        UnwPrintd4("\n  r%d = 0x%08x\t; %s",
                                   r, state->regData[r].v, M_Origin2Str(state->regData[r].o));

                        state->regData[13].v -= 4;

                        if(!UnwMemWriteRegister(state, state->regData[13].v, &state->regData[r]))
                        {
                            return UNWIND_DWRITE_W_FAIL;
                        }
                    }
                }
            }
        }
        /* Format 18: unconditional branch
         *  B label
         */
        else if((instr & 0xf800) == 0xe000)
        {
            int16_t branchValue = signExtend11(instr & 0x07ff);

            /* Branch distance is twice that specified in the instruction. */
            branchValue *= 2;

            UnwPrintd2("B %d \n", branchValue);

            /* Update PC */
            state->regData[15].v += branchValue;

            /* Need to advance by a word to account for pre-fetch.
             *  Advance by a half word here, allowing the normal address
             *  advance to account for the other half word.
             */
            state->regData[15].v += 2;

            /* Display PC of next instruction */
            UnwPrintd2(" New PC=%x", state->regData[15].v + 2);

        }
        else
        {
            UnwPrintd1("????");

            /* Unknown/undecoded.  May alter some register, so invalidate file */
            UnwInvalidateRegisterFile(state->regData);
        }

        UnwPrintd1("\n");

        /* Should never hit the reset vector */
        if(state->regData[15].v == 0) return UNWIND_RESET;

        /* Check next address */
        state->regData[15].v += 2;

        /* Garbage collect the memory hash (used only for the stack) */
        UnwMemHashGC(state);

        t--;
        if(t == 0) return UNWIND_EXHAUSTED;

    }
    while(!found);

    return UNWIND_SUCCESS;
}
// client is here



/***************************************************************************
 * Callbacks
 ***************************************************************************/

/***************************************************************************
 *
 * Function:     CliReport
 *
 * Parameters:   data    - Pointer to data passed to UnwindStart()
 *               address - The return address of a stack frame.
 *
 * Returns:      TRUE if unwinding should continue, otherwise FALSE to
 *                 indicate that unwinding should stop.
 *
 * Description:  This function is called from the unwinder each time a stack
 *                 frame has been unwound.  The LSB of address indicates if
 *                 the processor is in ARM mode (LSB clear) or Thumb (LSB
 *                 set).
 *
 ***************************************************************************/
static bool CliReport(void *data, uint32_t address)
{
    CliStack *s = (CliStack *)data;

#if defined(UNW_DEBUG)
    trace_printf("\nCliReport: 0x%08x\n", address);
#endif

    s->address[s->frameCount] = address;
    s->frameCount++;

    if(s->frameCount >= (sizeof(s->address) / sizeof(s->address[0])))
    {
        return false;
    }
    else
    {
        return true;
    }
}

static bool CliReadW(const uint32_t a, uint32_t *v)
{
    *v = *(uint32_t *)a;
    return true;
}

static bool CliReadH(const uint32_t a, uint16_t *v)
{
    *v = *(uint16_t *)a;
    return true;
}

static bool CliReadB(const uint32_t a, int8_t *v)
{
    *v = *(int8_t *)a;
    return true;
}

bool CliInvalidateW(const uint32_t a)
{
    *(uint32_t *)a = 0xdeadbeef;
    return true;
}
/* Table of function pointers for passing to the unwinder */
extern void trace_printf(const char*,...);
const UnwindCallbacks cliCallbacks =
    {
        CliReport,
        CliReadW,
        CliReadH,
        CliReadB
#if defined(UNW_DEBUG)
       ,printk
#endif
    };

#endif /* UPGRADE_ARM_STACK_UNWIND */

/* END OF FILE */

