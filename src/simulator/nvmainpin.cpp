/*******************************************************************************
* Copyright (c) 2012-2013, The Microsystems Design Labratory (MDL)
* Department of Computer Science and Engineering, The Pennsylvania State University
* All rights reserved.
* 
* This source code is part of NVMain - A cycle accurate timing, bit accurate
* energy simulator for both volatile (e.g., DRAM) and nono-volatile memory
* (e.g., PCRAM). The source code is free and you can redistribute and/or
* modify it by providing that the following conditions are met:
* 
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
* 
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* Author list: 
*   Matt Poremba    ( Email: mrp5060 at psu dot edu 
*                     Website: http://www.cse.psu.edu/~poremba/ )
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "simnvm.h"
#include "pin.H"


//FILE * trace;
UINT32 icount = 0;
NVMSim *nvmsim;
unsigned long currentCycle = 0;
int started = 0;

void cycle(int c)
{
	for (int i = 0; i < c; i++)
	{
		currentCycle++;
		nvmsim->update();
	}
}


// Print a memory read record
VOID RecordMemRead(VOID * ip, VOID * addr, int numsince)
{
    //fprintf(trace,"%p L %p %d\n", ip, addr, numsince);
	if (!started)
		return;

	//checkMemRead(addr);
	cycle(numsince);
	nvmsim->issue(addr, READ, PIN_ThreadId());
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, int numsince)
{
    //fprintf(trace,"%p S %p %d\n", ip, addr, numsince);
	if (!started)
		return;

	//checkMemWrite(addr);
	cycle(numsince);
	nvmsim->issue(addr, WRITE, PIN_ThreadId());
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

    nvmsim->printStats( );
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

    //  NVMain
    nvmsim = new NVMSim();

    // Never returns
    PIN_StartProgram();
    //  PIN_StartProgramProbe();  For IMG processing probing.

    return 0;
}


int oldmain( int argc, char *argv[] )
{
    Config *config = new Config( );
    GenericTrace *trace = NULL;
    TraceLine *tl = new TraceLine( );
    SimInterface *simInterface = new NullInterface( );
    NVMain *nvmain = new NVMain( );
    EventQueue *mainEventQueue = new EventQueue( );

    unsigned int simulateCycles;
    unsigned int currentCycle;
    
    if( argc != 3 && argc != 4 )
    {
        std::cout << "Usage: nvmain CONFIG_FILE TRACE_FILE [CYCLES]" 
            << std::endl;
        return 1;
    }

    config->Read( argv[1] );
    config->SetSimInterface( simInterface );
    nvmain->SetEventQueue( mainEventQueue );

    /*  Add any specified hooks */
    std::vector<std::string>& hookList = config->GetHooks( );

    for( size_t i = 0; i < hookList.size( ); i++ )
    {
        std::cout << "Creating hook " << hookList[i] << std::endl;

        NVMObject *hook = HookFactory::CreateHook( hookList[i] );
        
        if( hook != NULL )
        {
            nvmain->AddHook( hook );
            hook->SetParent( nvmain );
            hook->Init( config );
        }
        else
        {
            std::cout << "Warning: Could not create a hook named `" 
                << hookList[i] << "'." << std::endl;
        }
    }

    simInterface->SetConfig( config );
    nvmain->SetConfig( config, "defaultMemory" );


    if( config->KeyExists( "TraceReader" ) )
        trace = TraceReaderFactory::CreateNewTraceReader( 
                config->GetString( "TraceReader" ) );
    else
        trace = TraceReaderFactory::CreateNewTraceReader( "NVMainTrace" );

    trace->SetTraceFile( argv[2] );

    if( argc == 3 )
        simulateCycles = 0;
    else
        simulateCycles = atoi( argv[3] );

    simulateCycles *= (unsigned int)ceil( (double)(config->GetValue( "CPUFreq" )) 
                        / (double)(config->GetValue( "CLK" )) ); 


    currentCycle = 0;
    while( currentCycle <= simulateCycles || simulateCycles == 0 )
    {
        if( !trace->GetNextAccess( tl ) )
        {
            std::cout << "Could not read next line from trace file!" 
                << std::endl;

            /* Just ride it out 'til the end. */
            while( currentCycle < simulateCycles )
            {
                nvmain->Cycle( 1 );
              
                currentCycle++;
            }

            break;
        }

        NVMainRequest *request = new NVMainRequest( );
        
        request->address.SetPhysicalAddress( tl->GetAddress( ) );
        request->type = tl->GetOperation( );
        request->bulkCmd = CMD_NOP;
        request->threadId = tl->GetThreadId( );
        request->data = tl->GetData( );
        request->status = MEM_REQUEST_INCOMPLETE;
        request->owner = (NVMObject *)nvmain;
        
        /* 
         * If you want to ignore the cycles used in the trace file, just set
         * the cycle to 0. 
         */
        if( config->KeyExists( "IgnoreTraceCycle" ) 
                && config->GetString( "IgnoreTraceCycle" ) == "true" )
            tl->SetLine( tl->GetAddress( ), tl->GetOperation( ), 0, 
                            tl->GetData( ), tl->GetThreadId( ) );

        if( request->type != READ && request->type != WRITE )
            std::cout << "traceMain: Unknown Operation: " << request->type 
                << std::endl;

        /* 
         * If the next operation occurs after the requested number of cycles,
         * we can quit. 
         */
        if( tl->GetCycle( ) > simulateCycles && simulateCycles != 0 )
        {
            /* Just ride it out 'til the end. */
            while( currentCycle < simulateCycles )
            {
                nvmain->Cycle( 1 );
              
                currentCycle++;
            }

            break;
        }
        else
        {
            /* 
             *  If the command is in the past, it can be issued. This would 
             *  occur since the trace was probably generated with an inaccurate 
             *  memory *  simulator, so the cycles may not match up. Otherwise, 
             *  we need to wait.
             */
            if( tl->GetCycle( ) > currentCycle )
            {
                /* Wait until currentCycle is the trace operation's cycle. */
                while( currentCycle < tl->GetCycle( ) )
                {
                    if( currentCycle >= simulateCycles )
                        break;

                    nvmain->Cycle( 1 );

                    currentCycle++;
                }

                if( currentCycle >= simulateCycles )
                    break;
            }

            /* 
             *  Wait for the memory controller to accept the next command.. 
             *  the trace reader is "stalling" until then.
             */
            while( !nvmain->IssueCommand( request ) )
            {
                if( currentCycle >= simulateCycles )
                    break;

                nvmain->Cycle( 1 );

                currentCycle++;
            }

            if( currentCycle >= simulateCycles )
                break;
        }
    }       

    nvmain->PrintStats( );

    std::cout << "Exiting at cycle " << currentCycle << " because simCycles " 
        << simulateCycles << " reached." << std::endl; 

    delete config;

    return 0;
}
