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

#include <pthread.h>
#include <string.h>
#include "WrapLogManagerBlock.h"
#include "wrap.h"
#include "debug.h"

//pthread_rwlock_t _managerlock = PTHREAD_RWLOCK_INITIALIZER;

#define MAX_THREADS 100
#define LOGS_PER_THREAD	10

//  Initializes the wrap log manager and sets the default wrap log size and number of logs.
WrapLogManagerBlock::WrapLogManagerBlock()
{

 	char *s = getenv("WrapInitLogSize");
	if ((s != NULL) && (strcmp(s, "") != 0))
	{
		m_logSize = atoi(s);
		assert(m_logSize > 0);
	}
	else
		m_logSize = 8192;

		m_activeThreads = 0;
		m_maxLogs = LOGS_PER_THREAD * MAX_THREADS;
		m_totalSize = m_logSize*m_maxLogs;
		m_logArea = (char*)pmallocLog(m_totalSize);
		m_completed = 0;
		m_logs = (WrapLogger **)malloc(m_maxLogs * sizeof(WrapLogger*));
		for (int i = 0; i < m_maxLogs; i++)
			m_logs[i] = new WrapLogger(m_logSize, m_logArea + i*m_logSize);
};

__thread int WrapLogManagerBlock::m_currentFree = 0;
__thread int WrapLogManagerBlock::m_currentId = -1;

WrapLogManagerBlock::~WrapLogManagerBlock()
{
	restoreAllLogs();
	//  TODO free
};

WrapLogger *WrapLogManagerBlock::getWrapLog(int wrap)
{
	if (m_currentId == -1)
	{
		m_currentId = __sync_fetch_and_add(&m_activeThreads, 1);
	}
	int i = m_currentFree++;
	//return new WrapLogger(m_logSize, m_logArea + (m_currentFree*m_logSize));
	int logOffset = m_currentId * LOGS_PER_THREAD + (i % LOGS_PER_THREAD);
//	printf("%d %d\n", i, logOffset);
	return m_logs[logOffset];
	//return m_logs[i%1024];
};

void WrapLogManagerBlock::addLog(WrapLogger *log)
{
//	pthread_rwlock_wrlock(&_managerlock);
//	pthread_rwlock_unlock(&_managerlock);
//	__sync_synchronize();
}

//  TODO
void WrapLogManagerBlock::removeLog(WrapLogger *log)
{
//	pthread_rwlock_wrlock(&_managerlock);
//	pthread_rwlock_unlock(&_managerlock);
	//delete log;
}

void WrapLogManagerBlock::removeAllLogs()
{
//	pthread_rwlock_wrlock(&_managerlock);
	//_queue.clear();
//	pthread_rwlock_unlock(&_managerlock);
}

int WrapLogManagerBlock::restoreOneLog()
{
	return 0;
}

int WrapLogManagerBlock::restoreAllLogs()
{
	return 0;
};

