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

/*
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <map>
#include <errno.h>

#include "simnvm.h"
#include "pin.H"
#include "simcache.h"
#include "simdram.h"
#include "pmem.h"
#include "debug.h"

//FILE * trace;
UINT32 icount = 0;
#define MAXTHREADS 128
int *numWrapsOpen = (int*)calloc(MAXTHREADS, sizeof(int));
CacheSim *cachesim;
DRAM *dramsim;
DRAM *nvmsim;
UINT64 currentCycle = 0;
UINT64 ticCycle = 0;  // used for -tictoc overloading of client clock start time.
//UINT32 nWtstores = 0;
UINT32 nNtstores = 0;
UINT32 nSyncs = 0;
UINT64 nInstructions = 0;
bool started = false;
int nThreads = 0;
int nMaxThreads = 0;
int totalThreads = 0;
int nActive = 0;
//  TODO
//int wtstoreCycles = 6;
int ntstoreCycles = 4;
//  LOCKS
LEVEL_BASE::PIN_MUTEX _mutex;
PIN_LOCK pmemlock;
PIN_LOCK activelock;
PIN_SEMAPHORE _memorySema[MAXTHREADS];
#define GetLock PIN_GetLock
#define ReleaseLock PIN_ReleaseLock
#define InitLock PIN_InitLock

//  Command line options for the pin wrap tool.
//KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o",
//	"pinwraptrace.out", "specify memory trace file");
KNOB<BOOL>   KnobCheckWrap(KNOB_MODE_WRITEONCE, "pintool", "checkwrap",
		"0", "checks persistent accesses are wrapped");
KNOB<BOOL>   KnobAssertWrap(KNOB_MODE_WRITEONCE, "pintool", "assertwrap",
		"0", "assert that persistent accesses are wrapped");
KNOB<BOOL>   KnobHardwareWrap(KNOB_MODE_WRITEONCE, "pintool", "hardwarewrap",
		"0", "models a hardware wrap");
KNOB<BOOL>   KnobInterleaved(KNOB_MODE_WRITEONCE, "pintool", "interleaved",
		"0", "on a cache miss, if other instructions are waiting, start executing those while waiting on memory." \
		"If 0, then wait until the memory read is complete.");
KNOB<BOOL>   KnobTraceWrap(KNOB_MODE_WRITEONCE, "pintool", "tracewrap",
		"0", "trace wrap open and close function calls");
KNOB<string> KnobDramSim(KNOB_MODE_WRITEONCE, "pintool", "dramsim",
		"", "specify DramSim2 Configuration File");
KNOB<string> KnobNVMain(KNOB_MODE_WRITEONCE, "pintool", "nvmain",
		"", "specify slowdown factor (if zero then all in dram) for dramsim equivalent nvm or NVMain Configuration File");
KNOB<BOOL>   KnobCompStats(KNOB_MODE_WRITEONCE, "pintool", "compstats",
		"0", "print statistics for each of the components");
KNOB<BOOL>   KnobTicToc(KNOB_MODE_WRITEONCE, "pintool", "tictoc",
		"1", "overload tic() and toc() functions to start timer and get time elapsed to be simulated cycles");

int *inWrapRead = (int *)calloc(128, sizeof(int));
int *inWrapWrite = (int *)calloc(128, sizeof(int));
int NVMSlowdownFactor = 1;

class Locker
{
public:
	Locker()
	{
		PIN_MutexLock(&_mutex);
	}
	~Locker()
	{
		PIN_MutexUnlock(&_mutex);
	}
};


void setInWrapRead(int flag)
{
	inWrapRead[PIN_ThreadId()] = flag;
}
void setInWrapWrite(int flag)
{
	inWrapWrite[PIN_ThreadId()] = flag;
}

bool isInWrapRead()
{
	return (numWrapsOpen[PIN_ThreadId()] > 0) && (inWrapRead[PIN_ThreadId()] > 0);
}

bool isInWrapWrite()
{
	return (numWrapsOpen[PIN_ThreadId()] > 0) && (inWrapWrite[PIN_ThreadId()] > 0);
}

VOID markWrapOpen(int returnValue, int threadID)
{
	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "markWrapOpen(returnValue=%d, threadID=%d)\n", returnValue, threadID);
		LOG(buf);
	}
	numWrapsOpen[threadID]++;
}

VOID markWrapClose(int wrapToken, int threadID)
{
	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "markWrapClose(wrapToken=%d, threadID=%d)\n", wrapToken, threadID);
		LOG(buf);
	}
	numWrapsOpen[threadID]--;
}

void doPersistentNotifyPin(void *v, size_t size, bool isLogArea)
{
	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "doPersistentNotifyPin(%p, %ld %d)\n", v, size, isLogArea);
		LOG(buf);
	}
	GetLock(&pmemlock, PIN_ThreadId());
	allocatedPMem(v, size);
	ReleaseLock(&pmemlock);
}

void doPFreeNotifyPin(void *p)
{
	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "doPFreeNotifyPin(%p)\n", p);
		LOG(buf);
	}
	GetLock(&pmemlock, PIN_ThreadId());
	freedPMem(p);
	ReleaseLock(&pmemlock);
}

int doHardwareWrapOpen()
{
	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "doHardwareWrapOpen() threadID=%d\n", PIN_ThreadId());
		LOG(buf);
	}
	//  Do whatever we need for wrap open in hardware.
	//  TODO
	int returnValue = random();

	if ((KnobCheckWrap.Value() || KnobAssertWrap.Value()) && KnobHardwareWrap.Value())
	{
		markWrapOpen(returnValue, PIN_ThreadId());
	}
	return returnValue;
}

int doHardwareWrapClose(int wrapToken)
{
	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "doHardwareWrapClose(wrapToken=%d) threadID=%d\n", wrapToken, PIN_ThreadId());
		LOG(buf);
	}
	if ((KnobCheckWrap.Value() || KnobAssertWrap.Value()) && KnobHardwareWrap.Value())
	{
		markWrapClose(wrapToken, PIN_ThreadId());
	}

	//  Do whatever we need to do for wrap close in hardware.
	//  TODO
	return 1;
}

void doHardwareWrapWrite(void *ptr, void *src, int size, int w)
{
	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "doHardwareWrapWrite(ptr=%p, src=%p, size=%d, wrapToken=%d) threadID=%d\n",
				ptr, src, size, w, PIN_ThreadId());
		LOG(buf);
	}
	if ((KnobCheckWrap.Value() || KnobAssertWrap.Value()) && KnobHardwareWrap.Value())
	{
		setInWrapWrite(1);
	}

	//  Do whatever we need to do for wrap write in hardware.
	//  TODO

	if ((KnobCheckWrap.Value() || KnobAssertWrap.Value()) && KnobHardwareWrap.Value())
	{
		setInWrapWrite(0);
	}
	return;
}

void *doHardwareWrapRead(void *ptr, int size, int w)
{
	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "doHardwareWrapRead(ptr=%p, size=%d, wrapToken=%d) threadID=%d\n",
				ptr, size, w, PIN_ThreadId());
		LOG(buf);
	}
	if ((KnobCheckWrap.Value() || KnobAssertWrap.Value()) && KnobHardwareWrap.Value())
	{
		setInWrapRead(1);
	}

	//  Do whatever we need to do for wrap write in hardware.
	//  TODO
	void *ret = ptr;

	if ((KnobCheckWrap.Value() || KnobAssertWrap.Value()) && KnobHardwareWrap.Value())
	{
		setInWrapRead(0);
	}
	return ret;
}

int checkSymbolName(RTN rtn, const char *name)
{
	if (PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY) == name)
		return 1;
	return 0;
}

void doGetPinStatistics(char *c)
{
	//	started = false;

	//sprintf(c, "Hello from PIN!  Cycles = %ld", currentCycle);
	sprintf(c, "Pin: \tL1HME: \t%u \t%u \t%u \tL2HME: \t%u \t%u \t%u \tL3HME: \t%u \t%u \t%u \t" \
			"DRamReads: \t%lu \tDRamWrites: \t%lu \tSCMReads: \t%lu \tSCMWrites: \t%lu \t" \
			//"WtStores: \t%u \t"
			"NtStores: \t%u \tSyncs: \t%u \tInst: \t%lu \tTotalCycles: \t%lu",
			dl1.Hits(), dl1.Misses(), dl1.Evictions(), ul2.Hits(), ul2.Misses(), ul2.Evictions(),
			ul3.Hits(), ul3.Misses(), ul3.Evictions(),
			dramsim->nReads, dramsim->nWrites, nvmsim->nReads, nvmsim->nWrites,
			//nWtstores,
			nNtstores, nSyncs, nInstructions, currentCycle);
}
void doStartPinStatistics()
{
	started = true;
}

void cycle(int c)
{
	for (int i = 0; i < c; i++)
	{
		currentCycle++;
		dramsim->update();

		if (NVMSlowdownFactor > 1)
		{
			if (currentCycle % NVMSlowdownFactor == 0)
				nvmsim->update();
			else
			{
				//Debug("Skip\n");
			}
		}
		else
			nvmsim->update();
	}
}

/*
bool wait = false;
void waitRead()
{
	while (wait)
	{
		cycle(1);
	}
}
extern void onReadComplete(uint64_t address)
{
	wait = false;
}
 */


