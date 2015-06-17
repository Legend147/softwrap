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
  File format for benchmark output:
  First run the output through the script ./formatoutput
0x811a43340
0x811a433c0
0x80be1b040 4096
0x8118ec040 4096
0x8094d2040 4096
...


File format for trace output:
transaction_type  address  agent(processor_socket#)  time in memory clocks since previous

MEMWR64B 4ee049600 0 29
MEMRD64B 4ee049640 0 6075
MEMWR64B 4ee049640 0 29
MEMRD64B 4ee049680 0 6075
MEMWR64B 4ee049680 0 29

*/

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <iomanip>
#include <set>
#include <vector>
#include "tracer.h"

FILE *scm, *trace;
unsigned long programStart, programEnd;
unsigned long numDramLoads = 0;
unsigned long numDramStores = 0;
unsigned long numScmLoads = 0;
unsigned long numScmStores = 0;
unsigned long numScmLogLoads = 0;
unsigned long numScmLogStores = 0;
unsigned long totalMemoryCycles = 0;
int logPages = 0;
int numScmSegs = 0;

struct tracerecord
{
	unsigned long address;
	int delay;
	int type;
};

struct plocs
{
  unsigned long start;
  unsigned long size;
  bool log;
};


struct plocs scmLocs[1<<18];

int getSCMIndex(unsigned long addr)
{
	for (int i = 0; i < numScmSegs; i++)
	{
		if ((addr >= scmLocs[i].start) && (addr  <= (scmLocs[i].start + scmLocs[i].size)))
			return i;
	}
	return -1;
}

bool isSCMLog(int index)
{
	if (scmLocs[index].log == true)
		return true;
	return false;
}

void parseSCMAddresses()
{

	fscanf(scm, "%lx", &programStart);
	fscanf(scm, "%lx", &programEnd);

	programStart = physicalToActual(programStart);
	programEnd = physicalToActual(programEnd);

	unsigned long pa;
	int size;
	while (fscanf(scm, "%lx %d", &pa, &size) != EOF)
	{
		scmLocs[numScmSegs].start = physicalToActual(pa);
		scmLocs[numScmSegs].size = size;
		scmLocs[numScmSegs].log = false;

		if (logPages > 0)
		{
			if (numScmSegs < logPages)
			{
				scmLocs[numScmSegs].log = true;
			}
		}
		//printf("%lx %d\n", pa, size);
		numScmSegs++;
	}
	printf("ProgramStart=%lx \tProgramEnd=%lx \tnumScmSegments=%d\n",
			programStart, programEnd, numScmSegs);
	fflush(stdout);

	fclose(scm);
}

void parseTrace(int startTokens)
{
	int started = 0;
	unsigned long totalTraces = 0;
	unsigned long totalRun = 0;

	assert(startTokens > 0);

	//  MEMWR64B 4ee049600 0 29
	char rw[16];
	unsigned long addr;
	int na, memCycles;
	while (fscanf(trace, "%s %lx %d %d", rw, &addr, &na, &memCycles) > 0)
	{
		totalTraces++;
		if (!started)
		{
			//  Don't do anything until the program starts.
			if ((addr == programStart) && (--startTokens == 0))
			{
				printf("Program started at trace %ld.\n", totalTraces);
				started = 1;
			}
		}
		else
		{
			if (addr == programEnd)
			{
				printf("Program ended at trace %ld.\n", totalTraces);
				break;
			}

			totalRun++;
			totalMemoryCycles += memCycles;

			//  Determine Load or Store
			//  of DRAM or SCM (log or regular access)
			int scmindex = getSCMIndex(addr);
			if (strcmp(rw, "MEMWR64B") == 0)
			{
				if (scmindex >= 0)
				{
					if (isSCMLog(scmindex))
						numScmLogStores++;
					else
						numScmStores++;
				}
				else
					numDramStores++;
			}
			else if (strcmp(rw, "MEMRD64B") == 0)
			{
				if (scmindex >= 0)
				{
					if (isSCMLog(scmindex))
						numScmLogLoads++;
					else
						numScmLoads++;
				}
				else
					numDramLoads++;
			}
			else
			{
				assert(0);
			}
		}
	}
	printf("TOTALS totalTraces= \t%ld \ttotalRun= \t%ld \t", totalTraces, totalRun);
	printf("numDramLoads= \t%ld \tnumDramStores= \t%ld \t", numDramLoads, numDramStores);
	printf("numScmLoads= \t%ld \tnumScmStores= \t%ld \t", numScmLoads, numScmStores);
	if (logPages > 0)
	{
		printf("numScmLogLoads= \t%ld \tnumScmLogStores= \t%ld \tTotalScmLoads= \t%ld \ttotalScmStores= \t%ld \t",
				numScmLogLoads, numScmLogStores, numScmLoads + numScmLogLoads, numScmStores + numScmLogStores);

	}
	printf("totMemCycles= \t%ld\n", totalMemoryCycles);
}

int main(int argc, char *argv[])
{
	if ((argc < 4) || (argc > 5))
	{
		std::cerr << "Usage: " << argv[0] << " numStartTokens benchmarkOutput traceFile [logpages]" << std::endl;
		std::cerr << "\tThis tool analyzes the output of a physical memory trace from a wrapped program." << std::endl;
		return -1;
	}
	int numtokens = atoi(argv[1]);
	if (numtokens <= 0)
	{
		std::cerr << "numStartTokens must be >= 1" << std::endl;
		return -1;
	}
	scm = fopen(argv[2], "r");
	trace = fopen(argv[3], "r");

	if (!scm || !trace)
	{
		std::cerr <<  "Error opening file!" << std::endl;
		return -1;
	}
	if (argc >= 5)
	{
		logPages = atoi(argv[4]);
		std::cout << "Log pages = " << logPages << std::endl;
		if (logPages <= 0)
		{
			std::cerr << "Log pages should be positive!" << std::endl;
		}
	}

	parseSCMAddresses();
	parseTrace(numtokens);

	return 0;
}



