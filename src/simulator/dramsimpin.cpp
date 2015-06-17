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

#include <string>
#include <stdint.h>
#include <stdio.h>
#include "simdram.h"
#include "pin.H"

UINT32 icount = 0;
unsigned long currentCycle = 0;
int started = 0;


DRAM *dramsim;

void cycle(int c)
{
	for (int i = 0; i < c; i++)
	{
		currentCycle++;
		dramsim->update();
	}
}

bool wait = false;
void waitRead()
{
	while (wait)
	{
		cycle(1);
	}
}
void onReadComplete(uint64_t address)
{
	wait = false;
}

// Print a memory read record
VOID RecordMemRead(VOID * ip, VOID * addr, int numsince)
{
    if (!started)
		return;
    //fprintf(stderr,"%p L %p %d\n", ip, addr, numsince);
	//checkMemRead(addr);
	cycle(numsince);
	wait = true;
	dramsim->issue(addr, 0, PIN_ThreadId());
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, int numsince)
{
	if (!started)
		return;
    //fprintf(stderr,"%p S %p %d\n", ip, addr, numsince);

	//checkMemWrite(addr);
	cycle(numsince);
	dramsim->issue(addr, 1, PIN_ThreadId());
}

int checkSymbolName(RTN rtn, const char *name)
{
	if (PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY) == name)
		return 1;
	return 0;
}

void doGetPinStatistics(char *c)
{
	sprintf(c, "Hello from PIN!  Cycles = %ld", currentCycle);
	started = 0;
}
void doStartStatistics()
{
	started = 1;
}

VOID Routine(RTN rtn, VOID *v)
{
	//  Override the getPinStatistics routine.
	if (checkSymbolName(rtn, "getPinStatistics"))
	{
		RTN_Replace(rtn, (AFUNPTR)doGetPinStatistics);
	}
	//  Start collecting statistics.
	if (checkSymbolName(rtn, "startStatistics"))
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)doStartStatistics, IARG_END);
		RTN_Close(rtn);
	}
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
	icount++;

    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // The IA-64 architecture has explicitly predicated instructions.
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE,
                (AFUNPTR)RecordMemRead, IARG_INST_PTR, IARG_MEMORYOP_EA, memOp, IARG_UINT32, icount,
                IARG_END);
            icount = 0;
        }
        // Note that in some architectures a single memory operand can be
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE,
                (AFUNPTR)RecordMemWrite, IARG_INST_PTR, IARG_MEMORYOP_EA, memOp, IARG_UINT32, icount,
                IARG_END);
            icount = 0;
        }
    }
}

VOID Fini(INT32 code, VOID *v)
{
    //fprintf(trace, "#eof\n");
//    fclose(trace);
	//free(numWrapsOpen);
	//free(inWrapRead);
	//free(inWrapWrite);

    dramsim->printStats( );
}


/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR( "This Pintool prints a trace of memory addresses\n"
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    // Initialize symbol processing for routine and image processing
    PIN_InitSymbols();

    if (PIN_Init(argc, argv)) return Usage();

    // Initialize the pin lock
  //  InitLock(&lock);

 //   trace = fopen(KnobOutputFile.Value().c_str(), "w");

    RTN_AddInstrumentFunction(Routine, 0);
    //IMG_AddInstrumentFunction(Image, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    dramsim = new DRAM();

    // Never returns
    PIN_StartProgram();
    //  PIN_StartProgramProbe();  For IMG processing probing.

    return 0;
}

