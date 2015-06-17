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

#include <sstream>
#include <cmath>
#include <stdlib.h>
#include "src/Interconnect.h"
#include "Interconnect/InterconnectFactory.h"
#include "src/Config.h"
#include "src/TranslationMethod.h"
#include "traceReader/TraceReaderFactory.h"
#include "src/AddressTranslator.h"
#include "Decoders/DecoderFactory.h"
#include "src/MemoryController.h"
#include "MemControl/MemoryControllerFactory.h"
#include "Endurance/EnduranceDistributionFactory.h"
#include "SimInterface/NullInterface/NullInterface.h"
#include "include/NVMHelpers.h"
#include "Utils/HookFactory.h"
#include "src/EventQueue.h"
#include "NVM/nvmain.h"
#include "debug.h"

using namespace NVM;

void cycle(int c);

unsigned long nWriteBuffer;


class NVMainRequestTracker: public NVM::NVMain
{
   public:

   	bool RequestComplete( NVMainRequest *request )
   		{
   		if (request->type == WRITE)
   		{
   			nWriteBuffer--;
   			delete request;
   		}
   			return true;
   		}
};

class NVMSim
{
public:
	NVMain *nvm;
	unsigned long nReads, nWrites;

	int getWriteBufferSize()
	{
		//printf("%lu", nWriteBuffer);
		return nWriteBuffer;
//		return nWriteBuffer;
	}

	NVMSim(const char *configFile = "FCFS-PCM.config")
	{
		nvm = new NVMainRequestTracker();

		nReads = nWrites = nWriteBuffer = 0;

		Config *config = new Config( );
		SimInterface *simInterface = new NullInterface( );
		EventQueue *mainEventQueue = new EventQueue( );


		config->Read( configFile );
		config->SetSimInterface( simInterface );
		nvm->SetEventQueue( mainEventQueue );
		simInterface->SetConfig( config );
		nvm->SetConfig( config, "defaultMemory" );
	}

	void issue(void *addr, OpType type, int threadId)
	{
		if (type == READ)
			nReads++;
		else
			nWrites++;

		NVMainRequest *request = new NVMainRequest( );

		request->address.SetPhysicalAddress( (uint64_t)addr );
		request->type = type;
		request->bulkCmd = CMD_NOP;
		request->threadId = threadId;
		//request->data = 0;
		request->status = MEM_REQUEST_INCOMPLETE;
		request->owner = nvm;

		//while( !IssueCommand( request ) )
		while (!nvm->IsIssuable(request, NULL) || (getWriteBufferSize() >= 24))
		{
			cycle(1);
			//Debug("-");
		}
		if (request->type == WRITE)
			nWriteBuffer++;

		assert(nvm->IssueCommand(request));
		//Debug("%c", (request->type==READ)?'r':'w');
		if (request->type == READ)
		{
			//  We have to wait until return to install the value.
			while (request->status != MEM_REQUEST_COMPLETE)
			{
				//printf("%d ", request->status);
				cycle(1);
			}
			//Debug("C");
			delete request;
		}
	}

	void update()
	{
		nvm->Cycle(1);

	}

	void printStats()
	{
		nvm->PrintStats( );
	}
};