bool _wait = false;
int numWaiting = 0;
map <unsigned long, int> _readMap;
void waitRead()
{
	GetLock(&activelock, PIN_ThreadId());
	numWaiting++;
	ReleaseLock(&activelock);

	if (KnobInterleaved.Value())
	{
		PIN_MutexUnlock(&_mutex);
		while (!PIN_SemaphoreIsSet(&(_memorySema[PIN_ThreadId()])))
		{
			bool docycle = false;
			PIN_MutexLock(&_mutex);
			GetLock(&activelock, PIN_ThreadId());
			//Debug("In thread %d  nActive=%d \tnumWaiting=%d\n",  PIN_ThreadId(), nActive, numWaiting);
			if (nActive - numWaiting <= 0)
				docycle = true;
			ReleaseLock(&activelock);

			if (docycle)
				cycle(1);
			PIN_MutexUnlock(&_mutex);
			//PIN_SemaphoreTimedWait(&(_memorySema[PIN_ThreadId()]), 500);
		}
		PIN_MutexLock(&_mutex);
	}
	else
	{
		while (_wait)
		{
			cycle(1);
		}
	}
}

void onReadComplete(uint64_t addr)
{
	GetLock(&activelock, PIN_ThreadId());
	numWaiting--;
	ReleaseLock(&activelock);

	if (KnobInterleaved.Value())
	{
		addr &= ~(64-1);
		//Debug("arrd=%d Setting sema for thread %d\n", addr, _readMap[addr]);
		PIN_SemaphoreSet(&(_memorySema[_readMap[addr]]));
		assert(_readMap[addr] != -1);
		_readMap[addr] = -1;
	}
	else
	{
		_wait = false;
	}
}

