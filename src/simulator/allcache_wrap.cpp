/**
 *  Copyright (c) 2015, Ellis Giles, Peter Varman, and Kshitij Doshi
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors
 *     may be used to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include "pin.H"
#include "simcache.h"

CacheSim *cachesim;
UINT32 icount = 0;
unsigned long currentCycle = 0;
int started = 0;

void cycle(int c)
{
	for (int i = 0; i < c; i++)
	{
		currentCycle++;
	}
}

void memoryRead(ADDRINT addr, int size)
{
	fprintf(stderr, "R");
	cycle(50);
}

void memoryWrite(ADDRINT addr, int size)
{
	fprintf(stderr, "W");
	cycle(100);
}



LOCALFUN VOID Fini(int code, VOID * v)
{
    cachesim->printStats();
    printf("NumCycles = %ld\n", currentCycle);
}

// Print a memory read record
VOID RecordMemRead(VOID * ip, ADDRINT addr, int size, int numsince)
{
    //if (!started)
		//return;
    //fprintf(stderr,"%p L %p %d\n", ip, addr, numsince);
	//checkMemRead(addr);
	cycle(numsince);
	//dramsim->issue(addr, 0, PIN_ThreadId());
	cachesim->MemRefSingle(addr, size, ACCESS_TYPE_LOAD);
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, ADDRINT addr, int size, int numsince)
{
//	if (!started)
	//	return;
    //fprintf(stderr,"%p S %p %d\n", ip, addr, numsince);

	//checkMemWrite(addr);
	cycle(numsince);
	//dramsim->issue(addr, 1, PIN_ThreadId());
	cachesim->MemRefSingle(addr, size, ACCESS_TYPE_STORE );
}

LOCALFUN VOID InsRef(ADDRINT addr)
{
	cachesim->InsRef(addr);
}

LOCALFUN VOID Instruction(INS ins, VOID *v)
{
    // All instruction fetches access the I-cache.
    INS_InsertCall(
        ins, IPOINT_BEFORE, (AFUNPTR)InsRef,
        IARG_INST_PTR,
        IARG_END);
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    icount = 0;
    // Iterate over each memory operand of the instruction.
        for (UINT32 memOp = 0; memOp < memOperands; memOp++)
        {
        	UINT32 refSize = INS_MemoryOperandSize(ins, memOp);

            if (INS_MemoryOperandIsRead(ins, memOp))
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,
                    (AFUNPTR)RecordMemRead, IARG_INST_PTR, IARG_MEMORYOP_EA, memOp, IARG_UINT32, refSize,
                    IARG_UINT32, icount,
                    IARG_END);
            }
            // Note that in some architectures a single memory operand can be
            // both read and written (for instance incl (%eax) on IA-32)
            // In that case we instrument it once for read and once for write.
            if (INS_MemoryOperandIsWritten(ins, memOp))
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,
                    (AFUNPTR)RecordMemWrite, IARG_INST_PTR, IARG_MEMORYOP_EA, memOp, IARG_UINT32, refSize,
                    IARG_UINT32, icount,
                    IARG_END);
            }

            if (memOp > 0)
            {
            	fprintf(stderr, "M");
            }
        }

/*
    if (INS_IsMemoryRead(ins))
    {
        const UINT32 size = INS_MemoryReadSize(ins);
        const AFUNPTR countFun = (size <= 4 ? (AFUNPTR) MemRefSingle : (AFUNPTR) MemRefMulti);
        fprintf(stderr, "%d ", size);

        // only predicated-on memory instructions access D-cache
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, countFun,
            IARG_MEMORYREAD_EA,
            IARG_MEMORYREAD_SIZE,
            IARG_UINT32, ACCESS_TYPE_LOAD,
            IARG_END);
    }

    if (INS_IsMemoryWrite(ins))
    {
        const UINT32 size = INS_MemoryWriteSize(ins);
        const AFUNPTR countFun = (size <= 4 ? (AFUNPTR) MemRefSingle : (AFUNPTR) MemRefMulti);

        // only predicated-on memory instructions access D-cache
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, countFun,
            IARG_MEMORYWRITE_EA,
            IARG_MEMORYWRITE_SIZE,
            IARG_UINT32, ACCESS_TYPE_STORE,
            IARG_END);
    }
    */
}

GLOBALFUN int main(int argc, char *argv[])
{
    PIN_Init(argc, argv);

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    //  CacheSim
    cachesim = new CacheSim();

    // Never returns
    PIN_StartProgram();

    return 0; // make compiler happy
}

