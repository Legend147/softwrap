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

#include <DRAMSim.h>
#include "debug.h"

using namespace DRAMSim;

void cycle(int c);
int SHOW_SIM_OUTPUT = 0;
extern bool VIS_FILE_OUTPUT;
extern bool VERIFICATION_OUTPUT;
//bool wait = false;
extern void waitRead();
extern void onReadComplete(uint64_t address);

class DRAM
{
private:
	MultiChannelMemorySystem *mem;

public:
	unsigned long nReads, nWrites, nWriteBuffer;

	int getWriteBufferSize()
	{
		return nWriteBuffer;
	}

	/* callback functors */
	void read_complete(unsigned id, uint64_t address, uint64_t clock_cycle)
	{
		//Debug("[Callback] read complete: %d 0x%lx cycle=%lu\n", id, address, clock_cycle);
		//wait = false;
		onReadComplete(address);
	}

	void write_complete(unsigned id, uint64_t address, uint64_t clock_cycle)
	{
		//printf("[Callback] write complete: %d 0x%lx cycle=%lu\n", id, address, clock_cycle);
		nWriteBuffer--;
	}

	void power_callback(double a, double b, double c, double d)
	{
		Debug("power callback: %0.3f, %0.3f, %0.3f, %0.3f\n",a,b,c,d);
	}

/*	void alignTransactionAddress(Transaction &trans)
	{
		// zero out the low order bits which correspond to the size of a transaction
		unsigned throwAwayBits = dramsim_log2((BL*JEDEC_DATA_BUS_BITS/8));

		trans.address >>= throwAwayBits;
		trans.address <<= throwAwayBits;
	}*/

	//  TODO add config file.
	DRAM(const char *configFile = "DDR3_micron_16M_8B_x8_sg15.ini" , uint64_t cpuClockSpeed = 3000000000)
	{
		nReads = nWrites = nWriteBuffer = 0;

		TransactionCompleteCB *read_cb = new Callback<DRAM, void, unsigned, uint64_t, uint64_t>(this, &DRAM::read_complete);
		TransactionCompleteCB *write_cb = new Callback<DRAM, void, unsigned, uint64_t, uint64_t>(this, &DRAM::write_complete);

		//viz = NULL;
		/* pick a DRAM part to simulate */
		mem = getMemorySystemInstance(configFile, "dramsystem.ini", ".",
				"dramsimpin", 2048);
		mem->RegisterCallbacks(read_cb, write_cb, NULL);
		mem->setCPUClockSpeed(cpuClockSpeed);
#ifndef DEBUG
		VIS_FILE_OUTPUT = false;
		VERIFICATION_OUTPUT = false;
#endif
	}

	void update()
	{
		mem->update();
	}

	void issue(void *addr, bool isWrite, int threadId)
	{
		//Debug(".");
		if (isWrite)
			nWrites++;
		else
			nReads++;

		while( !mem->willAcceptTransaction() || (nWriteBuffer >= 6))
		{
			cycle(1);
			Debug("-");
		}
		//Debug("a");

		uint64_t u = (uint64_t)addr;
		u >>= 6;
		u <<= 6;
		if (!isWrite)
		{
			//wait = true;
		}
		else
		{
			nWriteBuffer++;
			//wait = false;
		}
		//Debug("b");
		mem->addTransaction(isWrite, u);

		//Debug("c");
		if (!isWrite)
		{
			/*
			while (wait)
			{
				cycle(1);
			}
			*/
			waitRead();
		}

	}
	void printStats()
	{
		/* get a nice summary of this epoch */
		mem->printStats(true);
	}
};