void memoryRead(ADDRINT addr, int size)
{
	//cycle(50);
	//Debug("memory read addr=%d thread=%d\n", addr, PIN_ThreadId());
	if (KnobInterleaved.Value())
	{
		PIN_SemaphoreClear(&(_memorySema[PIN_ThreadId()]));
		_readMap[addr & ~63] = PIN_ThreadId();
	}
	else
	{
		_wait = true;
	}

	if (isInPMem((void*)addr))
	{
		Debug("R");
		//	nvmsim->issue((void*)addr, READ, PIN_ThreadId());
		nvmsim->issue((void*)addr, 0, PIN_ThreadId());
		if (size > 32)
			nvmsim->issue((void *)((unsigned long)addr + 32), 0, PIN_ThreadId());
	}
	else
	{
		Debug("r");
		dramsim->issue((void *)addr, 0, PIN_ThreadId());
	}
}

void memoryWrite(void *addr, int size)
{
	//cycle(100);
	//Debug("memoryWrite %p\n", (void*)addr);
	if (isInPMem((void *)addr))
	{
		Debug("W");
		//		nvmsim->issue((void *)addr, WRITE, PIN_ThreadId());
		nvmsim->issue((void *)addr, 1, PIN_ThreadId());
		if (size > 32)
			nvmsim->issue((void *)((unsigned long)addr + 32), 1, PIN_ThreadId());

	}
	else
	{
		Debug("w");
		dramsim->issue((void *)addr, 1, PIN_ThreadId());
	}
}


