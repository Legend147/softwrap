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
 *  This file contains an ISA-portable PIN tool for tracing memory accesses to make
 *  sure that they are wrapped for atomic durability to SCM.
 */

#include <stdio.h>
#include <iomanip>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <map>
#include <list>
#include <vector>
#include <iostream>
#include "emmintrin.h"
#include "pin.H"
#include "pmem.h"
#include "wrap.h"
#include "memtool.h"
#include "debug.h"

using std::cout;

#define MAXTHREADS 128
int *numWrapsOpen;

UINT64 dramReads = 0;
UINT64 dramWrites = 0;
UINT64 scmReads = 0;
UINT64 scmWrites = 0;
UINT32 nNtstores = 0;
UINT32 nPCommits = 0;
UINT32 nClflushes = 0;
UINT64 nInstructions = 0;

int nThreads = 0;
int nMaxThreads = 0;
int totalThreads = 0;

#define GetLock PIN_GetLock
#define ReleaseLock PIN_ReleaseLock
#define InitLock PIN_InitLock

PIN_LOCK pmemlock;
PIN_LOCK tracelock;

/**
 * checkwrapinit  no errors are printed before main is called also
 * 					todo this will also check log inits that don't use wrapstream
 * assertwrap die on error
 * tracewrap trace wrap api usage
 * routineonly  only print the summary per routine errors
 * allroutines  prints usage in all routines
 *
 * compare only function names in stack trace (instead of address/linenumber call trace)
 *
 */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
		"o", "wrapAnalysis.out", "Output file for SCM Wrap Analysis");
//  TODO
//ofstream OutputFile;
#define OutputFile cout

//  Command line options for the pin wrap tool.
KNOB<BOOL> KnobTraceWrap(KNOB_MODE_WRITEONCE, "pintool", "tracewrap",
		"0", "trace wrap open and close function calls");

KNOB<BOOL> KnobCheckInitWrap(KNOB_MODE_WRITEONCE, "pintool", "checkwrapinit",
		"0", "check wrap initialization routines before main calls");

KNOB<BOOL> KnobByFile(KNOB_MODE_WRITEONCE, "pintool", "sortfile",
		"1", "sort output by file");

KNOB<BOOL> KnobDisplayCalls(KNOB_MODE_WRITEONCE, "pintool", "displaycalls",
		"0", "display call stacks");

KNOB<BOOL> KnobTraceErrors(KNOB_MODE_WRITEONCE, "pintool", "traceerrors",
		"0", "trace and output on every persistent unwrapped error");

KNOB<BOOL> KnobWrapErrors(KNOB_MODE_WRITEONCE, "pintool", "wraperrors",
		"1", "show no wrap open errors");

//KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o",
//	"pinwraptrace.out", "specify memory trace file");
/*
KNOB<BOOL>   KnobCheckWrap(KNOB_MODE_WRITEONCE, "pintool", "checkwrap",
		"0", "checks persistent accesses are wrapped");
KNOB<BOOL>   KnobAssertWrap(KNOB_MODE_WRITEONCE, "pintool", "assertwrap",
		"0", "assert that persistent accesses are wrapped");
KNOB<string> KnobDramSim(KNOB_MODE_WRITEONCE, "pintool", "dramsim",
		"", "specify DramSim2 Configuration File");
KNOB<string> KnobNVMain(KNOB_MODE_WRITEONCE, "pintool", "nvmain",
		"", "specify slowdown factor (if zero then all in dram) for dramsim equivalent nvm or NVMain Configuration File");
KNOB<BOOL>   KnobCompStats(KNOB_MODE_WRITEONCE, "pintool", "compstats",
		"0", "print statistics for each of the components");
KNOB<BOOL>   KnobTicToc(KNOB_MODE_WRITEONCE, "pintool", "tictoc",
		"1", "overload tic() and toc() functions to start timer and get time elapsed to be simulated cycles");
 */

int *inWrapFlags;
#define IN_WRAP_READ_FLAG  	1
#define IN_WRAP_WRITE_FLAG 	2
#define IN_PMALLOC_FLAG 	4
#define IN_WRAP_LOG_INIT 	8
int isInMain = 0;

#define atomicIncrement(var, val)	__atomic_add_fetch(&(var), val, __ATOMIC_SEQ_CST)

void setInWrapRead(int flag)
{
	if (flag)
		inWrapFlags[PIN_ThreadId()] |= IN_WRAP_READ_FLAG;
	else
		inWrapFlags[PIN_ThreadId()] ^= IN_WRAP_READ_FLAG;
}
void setInWrapWrite(int flag)
{
	if (flag)
		inWrapFlags[PIN_ThreadId()] |= IN_WRAP_WRITE_FLAG;
	else
		inWrapFlags[PIN_ThreadId()] ^= IN_WRAP_WRITE_FLAG;
}

