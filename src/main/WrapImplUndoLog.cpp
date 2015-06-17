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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "WrapImpl.h"
#include "memtool.h"
#include "WrapImplUndoLog.h"


//  TLS
WRAP_TLS int WrapImplUndoLog::_wrapToken = 0;
WRAP_TLS int WrapImplUndoLog::_numOpen = 0;
typedef WrapLogger * wlp;
WRAP_TLS wlp WrapImplUndoLog::_log = NULL;
volatile int WOUL = 1;

int WrapImplUndoLog::wrapImplOpen()
{
	if (_numOpen == 0)
	{
		_wrapToken = __sync_add_and_fetch(&WOUL,1);
		__sync_synchronize();
		//printf("tok=%d\n", _wrapToken);
		_log = _logManager->getWrapLog(_wrapToken);
		_log->wrapLogOpen(_wrapToken);
		//  Add the log to be played since it's copy-on-write.
		_logManager->addLog(_log);
	}
	_numOpen++;
	return _wrapToken;
}


int WrapImplUndoLog::wrapImplClose(int wrapToken)
{
	//fprintf(stderr, "wrapimplundologclose\n");
	if ((_wrapToken == 0) || (_numOpen == 0))
	{
		//  We shouldn't be closing if none are open and the token should never be zero.
		assert(0);
	}
	_numOpen--;
	if (_numOpen == 0)
	{
		_wrapToken = 0;
		//  This does a pcommit after the close of the log.
		_log->wrapLogClose();
		//  Remove the log from the log to be played list.
		_logManager->removeLog(_log);
		if (m_options == 99)
		{
			printf("Log Cleanup.");
			//restoreLogs();
		}
	}

	return 1;
}

void WrapImplUndoLog::wrapImplWrite(void *ptr, void *src, int size, WRAPTOKEN w)
{
	if (ptr != src)
	{
		_log->addWrapLogEntry(ptr, ptr , size, w);
		p_msync();
		ntstore(ptr, src, size);
		//  Only one pcommit per write.
		//p_msync();
	}
}

void *WrapImplUndoLog::wrapImplRead(void *ptr, int size, WRAPTOKEN w)
{
	return ptr;
}

////
void WrapImplUndoLog::wrapImplStore64(void *ptr, uint64_t value, WRAPTOKEN w)
{
	_log->addWrapLogEntry(ptr, ptr, 8, w);
	p_msync();
	ntstore(ptr, &value, 8);
	//  Only one pcommit per write.
	//p_msync();
}

void WrapImplUndoLog::wrapImplStore32(void *ptr, uint32_t value, WRAPTOKEN w)
{
	_log->addWrapLogEntry(ptr, ptr, 4, w);
	p_msync();
	ntstore(ptr, &value, 4);
	//  Only one pcommit per write.
	//p_msync();
}

uint64_t WrapImplUndoLog::wrapImplLoad64(void *ptr, WRAPTOKEN w)
{
	return *(uint64_t*)ptr;
}
uint32_t WrapImplUndoLog::wrapImplLoad32(void *ptr, WRAPTOKEN w)
{
	return *(uint32_t*)ptr;
}

WrapImplUndoLog::WrapImplUndoLog()
{
}

WrapImplUndoLog::~WrapImplUndoLog()
{
}

void WrapImplUndoLog::getOptionString(char *c)
{
	sprintf(c, "none");
}