/*
PIN_GetSourceLocation	( 	ADDRINT 	address,
INT32 * 	column,
INT32 * 	line,
string * 	fileName
)
 */

void dumpDebugInfo(ADDRINT instructionAddress, const char *str)
{
	INT32 column, line;
	string fileName;
	PIN_LockClient();
	PIN_GetSourceLocation(instructionAddress, &column, &line, &fileName);
	PIN_UnlockClient();

	fflush(stdout);
	fprintf(stdout, "In file %s line %d, persistent %s not under wrap bounds.\n",
			//		str, (void*)instructionAddress,
			//		fileName.c_str(), line, column);
			fileName.c_str(), line, str);

	fflush(stdout);
}

void checkMemRead(void *addr, ADDRINT instructionAddress)
{
	int tid = PIN_ThreadId();
	if ((KnobCheckWrap.Value() || KnobAssertWrap.Value()) && !isInWrapRead() && isInPMem(addr))
	{
		char buf[256];
		sprintf(buf, "Error - Persistent memory read access out of a wrapRead at address %p for thread %d\n", addr, tid);
		LOG(buf);
		dumpDebugInfo(instructionAddress, "Read");

		if (KnobAssertWrap.Value())
		{
			fprintf(stderr, "%s", buf);
			assert(isInPMem(addr) && inWrapRead);
		}
	}
}

void checkMemWrite(void *addr, ADDRINT instructionAddress)
{
	int tid = PIN_ThreadId();
	if ((KnobCheckWrap.Value() || KnobAssertWrap.Value()) && !isInWrapWrite() && isInPMem(addr))
	{
		char buf[256];
		sprintf(buf, "Error - Persistent memory write access out of a wrap at address %p for thread %d\n", addr, tid);
		LOG(buf);
		dumpDebugInfo(instructionAddress, "Write");

		if (KnobAssertWrap.Value())
		{
			fprintf(stderr, "%s", buf);
			assert(isInPMem(addr) && (numWrapsOpen[tid] > 0) && inWrapWrite);
		}
	}
}

// Print a memory read record
VOID RecordMemRead(VOID * ip, VOID * addr, int size, int numsince)
{
	//fprintf(trace,"%p L %p %d\n", ip, addr, numsince);
	checkMemRead(addr, (ADDRINT)ip);

	if (!started)
		return;

	cycle(numsince);
	//dramsim->issue(addr, 0, PIN_ThreadId());
	while (!cachesim->MemRefSingle((ADDRINT)addr, size, ACCESS_TYPE_LOAD))
	{
		Debug("&");
	}
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, int size, int numsince)
{
	//fprintf(trace,"%p S %p %d\n", ip, addr, numsince);
	checkMemWrite(addr, (ADDRINT)ip);

	if (!started)
		return;
	cycle(numsince);
	//dramsim->issue(addr, 1, PIN_ThreadId());

	while (!cachesim->MemRefSingle((ADDRINT)addr, size, ACCESS_TYPE_STORE))
	{
		Debug("&");
	}
}

//void doNtstore(void *dest, void *src)
void doStreamingStore(int *p, int a)
{
	Locker l;
	//int size = 32;

	//  Increment the number of ntstores
	nNtstores++;

	//printf("N");
	Debug("N");

	cycle(ntstoreCycles);

	//  Actually copy the data.
	/*if (dest != src)
	{
		memcpy(dest, src, size);

		//  Record the mem read.
		RecordMemRead(NULL, src, size, 0);
	}
	*/
	*p = a;

	//  Write through to memory.
	//memoryWrite(dest, size);
	//assert(size <= 64);
	memoryWrite(p,4);
}

/*
void doWtstore(void *dest, void *src, int size)
{
	//  Increment the number of wtstores.
	nWtstores++;

	//Debug("doWtstore %p %p %d\n", dest, src, size);
	Debug("T");

	//  Pay a small penalty?
	cycle(wtstoreCycles);

	//  Copy the data and write to the cache.
	if (dest != src)
	{
		memcpy(dest, src, size);

		//  Record the mem read.
		RecordMemRead(NULL, src, size, 0);

		//  Record the mem write.
		RecordMemWrite(NULL, dest, size, 0);
	}

	//  Write through to memory.
	memoryWrite(dest, size);
	assert(size <= 64);
}
 */