void setInWrapLogInit(int flag)
{
	if (flag)
		inWrapFlags[PIN_ThreadId()] |= IN_WRAP_LOG_INIT;
	else
		inWrapFlags[PIN_ThreadId()] ^= IN_WRAP_LOG_INIT;
}

bool isInWrapRead()
{
	//  @TODO check if wrapOpen
	//return (numWrapsOpen[PIN_ThreadId()] > 0) && ((inWrapFlags[PIN_ThreadId()]&IN_WRAP_READ_FLAG) > 0);
	return ((inWrapFlags[PIN_ThreadId()]&IN_WRAP_READ_FLAG) > 0);
}

bool isInWrapWrite()
{
	//  @TODO check if wrapOpen
	//return (numWrapsOpen[PIN_ThreadId()] > 0) && ((inWrapFlags[PIN_ThreadId()]&IN_WRAP_WRITE_FLAG) > 0);
	return ((inWrapFlags[PIN_ThreadId()]&IN_WRAP_WRITE_FLAG) > 0);
}


bool isWrapOpen()
{
	return ((numWrapsOpen[PIN_ThreadId()]) > 0);
}

bool isInPMalloc()
{
	return ((inWrapFlags[PIN_ThreadId()]&IN_PMALLOC_FLAG) > 0);
}

bool isInWrapLogInit()
{
	return ((inWrapFlags[PIN_ThreadId()]&IN_WRAP_LOG_INIT) > 0);
}

bool continueCheck()
{
	if (isInMain)
		return true;
	return KnobCheckInitWrap.Value();
}

//  TODO make this per thread!!
typedef struct ReadWriteBytesType
{
	ADDRINT _address;
	uint64_t size;
	int read;
} ReadWriteBytes;
std::list<ReadWriteBytes> ReadWriteBytesList;

//  This function is called on a wrapRead(addr, size) to mark the bytes as ok and not to spawn errors.
//  It adds to a list which is cleared on wrapClose calls.
//  Also creates unsafe entries on conflicts of wrapWrite(addr, size, val).
//  TODO get/pass oer threadid
void setWrapReadWriteBytes(ADDRINT addr, uint32_t size, int read)
{
	ReadWriteBytes b;
	b._address = addr;
	b.size = size;
	b.read = read;
	//cout << "setWrapReadWriteBytes " << addr << " " << size << endl;
	ReadWriteBytesList.push_back(b);
}

int wrapReadBytesSafe(ADDRINT addr, int size)
{
	int safe = 0;
	for (std::list<ReadWriteBytes>::iterator it = ReadWriteBytesList.begin(); it != ReadWriteBytesList.end(); it++)
	{
		ReadWriteBytes b = *it;
		if ((addr >= b._address) && (addr <= (b._address+b.size)) && ((addr+size) <= (b._address+b.size)))
		{
			//  Covered by a read or write.
			safe = b.read;
		}
	}
	return safe;
}

VOID markWrapOpen(int returnValue, int threadID)
{
	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "markWrapOpen(returnValue=%d, threadID=%d)\n", returnValue, threadID);
		//LOG(buf);
		printf("%s", buf);
	}
	numWrapsOpen[threadID]++;
}

VOID markWrapClose(int wrapToken, int threadID)
{
	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "markWrapClose(wrapToken=%d, threadID=%d)\n", wrapToken, threadID);
		//LOG(buf);
		printf("%s", buf);
	}
	numWrapsOpen[threadID]--;
	assert(numWrapsOpen[threadID] >= 0);
	if (numWrapsOpen[threadID] == 0)
	{
		//  Clear the read/write list.
		//  TODO per thread.
		ReadWriteBytesList.clear();
	}
}

void *PMalloc(size_t size)
{
	printf("in pin pmalloc!\n");

	inWrapFlags[PIN_ThreadId()] |= IN_PMALLOC_FLAG;
	void *v = pmalloc(size);
	inWrapFlags[PIN_ThreadId()] ^= IN_PMALLOC_FLAG;
	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "pmalloc(%ld) = %p\n", size, v);
		printf("%s", buf);
		//LOG(buf);
	}
	GetLock(&pmemlock, PIN_ThreadId());
	allocatedPMem(v, size);
	ReleaseLock(&pmemlock);

	printf("returned\n");
	return v;
}

