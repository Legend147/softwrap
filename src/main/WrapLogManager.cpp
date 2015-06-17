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
#include "WrapLogManager.h"
#include "wrap.h"
#include "debug.h"

pthread_rwlock_t _managerlock = PTHREAD_RWLOCK_INITIALIZER;

//  Initializes the wrap log manager and sets the default wrap log size and number of logs.
WrapLogManager::WrapLogManager()
{
	char *s = getenv("WrapInitLogSize");
	if ((s != NULL) && (strcmp(s, "") != 0))
	{
		m_size = atoi(s);
		assert(m_size > 0);
	}
	else
		m_size = 1024*1024*8*2;
};

WrapLogManager::~WrapLogManager()
{
	restoreAllLogs();
};

WrapLogger *WrapLogManager::getWrapLog(int wrap)
{
	return new WrapLogger(m_size);
};

void WrapLogManager::addLog(WrapLogger *log)
{
	pthread_rwlock_wrlock(&_managerlock);
	_queue.push_back(log);
	pthread_rwlock_unlock(&_managerlock);
}

void WrapLogManager::removeLog(WrapLogger *log)
{
	pthread_rwlock_wrlock(&_managerlock);
	_queue.remove(log);
	pthread_rwlock_unlock(&_managerlock);
	delete log;
}

void WrapLogManager::removeAllLogs()
{
	pthread_rwlock_wrlock(&_managerlock);
	while (!_queue.empty())
	{
		WrapLogger *l = _queue.front();
		if (l != NULL)
			delete l;
		_queue.pop_front();
	}
	//_queue.clear();
	pthread_rwlock_unlock(&_managerlock);
}

int WrapLogManager::restoreOneLog()
{
//  TODO LOCK THIS!
	WrapLogger *logger = _queue.front();
	if (logger == NULL)
		return 0;
	Debug("restoring log\n");
	int n = logger->restoreWrapLogEntries();
	Debug("restored %d entries\n", n);
	_queue.pop_front();
	delete logger;
	return n;
}

int WrapLogManager::restoreAllLogs()
{
	int i = 0;
	int n = 0;
	//  Walk over all logs and restore.
	while (!_queue.empty())
	{
		n += restoreOneLog();
		i++;
	}
	if (i != 0)
		printf("restored %d logs and %d total entries.\n", i, n);
	return i;
};