void doCacheFlush()
{
	Locker l;

	Debug("doCacheFlush\n");
	assert(0);
}

void doClflush(VOID *p)
{
	Locker l;

	//  Clflush will call cycle.
	cachesim->clflush((ADDRINT)p);
}

void doPMsync()
{
	Locker l;

	Debug("S");
	nSyncs++;
	//  TODO syncing both!
	//Debug("doPMsync %d\n", nvmsim->getWriteBufferSize());
	//while ((nvmsim->getWriteBufferSize() > 0) || (dramsim->getWriteBufferSize() > 0))
	while (nvmsim->getWriteBufferSize() > 0)
	{
		//printf("%d", nvmsim->getWriteBufferSize());
		cycle(1);
	}
}

long doGetTime()
{
	return currentCycle;
}
void doTic()
{
	ticCycle = currentCycle;
}

double doToc()
{
	return currentCycle - ticCycle;
}

VOID Routine(RTN rtn, VOID *v)
{
	//LOG(RTN_Name(rtn) + " " + PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY) + "\n");
	if ((KnobCheckWrap.Value() || KnobAssertWrap.Value()) && !KnobHardwareWrap.Value())
	{
		if (checkSymbolName(rtn, "wrapOpen"))
		{
			RTN_Open(rtn);
			// Insert a call at the exit point of the wrap open routine to increment wrap open count for the thread.
			RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)markWrapOpen, IARG_FUNCRET_EXITPOINT_VALUE,  IARG_THREAD_ID, IARG_END);
			RTN_Close(rtn);
		}
		if (checkSymbolName(rtn, "wrapClose"))
		{
			RTN_Open(rtn);
			// Insert a call at the entry point of the wrap close routine to decrement the thread's wrap open count.
			RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)markWrapClose, IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
					IARG_THREAD_ID, IARG_END);
			RTN_Close(rtn);
		}
		if (checkSymbolName(rtn, "wrapRead") || checkSymbolName(rtn, "wrapLoad32") || checkSymbolName(rtn, "wrapLoad64"))
		{
			RTN_Open(rtn);
			RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)setInWrapRead, IARG_UINT32, 1, IARG_END);
			RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)setInWrapRead, IARG_UINT32, 0, IARG_END);
			RTN_Close(rtn);
		}
		if (checkSymbolName(rtn, "wrapWrite") || checkSymbolName(rtn, "wrapStore32") || checkSymbolName(rtn, "wrapStore64"))
		{
			RTN_Open(rtn);
			RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)setInWrapWrite, IARG_UINT32, 1, IARG_END);
			RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)setInWrapWrite, IARG_UINT32, 0, IARG_END);
			RTN_Close(rtn);
		}
	}
	//  Enable hardware wrap through direct.
	if (KnobHardwareWrap.Value() == 1)
	{
		if (checkSymbolName(rtn, "wrapOpen"))
		{
			RTN_Replace(rtn, (AFUNPTR)doHardwareWrapOpen);
		}
		if (checkSymbolName(rtn, "wrapClose"))
		{
			RTN_Replace(rtn, (AFUNPTR)doHardwareWrapClose);
		}
		if (checkSymbolName(rtn, "wrapRead"))
		{
			RTN_Replace(rtn, (AFUNPTR)doHardwareWrapRead);
		}
		if (checkSymbolName(rtn, "wrapWrite"))
		{
			RTN_Replace(rtn, (AFUNPTR)doHardwareWrapWrite);
		}
	}
	else
	{
		//  Also could enable hardware wrap at the application level and only pay a small cost for class checking.
		if (checkSymbolName(rtn, "hardwareWrapOpen"))
		{
			RTN_Replace(rtn, (AFUNPTR)doHardwareWrapOpen);
		}
		if (checkSymbolName(rtn, "hardwareWrapClose"))
		{
			RTN_Replace(rtn, (AFUNPTR)doHardwareWrapClose);
		}
		if (checkSymbolName(rtn, "hardwareWrapRead"))
		{
			RTN_Replace(rtn, (AFUNPTR)doHardwareWrapRead);
		}
		if (checkSymbolName(rtn, "hardwareWrapWrite"))
		{
			RTN_Replace(rtn, (AFUNPTR)doHardwareWrapWrite);
		}
	}
	//  Capture persistent memory locations from the application space when notifying pin.
	if (checkSymbolName(rtn, "persistentNotifyPin"))
	{
		RTN_Replace(rtn, (AFUNPTR)doPersistentNotifyPin);
	}
	/*
	//  Overload wtstore operations C function for now.
	if (checkSymbolName(rtn, "wtstore"))
	{
		RTN_Replace(rtn, (AFUNPTR)doWtstore);
	}
	 */
	//  Overload ntstore operations C function for now.
	//if (checkSymbolName(rtn, "ntstore"))
	if (checkSymbolName(rtn, "streamingStore"))
	{
		RTN_Replace(rtn, (AFUNPTR)doStreamingStore);//doNtstore);
	}
	//  Overload cache flush operations.
	if (checkSymbolName(rtn, "cacheflush"))
	{
		RTN_Replace(rtn, (AFUNPTR)doCacheFlush);
	}
	//  Overload cache line flush operations.
	if (checkSymbolName(rtn, "clflush"))
	{
		RTN_Replace(rtn, (AFUNPTR)doClflush);
	}

	//  Overload persistent memory sync.
	if (checkSymbolName(rtn, "p_msync"))
	{
		RTN_Replace(rtn, (AFUNPTR)doPMsync);
	}

	//  Mark the persistent memory location as freed.
	if (checkSymbolName(rtn, "pfreeNotifyPin"))
	{
		RTN_Replace(rtn, (AFUNPTR)doPFreeNotifyPin);
	}

	//  Override the getPinStatistics routine.
	if (checkSymbolName(rtn, "getPinStatistics"))
	{
		RTN_Replace(rtn, (AFUNPTR)doGetPinStatistics);
	}
	//  Start collecting statistics.
	if (checkSymbolName(rtn, "startPinStatistics"))
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)doStartPinStatistics, IARG_END);
		RTN_Close(rtn);
	}

	//  Replace tic() and toc() functions to start and stop a timer to get number of elapsed simulated cpu cycles
	if (KnobTicToc.Value())
	{
		if (checkSymbolName(rtn, "getTime"))
		{
			RTN_Replace(rtn, (AFUNPTR)doGetTime);
		}
		if (checkSymbolName(rtn, "tic"))
		{
			RTN_Replace(rtn, (AFUNPTR)doTic);
		}
		if (checkSymbolName(rtn, "toc"))
		{
			RTN_Replace(rtn, (AFUNPTR)doToc);
		}
	}

}