void PFree(void *p)
{
	printf("in pin pfree!\n");
	pfree(p);
	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "pfree(%p)\n", p);
		printf("%s", buf);
		//LOG(buf);
	}
	GetLock(&pmemlock, PIN_ThreadId());
	freedPMem(p);
	ReleaseLock(&pmemlock);
}
void *PCalloc(size_t nmemb, size_t size)
{
	inWrapFlags[PIN_ThreadId()] |= IN_PMALLOC_FLAG;
	void *v = pcalloc(nmemb, size);
	inWrapFlags[PIN_ThreadId()] ^= IN_PMALLOC_FLAG;

	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "pcalloc(%lu, %lu)=%p\n", nmemb, size, v);
		printf("%s", buf);
		//LOG(buf);
	}
	GetLock(&pmemlock, PIN_ThreadId());
	allocatedPMem(v, size);
	ReleaseLock(&pmemlock);
	return v;
}
void *PRealloc(void *ptr, size_t size)
{
	printf("inprealloc\n\n");
	inWrapFlags[PIN_ThreadId()] |= IN_PMALLOC_FLAG;
	void *v = prealloc(ptr, size);
	inWrapFlags[PIN_ThreadId()] ^= IN_PMALLOC_FLAG;

	if (KnobTraceWrap.Value())
	{
		char buf[128];
		sprintf(buf, "prealloc(%p, %lu)=%p\n", ptr, size, v);
		printf("%s", buf);
		//LOG(buf);
	}
	GetLock(&pmemlock, PIN_ThreadId());
	reallocedPMem(v, size, ptr);
	ReleaseLock(&pmemlock);
	return v;
}

int checkSymbolName(RTN rtn, const char *name)
{
	if (PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY) == name)
		return 1;
	return 0;
}


enum WrapErrorType
{
	UnWrappedRead, UnWrappedWrite, NoWrapOpen
};

typedef struct WrapErrorStruct
{
	int lineNumber;
	int column;
	int count;
	WrapErrorType warningType;
} WrapError;

struct CallEntryStruct;
typedef std::vector<CallEntryStruct>	CallStack;

// Holds information and errors for a single procedure
typedef struct ProcedureStruct
{
	ADDRINT _address;
	RTN _rtn;
	UINT64 _rtnCount;
	UINT64 _icount;

	int lineNumber;
	string fileName;
	string functionName;
	string imageName;
	int nWrapErrors;

	std::list<CallStack> callStacks;
	std::vector<WrapError> wrapErrors;
} Procedure;

// Linked list of instruction counts for each routine
typedef std::list<Procedure*> ProcedurePtrList;
ProcedurePtrList ProcedureList;

typedef struct CallEntryStruct
{
	int lineNumber;
	Procedure *proc;
	ADDRINT _address;
} CallEntry;

//  TODO change to calltrace or what's the standard?
typedef std::vector<CallEntry> CallStack;
CallStack ThreadCallStacks[MAXTHREADS];

//  Compare procedures by file name.
bool compare_procedures_byfile (const Procedure *first, const Procedure *second)
{
	return strcmp(first->fileName.c_str(), second->fileName.c_str()) < 0;
}


void printStatistics()
{
	//	started = false;

	printf("Dram iReads: %lu \tiWrites: %lu \tSCM iReads: %lu \tiWrites: %lu \t" \
			"NtStores: %u \tPCommits: %u \tClFlushes: %u \tInst: %lu\n",
			dramReads, dramWrites, scmReads, scmWrites,
			nNtstores, nPCommits, nClflushes, nInstructions);
}

int equalCallStacks(CallStack &a, CallStack &b)
{
	if (a.size() != b.size())
		return 0;
	CallEntry ea, eb;
	for (unsigned int i = 0; i < a.size(); i++)
	{
		ea = a[i];
		eb = b[i];
		if (ea.proc != eb.proc)
			return 0;
		//  TODO option to compare line number call stack default no.
		//  TODO continued.... don't compare line numbers option or only compare line numbers, etc.
		//if (ea.lineNumber != eb.lineNumber)
		//return 0;
	}
	return 1;
}

int followedNewCallStack(Procedure *p)
{
	if (p == NULL)
		return 1;

	for (std::list<CallStack>::iterator stack_it = p->callStacks.begin(); stack_it != p->callStacks.end(); stack_it++)
	{
		if (equalCallStacks(*stack_it, ThreadCallStacks[PIN_ThreadId()]))
			return 0;
	}
	return 1;
}

void outputLocation(const char *fname, INT32 line, INT32 column, WrapErrorType t)
{
	const char *str;
	if (t == UnWrappedRead)
		str="read not under wrap bounds.";
	else if (t == UnWrappedWrite)
		str = "write not under wrap bounds.";
	else if (t == NoWrapOpen)
		str = "wrap operation has no wrap open.";
	else
		str = "SCM TOOL ERROR";

	fflush(stdout);
	fprintf(stdout, "In file %s line %d, persistent %s\n",
			//		str, (void*)instructionAddress,
			//		fileName.c_str(), line, column);
			fname, line, str);
	fflush(stdout);
}

