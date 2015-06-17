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
#include <sys/time.h>
#include "WrapImpl.h"
#include "memtool.h"
#include "WrapImplUndoLog.h"
#include "WrapImplSoftware.h"
#include "WrapLogManager.h"
#include "WrapLogManagerBlock.h"


//MemCheck, NoSCM, NoGuarantee, NoAtomicity, UndoLog, RedoLog, Wrap_Software, Wrap_Hardware, Wrap_Memory
WrapImplType WrapImpl::m_wrapImplType = MemCheck;
//  Options
int WrapImpl::m_options = 0;
//  Statistics
int WrapImpl::totWrapOpen = 0;
int WrapImpl::totWrapClose = 0;
int WrapImpl::totWrapRead = 0;
int WrapImpl::totWrapWrite = 0;
int WrapImpl::totWrapLoad = 0;
int WrapImpl::totWrapStore = 0;
int WrapImpl::maxWrapDepth = 0;
int WrapImpl::totUniqueWraps = 0;
long WrapImpl::totBytesWrite = 0;
long WrapImpl::totBytesRead = 0;
int WrapImpl::maxBytesRead = 0;
int WrapImpl::maxBytesWrite = 0;
long WrapImpl::totalWrapTime;
//int WrapImpl::m_maxLogSize = 1024*8*4;


int hardwareWrapOpen()
{
	//  Pin shall override this function.
	assert(0);
	return 0;
}

int hardwareWrapClose(int wrapToken)
{
	//  Pin shall override this function.
	assert(0);
	return 0;
}

void *hardwareWrapRead(void *ptr, int size, int w)
{
	//  Pin shall override this function.
	assert(0);
	return NULL;
}

void hardwareWrapWrite(void *ptr, void *src, int size, int w)
{
	//  Pin shall override this function.
	assert(0);
}

uint64_t hardwareWrapLoad(void *ptr, int size, int w)
{
	//  Pin shall override this function.
	assert(0);
	return (uint64_t)NULL;
}

void hardwareWrapStore(void *ptr, uint64_t value, int size, int w)
{
	//  Pin shall override this function.
	assert(0);
}

int WrapImpl::wrapImplOpen()
{
	static int i = 1;
	if ((m_wrapImplType == NoSCM) || (m_wrapImplType == NoGuarantee) ||
			(m_wrapImplType == NoAtomicity) || (m_wrapImplType == MemCheck))
	{
		//  TODO  for multi-threaded add tls inc num open.
		return i++;
	}
	if (m_wrapImplType == Wrap_Hardware)
		return hardwareWrapOpen();

	//  Other implementations should have overridden this method.
	assert(0);
	return 0;
}


int WrapImpl::wrapImplClose(int wrapToken)
{

	if (m_wrapImplType == NoAtomicity)
	{
		//  TODO multithreaded add tls inc num open, for now just check numopen==numclose
		if ((m_options == 0) && (totWrapOpen == totWrapClose))
			p_msync();
		else if (m_options == 1)
			p_msync();
		return 1;
	}
	if ((m_wrapImplType == NoSCM) || (m_wrapImplType == NoGuarantee) || (m_wrapImplType == MemCheck))
		return 1;
	if (m_wrapImplType == Wrap_Hardware)
		return hardwareWrapClose(wrapToken);

	//  Other implementations should have overridden this method.
	assert(0);
	return 0;
}

const char *WrapImpl::getAliasDetails()
{
	return "";
}

void WrapImpl::wrapImplWrite(void *ptr, void *src, int size, WRAPTOKEN w)
{
	if (m_wrapImplType == Wrap_Hardware)
		return hardwareWrapWrite(ptr, src, size, w);

	//printf("wrapWrite memcpy %p %p %d\n", ptr, src, size);

	if (m_wrapImplType == NoAtomicity)
	{
		ntstore(ptr, src, size);
	}
	else if (ptr != src)
		memcpy(ptr, src, size);

}


//  TODO!!  Add to TLS list.  On wrap closed free the tls list!!!
char itemp[1024];
void *WrapImpl::wrapImplRead(void *ptr, int size, WRAPTOKEN w)
{
	if (m_wrapImplType == Wrap_Hardware)
		return hardwareWrapRead(ptr, size, w);

	//printf("ptr=%p i=%p\n", ptr, &i);
	if (m_wrapImplType == MemCheck)
	{
		void *p = &itemp;
		memcpy(p, ptr, size);
		return p;
	}
	return ptr;
}


void WrapImpl::wrapImplStore64(void *ptr, uint64_t value, WRAPTOKEN w)
{
	if (m_wrapImplType == Wrap_Hardware)
		return hardwareWrapStore(ptr, value, 8, w);

	//printf("wrapWrite memcpy %p %p %d\n", ptr, src, size);

	if (m_wrapImplType == NoAtomicity)
	{
		ntstore(ptr, &value, 8);
		//p_msync();
	}
	else
		*(uint64_t*)ptr = value;
}