LOCALFUN VOID InsRef(ADDRINT addr)
{
	GetLock(&activelock, PIN_ThreadId());
	nActive++;
	ReleaseLock(&activelock);

	PIN_MutexLock(&_mutex);

	if (!started)
		return;

	cachesim->InsRef(addr);
}

LOCALFUN VOID InsComplete(ADDRINT addr)
{
	GetLock(&activelock, PIN_ThreadId());
	nActive--;
	ReleaseLock(&activelock);

	PIN_MutexUnlock(&_mutex);
}

// This routine is executed every time a thread is created.
VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
	Locker l;
	Debug("thread begin %d\n",threadid);
	//  Init Semaphore
	PIN_SemaphoreInit(&(_memorySema[threadid]));
	//  Count the threads.
	nThreads++;
	if (nThreads > nMaxThreads)
		nMaxThreads = nThreads;
	totalThreads++;
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
	Locker l;
	Debug("thread end %d code %d\n",threadid, code);
	//  Free Semaphore
	PIN_SemaphoreFini(&(_memorySema[threadid]));
	//  Count the threads.
	nThreads--;
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
	//icount++;
	icount = 0;
	nInstructions++;

	// All instruction fetches access the I-cache.
	INS_InsertCall(
			ins, IPOINT_BEFORE, (AFUNPTR)InsRef,
			IARG_INST_PTR,
			IARG_END);


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
		UINT32 refSize = INS_MemoryOperandSize(ins, memOp);

		if (INS_MemoryOperandIsRead(ins, memOp))
		{
			INS_InsertPredicatedCall(
					ins, IPOINT_BEFORE,
					(AFUNPTR)RecordMemRead, IARG_INST_PTR, IARG_MEMORYOP_EA, memOp, IARG_UINT32, refSize,
					IARG_UINT32, icount,
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
					(AFUNPTR)RecordMemWrite, IARG_INST_PTR, IARG_MEMORYOP_EA, memOp, IARG_UINT32, refSize,
					IARG_UINT32, icount,
					IARG_END);
			icount = 0;
		}

		//  Print an M if it's a multi memory operand call.
		if (memOp == 1)
		{
			Debug("M");
		}
	}

	// All instruction fetches access the I-cache.
	INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)InsComplete, IARG_INST_PTR, IARG_END);
}