void outputWrapError(Procedure *p, WrapError e)
{
	outputLocation(p->fileName.c_str(), e.lineNumber, e.column, e.warningType);
}

int updateNotifyWrapErrors(Procedure *p, WrapErrorType t, int lineNumber, int column)
{
	//  TODO make this thread safe.

	for (unsigned int i = 0; i < p->wrapErrors.size(); i++)
	{
		if ((p->wrapErrors[i].lineNumber == lineNumber) && (p->wrapErrors[i].warningType == t))
		{
			atomicIncrement(p->wrapErrors[i].count, 1);
			return 0;
		}
	}
	//  Wrap Error not found so add to vector.
	WrapError e;
	e.lineNumber = lineNumber;
	e.column = column;
	e.warningType = t;
	e.count = 1;
	p->wrapErrors.push_back(e);
	if (KnobTraceErrors.Value())
		outputWrapError(p, e);
	return 1;
}

void dumpDebugInfo(ADDRINT instructionAddress, WrapErrorType t, Procedure *p)
{
	INT32 column, line;
	string fileName;

	if (p == NULL)
		return;

	PIN_LockClient();
	PIN_GetSourceLocation(instructionAddress, &column, &line, &fileName);
	PIN_UnlockClient();

	//GetLock(&tracelock, PIN_ThreadId());

	p->nWrapErrors++;

	updateNotifyWrapErrors(p, t, line, column);

	//  Compare call stacks.
	if (followedNewCallStack(p))
	{
		//  Add the current call stack.
		CallStack s = ThreadCallStacks[PIN_ThreadId()];
		p->callStacks.push_back(s);
	}
	else
	{
		//  TODO if trace all
		//  outputLocation(fileName.c_str(), line, column, str);
	}
	//ReleaseLock(&tracelock);
}


void checkMemRead(void *addr, ADDRINT instructionAddress, int size, Procedure *p)
{
	//int tid = PIN_ThreadId();
	if (isInWrapLogInit())
		return;
	//  TODO add isinpmemsize argument
	if (!isInWrapRead() && isInPMem(addr))
	{
		//char buf[256];
		//sprintf(buf, "Error - Persistent memory read access out of a wrapRead at address %p for thread %d\n", addr, tid);
		//LOG(buf);
		if (!wrapReadBytesSafe((ADDRINT)addr, size))
			dumpDebugInfo(instructionAddress, UnWrappedRead, p);
	}
}

void checkMemWrite(void *addr, ADDRINT instructionAddress, Procedure *p)
{
	//int tid = PIN_ThreadId();
	if (isInWrapLogInit())
		return;
	//  TODO add size to this call and isinpmem call
	if (isInPMem(addr) && !isInPMalloc())
	{
		if (!isInWrapWrite())
		{
			//char buf[256];
			//sprintf(buf, "Error - Persistent memory write access out of a wrap at address %p for thread %d\n", addr, tid);
			//LOG(buf);
			dumpDebugInfo(instructionAddress, UnWrappedWrite, p);
		}
		else
		{
			//  We're in wrap write, so make sure wrap is open.
			//if (!isWrapOpen())
			{
				//dumpDebugInfo(instructionAddress, NoWrapOpen, p);
			}
		}
	}
}

VOID checkWrapOpen(Procedure *p, ADDRINT instructionAddress)
{
	if (!continueCheck())
		return;

	if (!KnobWrapErrors.Value())
		return;

	if (!isWrapOpen())
	{
		dumpDebugInfo(instructionAddress, NoWrapOpen, p);
	}
}



VOID onMemRead(VOID * ip, VOID * addr, int size, Procedure *p)
{
	if (!continueCheck())
		return;

	checkMemRead(addr, (ADDRINT)ip, size, p);
	//  TODO add size to isinpmem
	if (isInPMem((void*)addr))
	{
		atomicIncrement(scmReads, 1);
	}
	else
	{
		atomicIncrement(dramReads, 1);
	}
}

VOID onMemWrite(VOID * ip, VOID * addr, int size, Procedure *p)
{
	if (!continueCheck())
		return;

	checkMemWrite(addr, (ADDRINT)ip, p);
	if (isInPMem((void *)addr))
	{
		atomicIncrement(scmWrites, 1);
	}
	else
	{
		atomicIncrement(dramWrites, 1);
	}
}

void onStreamingStore(int *p, int a)
{
	if (!continueCheck())
		return;

	//  Increment the number of ntstores
	atomicIncrement(nNtstores, 1);
}

void onClflush(VOID *p)
{
	if (!continueCheck())
		return;

	atomicIncrement(nClflushes, 1);
}