void WrapImpl::wrapImplStore32(void *ptr, uint32_t value, WRAPTOKEN w)
{
	if (m_wrapImplType == Wrap_Hardware)
		return hardwareWrapStore(ptr, value, 4, w);

	//printf("wrapWrite memcpy %p %p %d\n", ptr, src, size);

	if (m_wrapImplType == NoAtomicity)
	{
		ntstore(ptr, &value, 4);
		//p_msync();
	}
	else
		*(uint32_t*)ptr = value;
}

uint64_t WrapImpl::wrapImplLoad64(void *ptr, WRAPTOKEN w)
{
	if (m_wrapImplType == Wrap_Hardware)
		return hardwareWrapLoad(ptr, 8, w);

	return *(uint64_t*)ptr;
}
uint32_t WrapImpl::wrapImplLoad32(void *ptr, WRAPTOKEN w)
{
	if (m_wrapImplType == Wrap_Hardware)
		return hardwareWrapLoad(ptr, 4, w);

	return *(uint32_t*)ptr;
}


/*
WrapLogManager *WrapImpl::getWrapLogManager()
{
	if (m_logManager == NULL)
		m_logManager = new WrapLogManager(m_maxLogSize);
	return m_logManager;
}
 */

long _startTime = 0;
long getTime()
{
	struct timeval t;
	//  Get start of first wrap.
	if (gettimeofday(&t, NULL) < 0)
	{
		perror("Error getting the time of day");
		abort();
	}
	return (t.tv_sec * 1000000) + t.tv_usec;
}
void tic()
{
	_startTime = getTime();
}
double toc()
{
	/*  Calculate the latency in us.  */
	return getTime() - _startTime;
}

WrapImpl::WrapImpl()
{
	//getWrapLogManager();
	_logManager = new WrapLogManagerBlock();
}

long WrapImpl::elapsedTime()
{
	return getTime() - totalWrapTime;
}

WrapImpl::~WrapImpl()
{
	printf("destructor\n");
	printf("totalWrapTime= \t%ld\n", elapsedTime());
	delete _logManager;
}

WrapImpl *WrapImpl::getWrapImpl(WrapImplType t)
{
	m_wrapImplType = t;
	FILE *stream = stdout;
	WrapImpl *w = NULL;

	if ((t == NoSCM) || (t == NoGuarantee) || (t == NoAtomicity) || (t == MemCheck) || (t == Wrap_Hardware))
	{
		if (t == MemCheck)
			fprintf(stream, "MemCheck\n");
		else if (t == NoAtomicity)
			fprintf(stream, "NoAtomicity\n");
		else if (t == NoSCM)
			fprintf(stream, "NoSCM\n");
		else if (t == NoGuarantee)
			fprintf(stream, "NoGuarantee\n");
		else if (t == Wrap_Hardware)
			fprintf(stream, "Wrap_Hardware\n");
		else
			assert(0);
		w = new WrapImpl();
	}
	if (t == UndoLog)
	{
		fprintf(stream, "UndoLog\n");
		w = new WrapImplUndoLog();
	}
	if (t == Wrap_Software)
	{
		fprintf(stream, "Software\n");
		w = new WrapImplSoftware();
	}

	tic();
	totalWrapTime = getTime();
	return w;
}

/*
WrapLogger *WrapImpl::getWrapLog(WRAPTOKEN w)
{
	return getWrapLogManager()->getWrapLog(w);
}

void WrapImpl::restoreLogs()
{
	getWrapLogManager()->restoreAllLogs();
}
 */


void WrapImpl::getOptionString(char *c)
{
	if (m_wrapImplType == NoAtomicity)
		sprintf(c, "0 (default) - pcommit only on wrapClose of depth zero\n1 - pcommit on every wrapClose\n"  \
				"2 - no pcommits at all\n");
	else
		sprintf(c, "None");  //  No options for other types.
}

void WrapImpl::startStatistics()
{
	totWrapOpen = totWrapClose = maxWrapDepth = totUniqueWraps = 0;
	totWrapRead = totWrapWrite = totBytesRead = totBytesWrite = maxBytesRead = maxBytesWrite = 0;
}

void WrapImpl::getStatistics(char *c)
{
	sprintf(c, "Type= %d \tOptions= %d \ttotWrapOpen= %d \ttotWrapClose= %d \t" \
			"maxWrapDepth= %d \ttotUniqueWraps= %d \ttotWrapRead= %d \ttotWrapWrite= %d \t" \
			"totBytesRead= %ld \ttotBytesWrite= %ld \tmaxBytesRead= %d \tmaxBytesWrite= %d",
			m_wrapImplType, m_options, totWrapOpen, totWrapClose, maxWrapDepth, totUniqueWraps,
			totWrapRead, totWrapWrite, totBytesRead, totBytesWrite, maxBytesRead, maxBytesWrite);
}