VOID Fini(INT32 code, VOID *v)
{
	printf("\n");

	if (KnobCompStats.Value())
	{
		cachesim->printStats();
		dramsim->printStats( );
		nvmsim->printStats( );
	}
	printf("NumCycles = %ld\n", currentCycle);

	free(numWrapsOpen);
	free(inWrapRead);
	free(inWrapWrite);
	delete cachesim;
	delete dramsim;
	delete nvmsim;

	//  Free locks.
	PIN_MutexFini(&_mutex);
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
	bzero(inWrapRead, 128 * sizeof(int));
	bzero(inWrapWrite, 128 * sizeof(int));

	// Initialize symbol processing for routine and image processing
	PIN_InitSymbols();

	if (PIN_Init(argc, argv)) return Usage();

	// Initialize the pin locks
	InitLock(&pmemlock);
	InitLock(&activelock);
	PIN_MutexInit (&_mutex);

	cachesim = new CacheSim();
	if (KnobDramSim.Value().length())
	{
		dramsim = new DRAM(KnobDramSim.Value().c_str());
	}
	else
		dramsim = new DRAM();

	//  NVMain
	if (KnobNVMain.Value().length())
	{
		errno = 0;
		int factor = strtol(KnobNVMain.Value().c_str(), NULL, 10);
		if ((factor < 0) || (errno != 0))
		{
			//nvmsim = new NVMSim(KnobNVMain.Value().c_str());
			nvmsim = new DRAM(KnobNVMain.Value().c_str());
		}
		else if (factor == 0)
		{
			nvmsim = dramsim;
		}
		else
		{
			printf("NVM Slowdown Factor = %d\n", factor);
			if (KnobDramSim.Value().length())
			{
				nvmsim = new DRAM(KnobDramSim.Value().c_str());
			}
			else
				nvmsim = new DRAM();
			NVMSlowdownFactor = factor;
		}
	}
	else
		nvmsim = new DRAM("DDR2_PCM.ini");

	RTN_AddInstrumentFunction(Routine, 0);
	//IMG_AddInstrumentFunction(Image, 0);
	INS_AddInstrumentFunction(Instruction, 0);

	// Register Analysis routines to be called when a thread begins/ends
	PIN_AddThreadStartFunction(ThreadStart, 0);
	PIN_AddThreadFiniFunction(ThreadFini, 0);

	PIN_AddFiniFunction(Fini, 0);

	// Never returns
	PIN_StartProgram();
	//  PIN_StartProgramProbe();  For IMG processing probing.

	return 0;
}

/*
typedef void* (*malloc_funptr_t)(size_t size);
malloc_funptr_t app_malloc;

VOID * malloc_wrap(size_t size)
{
	void *ptr = app_malloc(size);
	fprintf(trace, "Malloc %ld return %p\n", size, ptr);
	return ptr;
}

VOID Image(IMG img, VOID *v)
{
	RTN mallocRtn = RTN_FindByName(img, "malloc");
	if (RTN_Valid(mallocRtn))
	{
		app_malloc=(malloc_funptr_t)RTN_ReplaceProbed(mallocRtn,AFUNPTR(malloc_wrap));
	}
}

 */