void onPCommit()
{
	if (!continueCheck())
		return;

	atomicIncrement(nPCommits, 1);
}


LOCALFUN VOID InsComplete(ADDRINT addr)
{
	atomicIncrement(nInstructions, 1);
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
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
					(AFUNPTR)onMemRead, IARG_INST_PTR, IARG_MEMORYOP_EA, memOp, IARG_UINT32, refSize,
					IARG_PTR, v,
					IARG_END);
		}
		// Note that in some architectures a single memory operand can be
		// both read and written (for instance incl (%eax) on IA-32)
		// In that case we instrument it once for read and once for write.
		if (INS_MemoryOperandIsWritten(ins, memOp))
		{
			INS_InsertPredicatedCall(
					ins, IPOINT_BEFORE,
					(AFUNPTR)onMemWrite, IARG_INST_PTR, IARG_MEMORYOP_EA, memOp, IARG_UINT32, refSize,
					IARG_PTR, v,
					IARG_END);
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

// This function is called before every instruction is executed
VOID incrementCount(UINT64 * counter)
{
	atomicIncrement(*counter, 1);
}

VOID enterProcedure(Procedure *p, ADDRINT instructionAddress)
{
	if ((!isInMain) || (p->functionName.c_str()[0] == '.'))
		return;

	INT32 column, line;
	string fileName;
	PIN_LockClient();
	PIN_GetSourceLocation(instructionAddress, &column, &line, &fileName);
	PIN_UnlockClient();

	CallEntry entry;
	entry.lineNumber = line;
	entry.proc = p;
	entry._address = instructionAddress;
	ThreadCallStacks[PIN_ThreadId()].push_back(entry);

	if (isDebug())
	{
		for (unsigned int i = 0; i < ThreadCallStacks[PIN_ThreadId()].size(); i++)
			std::cout << " ";
		std::cout << "enter(" <<  p->imageName << ")" << p->functionName << endl;
	}
}

VOID exitProcedure(Procedure *rc)
{
	if ((!isInMain) || (rc->functionName.c_str()[0] == '.'))
		return;

	if (isDebug())
	{
		for (unsigned int i = 0; i < ThreadCallStacks[PIN_ThreadId()].size(); i++)
			std::cout << " ";
		std::cout << "exit " << rc->functionName << endl;
	}
	CallEntry call;
	while (ThreadCallStacks[PIN_ThreadId()].size() > 0)
	{
		call = ThreadCallStacks[PIN_ThreadId()].back();
		ThreadCallStacks[PIN_ThreadId()].pop_back();
		if (call.proc == rc)
			break;
		else
		{
			if (isDebug())
				std::cout << "SKIPPED " << call.proc->functionName << endl;
		}
	}
}

const char * StripPath(const char * path)
{
	const char * file = strrchr(path,'/');
	if (file)
		return file+1;
	else
		return path;
}

VOID InstrumentRoutine(RTN rtn, VOID *v)
{

	// Allocate a counter for this routine
	Procedure *rc = new Procedure;

	// The RTN goes away when the image is unloaded, so save it now
	// because we need it in the fini
	rc->functionName = PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY);
	rc->imageName = StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
	rc->_address = RTN_Address(rtn);
	rc->_icount = 0;
	rc->_rtnCount = 0;
	rc->nWrapErrors = 0;
	//  Add line and file number.
	INT32 column;
	PIN_GetSourceLocation(rc->_address, &column, &(rc->lineNumber), &(rc->fileName));

	// Add to list of routines
	//rc->_next = RtnList;
	//RtnList = rc;
	ProcedureList.push_back(rc);

	RTN_Open(rtn);

	//  Insert call to flag program is in main.
	if (RTN_Name(rtn) == "main")
	{
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)incrementCount, IARG_PTR, &(isInMain), IARG_END);
	}


	//  Insert a call to increment the routine count.
	RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)incrementCount, IARG_PTR, &(rc->_rtnCount), IARG_END);
	//  Insert a call to note in routine.
	RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)enterProcedure, IARG_PTR, rc, IARG_INST_PTR, IARG_END);

	if (checkSymbolName(rtn, "wrapRead") || checkSymbolName(rtn, "wrapLoad16") ||
			checkSymbolName(rtn, "wrapLoad32") || checkSymbolName(rtn, "wrapLoad64") ||
			checkSymbolName(rtn, "wrapLoad8") || checkSymbolName(rtn, "wrapLoadByte") ||
			checkSymbolName(rtn, "wrapWrite") || checkSymbolName(rtn, "wrapStore16") ||
			checkSymbolName(rtn, "wrapStore32") || checkSymbolName(rtn, "wrapStore64") ||
			checkSymbolName(rtn, "wrapStore8") || checkSymbolName(rtn, "wrapStoreByte"))
	{
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)checkWrapOpen, IARG_PTR, rc, IARG_INST_PTR, IARG_END);
	}

	// For each instruction of the routine
	for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
	{
		// Insert a call to docount to increment the instruction counter for this rtn
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)incrementCount, IARG_PTR, &(rc->_icount), IARG_END);

		//  Check the memory references.
		Instruction(ins, rc);

		if( INS_IsRet(ins) )
		{
			// instrument each return instruction.
			// IPOINT_TAKEN_BRANCH always occurs last after IPOINT_BEFORE the return.
			INS_InsertCall( ins, IPOINT_BEFORE, (AFUNPTR)exitProcedure, IARG_PTR, rc, IARG_END);
		}
	}
	//  Insert a call to return from routine.
	//RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)exitProcedure, IARG_PTR, rc, IARG_END);


	RTN_Close(rtn);
}

struct FileInfo
{
	string fileName;
	int routines, wrapErrors, wrapLines;
};

void PrintRoutines()
{
	cout << endl << "Routine Detail:" << endl;
	string lastFile;
	FileInfo finfo;
	finfo.routines = 0;
	finfo.wrapErrors = 0;
	finfo.wrapLines = 0;
	std::list<FileInfo> fileInfoList;

	for (std::list<Procedure*>::iterator it = ProcedureList.begin(); it != ProcedureList.end(); it++)
	{
		Procedure *p = *it;

		if (p->nWrapErrors > 0)
		{
			if (KnobByFile.Value() == 1)
			{
				if (lastFile != p->fileName)
				{
					//  Add the last file info if exists.
					if (finfo.wrapLines > 0)
						fileInfoList.push_back(finfo);

					lastFile = p->fileName;
					cout << endl << "File: " << lastFile << endl;
					finfo.fileName = lastFile;
					finfo.routines = 0;
					finfo.wrapErrors = 0;
					finfo.wrapLines = 0;
				}
			}

			cout << "Function: " << p->functionName;
			if (KnobByFile.Value() == 0)
				cout << " File: " << p->fileName;
			cout << " Line: " << p->lineNumber << " WrapErrors: " << p->nWrapErrors << endl;

			finfo.routines++;
			finfo.wrapErrors += p->nWrapErrors;
			finfo.wrapLines += p->wrapErrors.size();

			for (unsigned int ei = 0; ei < p->wrapErrors.size(); ei++)
			{
				WrapError err = p->wrapErrors[ei];
				outputWrapError(p, err);
			}

			if (KnobDisplayCalls.Value() && (p->callStacks.size() > 0))
			{
				cout << " Called From:" << endl;

				for (std::list<CallStack>::iterator stack_it = p->callStacks.begin(); stack_it != p->callStacks.end(); stack_it++)
				{
					CallStack s = *stack_it;
					for (unsigned int i = 0; i < s.size(); i++)
					{
						CallEntry entry = s[i];
						if (i != 0)
							cout << " -> ";
						else
							cout << " ";
						cout << entry.proc->functionName << " (file " << entry.proc->fileName <<
								" line " << entry.lineNumber << ")";
					}
					cout << endl << endl;
				}
			}

			if (KnobByFile.Value() == 0)
				cout << endl;
		}
	}
	//  Add the last file info if exists.
	if (finfo.wrapLines > 0)
		fileInfoList.push_back(finfo);

	long totalR = 0;
	long totalE = 0;
	long totalLines = 0;
	cout << endl;
	cout << "File Summary:" << endl;
	//unsigned int fn = 40;
	//cout << setw(fn) << "fileName" << setw(12) << "routines" << setw(12) << "wrapErrors" << setw(16) << "wrapLineErrors" << endl;

	cout << setw(12) << "routines" << setw(12) << "wrapErrors" << setw(16) << "wrapLineErrors" << "    fileName" << endl;
	for (std::list<FileInfo>::iterator it = fileInfoList.begin(); it != fileInfoList.end(); it++)
	{
		//cout << setw(fn) << ((it->fileName.size() > fn)?it->fileName.substr(it->fileName.size() - fn):it->fileName);
		cout <<	setw(12) << it->routines << setw(12) <<
				it->wrapErrors << setw(16) << it->wrapLines;
		cout << "    " << it->fileName << endl;
		totalR += it->routines;
		totalE += it->wrapErrors;
		totalLines += it->wrapLines;
	}

	cout << endl;
	cout << setw(12) << totalR << setw(12) << totalE << setw(16) << totalLines << "    FileSummaryTotals for " << fileInfoList.size() << " files" << endl;
}

unsigned long PrintRoutineSummary()
{
	cout << endl << setw(35) << "Procedure" << " "
			<< setw(18) << "Image" << " "
			<< setw(18) << "Address" << " "
			<< setw(12) << "Calls" << " "
			<< setw(12) << "Instructions" << " "
			<< setw(12) << "WrapErrors" << " "
			<< setw(16) << "WrapLineErrors" << "    "
			<< "Filename" << endl;

	unsigned long total = 0;
	unsigned long totalLines = 0;
	for (std::list<Procedure*>::iterator it = ProcedureList.begin(); it != ProcedureList.end(); it++)
	{
		Procedure *p = *it;
		total += p->nWrapErrors;
		totalLines += p->wrapErrors.size();

		//  TODO option dump all procedure usage counts
		if ((p->nWrapErrors > 0) || (0))
		{
			if (p->_icount > 0)
				cout << std::setw(35) << p->functionName << " "
				<< setw(18) << p->imageName << " "
				<< setw(18) << hex << p->_address << dec <<" "
				<< setw(12) << p->_rtnCount << " "
				<< setw(12) << p->_icount << " "
				<< setw(12) << p->nWrapErrors << " "
				<< setw(16) << p->wrapErrors.size() << "    "
				<< p->fileName << endl;
		}
	}
	cout << endl;
	cout << "totalWrapErrors " << total << "    totalWrapLineErrors " << totalLines << endl;
	return total;
}


VOID Routine(RTN rtn, VOID *v)
{
	//Debug("%s\n", PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY).c_str());
	if (checkSymbolName(rtn, "WrapLogger::WrapLogger"))
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)setInWrapLogInit, IARG_UINT32, 1, IARG_END);
		RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)setInWrapLogInit, IARG_UINT32, 0, IARG_END);
		RTN_Close(rtn);
	}
	//LOG(RTN_Name(rtn) + " " + PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY) + "\n");
	else if (checkSymbolName(rtn, "wrapOpen") || checkSymbolName(rtn, "wrapStreamOpen"))
	{
		RTN_Open(rtn);
		// Insert a call at the exit point of the wrap open routine to increment wrap open count for the thread.
		RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)markWrapOpen, IARG_FUNCRET_EXITPOINT_VALUE,  IARG_THREAD_ID, IARG_END);
		RTN_Close(rtn);
	}
	else if (checkSymbolName(rtn, "wrapClose") || checkSymbolName(rtn, "wrapStreamClose"))
	{
		RTN_Open(rtn);
		// Insert a call at the entry point of the wrap close routine to decrement the thread's wrap open count.
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)markWrapClose, IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
				IARG_THREAD_ID, IARG_END);
		RTN_Close(rtn);
	}
	else if (checkSymbolName(rtn, "wrapRead") || checkSymbolName(rtn, "wrapLoad16") ||
			checkSymbolName(rtn, "wrapLoad32") || checkSymbolName(rtn, "wrapLoad64") ||
			checkSymbolName(rtn, "wrapLoad8") || checkSymbolName(rtn, "wrapLoadByte"))
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)setInWrapRead, IARG_UINT32, 1, IARG_END);
		RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)setInWrapRead, IARG_UINT32, 0, IARG_END);
		if (checkSymbolName(rtn, "wrapRead"))
		{
			//  Mark bytes as safely read.
			cout << "inserted before wrapRead" << endl;
			RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)setWrapReadWriteBytes, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_UINT32, 1, IARG_END);
		}
		RTN_Close(rtn);
	}
	else if (checkSymbolName(rtn, "wrapWrite") || checkSymbolName(rtn, "wrapStore16") ||
			checkSymbolName(rtn, "wrapStore32") || checkSymbolName(rtn, "wrapStore64")  ||
			checkSymbolName(rtn, "wrapStore8") || checkSymbolName(rtn, "wrapStoreByte"))
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)setInWrapWrite, IARG_UINT32, 1, IARG_END);
		RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)setInWrapWrite, IARG_UINT32, 0, IARG_END);
		if (checkSymbolName(rtn, "wrapWrite"))
		{
			//  If doing a wrap write, then mark bytes as not safe to read again.
			RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)setWrapReadWriteBytes, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_FUNCARG_ENTRYPOINT_VALUE, 2, IARG_UINT32, 0, IARG_END);
		}
		RTN_Close(rtn);
	}

	//  Capture persistent memory locations from the application space when notifying pin.
	else if (checkSymbolName(rtn, "pmalloc"))
	{
		cout << "found pmalloc" << endl;
		cout << RTN_Replace(rtn, (AFUNPTR)PMalloc);
		cout << endl;
	}
	//  Capture persistent memory frees from the application space when notifying pin.
	else if (checkSymbolName(rtn, "pfree"))
	{
		RTN_Replace(rtn, (AFUNPTR)PFree);
	}

	//  Capture persistent memory locations from the application space when notifying pin.
	else if (checkSymbolName(rtn, "pcalloc"))
	{
		RTN_Replace(rtn, (AFUNPTR)PCalloc);
	}
	//  Capture persistent memory locations from the application space when notifying pin.
	else if (checkSymbolName(rtn, "prealloc"))
	{
		RTN_Replace(rtn, (AFUNPTR)PRealloc);
	}

	//  Count streaming store operations from C function for now.
	//if (checkSymbolName(rtn, "ntstore"))
	//  TODO change everything to a consistent ntstore, streamstore or something....
	else if (checkSymbolName(rtn, "streamingStore"))
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)onStreamingStore,
				//IARG_ADDRINT, "streamingStore",
				IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
				IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
				IARG_END);
		RTN_Close(rtn);
	}
	//  Overload cache line flush operations.
	else if (checkSymbolName(rtn, "clflush"))
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)onClflush,
				IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
				IARG_END);
		RTN_Close(rtn);
	}
	//  Overload persistent memory sync.
	else if (checkSymbolName(rtn, "pcommit"))
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)onPCommit, IARG_END);
		RTN_Close(rtn);
	}
	else if (checkSymbolName(rtn, "strcpy"))
	{

	}

	//printf("instrumenting routine\n");
	InstrumentRoutine(rtn, NULL);
	//printf("done!\n");
}


// This routine is executed every time a thread is created.
VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
	Debug("thread begin %d\n",threadid);
	//  Count the threads.
	atomicIncrement(nThreads, 1);

	//  TODO atomics here
	if (nThreads > nMaxThreads)
		nMaxThreads = nThreads;

	atomicIncrement(totalThreads, 1);
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
	Debug("thread end %d code %d\n",threadid, code);

	//  Count the threads.
	atomicIncrement(nThreads, -1);
}

VOID Fini(INT32 code, VOID *v)
{
	printf("\n");

	printStatistics();

	//  Close the output file.
	//  TODO
	//if (TraceFile.is_open()) { TraceFile.close(); }

	//  Sort by filename.
	if (KnobByFile.Value())
		ProcedureList.sort(compare_procedures_byfile);

	unsigned long total = PrintRoutineSummary();
	if (total > 0)
		PrintRoutines();

	free(numWrapsOpen);
	free(inWrapFlags);
}


//  When an image or shared library is loaded lazily.
VOID ImageLoad(IMG img, VOID *v)
{
	OutputFile << "ImageLoad " << IMG_Name(img) << ", Image id = " << IMG_Id(img) << endl;

	/*
	for (SYM sym = IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym))
	{
		string undFuncName = PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY);
	    printf("%s SYM\n", undFuncName.c_str());
	}
	*/
	for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
	{
		for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
		{
			Routine(rtn, 0);
			//printf("%s %s\n", PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY).c_str(), RTN_Name(rtn).c_str());
		}
	}
}


//  Called on image unload, and we can't instrument it since it won't be executed.
VOID ImageUnload(IMG img, VOID *v)
{
	OutputFile << "ImageUnload " << IMG_Name(img) << endl;
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
	numWrapsOpen = (int*)calloc(MAXTHREADS, sizeof(int));
	inWrapFlags = (int *)calloc(MAXTHREADS, sizeof(int));

	// Initialize symbol processing for routine and image processing
	//PIN_InitSymbols();
	//  We need the alt so we get things like memcpy.
	PIN_InitSymbolsAlt(SYMBOL_INFO_MODE(UINT32(IFUNC_SYMBOLS) | UINT32(DEBUG_OR_EXPORT_SYMBOLS)));

	if (PIN_Init(argc, argv)) return Usage();

	//  OutputFile
	//  TODO
	//OutputFile.open(KnobOutputFile.Value().c_str());

	// Initialize the pin locks
	InitLock(&pmemlock);
	InitLock(&tracelock);

	//RTN_AddInstrumentFunction(Routine, 0);
	//IMG_AddInstrumentFunction(Image, 0);
	//INS_AddInstrumentFunction(Instruction, 0);  Now this is done in the routine section.

	// Register Analysis routines to be called when a thread begins/ends
	PIN_AddThreadStartFunction(ThreadStart, 0);
	PIN_AddThreadFiniFunction(ThreadFini, 0);

	// Register ImageLoad to be called when an image is loaded
	IMG_AddInstrumentFunction(ImageLoad, 0);
	// Register ImageUnload to be called when an image is unloaded
	IMG_AddUnloadFunction(ImageUnload, 0);

	PIN_AddFiniFunction(Fini, 0);

	// Never returns
	PIN_StartProgram();
	//  PIN_StartProgramProbe();  For IMG processing probing.

	return 0;
}



