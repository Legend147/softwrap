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

#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "AT2.h"
#include "memtool.h"
#include "unistd.h"

#define atprintf(x) //printf(x); fflush(stdout)
//bool __sync_bool_compare_and_swap (type *ptr, type oldval type newval, ...)
/*
#define CAS __sync_bool_compare_and_swap
volatile int lock = 0;
#define openSpinLock 	while(!CAS(&lock, 0, 1));
#define closeSpinLock	lock = 0;
 */

// Use pthreads
#if 1
#define Lock	pthread_mutex_lock
#define Unlock	pthread_mutex_unlock
#define Wait	pthread_cond_wait
#define Signal 	pthread_cond_signal
#else
#define Lock(x)
#define Unlock(x)
#define Wait(x,y)	usleep(1)
#define Signal(x)
#endif

#define SynchronousWait(x,y)
//Wait(x,y)

#define otherTable ((m_aliasTableHash == m_atA)?m_atB:m_atA)

void *AT2::retireThreadFunc(void *t)
{
	((AT2*)t)->retireThread();
	return NULL;
}

const char *AT2::getDetails()
{
	static char c[80];

	sprintf(c, "atasize= %d   atbsize= %d", m_atA->GetNumItems(), m_atB->GetNumItems());
	return c;
}

int AT2::m_numWorkerThreads = 1;
void AT2::setNumWorkerThreads(int n)
{
	m_numWorkerThreads = n;
}

void AT2::retireThread()
{
	AttachRetireCore;
	//printf("prio=%d\n", setpriority(PRIO_PROCESS, 0, -1));
	printf("AliasTable Worker Threads = %d\n", m_numWorkerThreads);
	setSCMtwrFactor(m_numWorkerThreads);

	while (!_doExit)
	{
		Lock(&_aliasTableMutex);

		//  Wait for signal on state.
		while (!m_retireA && !m_retireB && !_doExit)
		{
			atprintf("wr");
			//  Wait for a notification of a full table or exit.
			Wait(&_notifyRetire, &_aliasTableMutex);
		}
		Unlock(&_aliasTableMutex);
		if (_doExit)
		{
			printf("nt_at_store= %d \tp_at_msync= %d \twrite_at_comb= %d \t", \
					getNumNtStores(), getNumPMSyncs(), getNumWriteComb());

			pthread_exit(NULL);
		}
		if (m_retireA)
		{
			atprintf("rA");
			m_atA->restoreAllElements();
			Lock(&_aliasTableMutex);
			m_retireA = 0;
			Signal(&_emptyCondition);
			Unlock(&_aliasTableMutex);
		}
		if (m_retireB)
		{
			atprintf("rB");
			m_atB->restoreAllElements();
			Lock(&_aliasTableMutex);
			m_retireB = 0;
			Signal(&_emptyCondition);
			Unlock(&_aliasTableMutex);
		}

		AttachRetireCore;
	}
}

AT2::AT2(WrapLogManager *manager, int size, int thresh)
{
	printf("AT2\n");
	printf("AliasTable Size = %d\n", size);
	printf("AliasTable Threshold = %d\n", thresh);

	_doExit = false;
	m_thresh = thresh;
	m_logManager = manager;
	totalwait = 0;
	//  Allocate the alias tables.
	m_atA = new AliasTableHash(manager, size, 0);
	m_atB = new AliasTableHash(manager, size, 0);
	//  Initialize mutex and condition variable.
	pthread_mutex_init(&_aliasTableMutex, NULL);
	pthread_cond_init (&_emptyCondition, NULL);
	pthread_cond_init (&_notifyRetire, NULL);
	//  Set table A to active.
	m_aliasTableHash = m_atA;
	m_retireA = 0;
	m_retireB = 0;

	AttachPrimaryCore;

	pthread_attr_init(&_attr);
	pthread_attr_setdetachstate(&_attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&_retireThread, &_attr, retireThreadFunc, this);

	usleep(100);
};

AT2::~AT2()
{
	printf("~AT2\n");
	printf("restoring\n");
	restoreAllElements();
	printf("totalwait= %ld\n", totalwait);
	/* Clean up and exit */
	Lock(&_aliasTableMutex);
	_doExit = true;
	printf("signaling\n");
	Signal(&_notifyRetire);
	Unlock(&_aliasTableMutex);
	printf("joining\n");
	printf("%f  avgrestoretime\n", (m_atA->getAvgRestoreTime() + m_atB->getAvgRestoreTime()) / 2.0);
	printf("%ld  totalrestores\n", m_atA->m_totalRestores + m_atB->m_totalRestores);

	pthread_join(_retireThread, NULL);
	pthread_attr_destroy(&_attr);
	pthread_mutex_destroy(&_aliasTableMutex);
	pthread_cond_destroy(&_emptyCondition);
	pthread_cond_destroy(&_notifyRetire);
	delete m_atA;
	delete m_atB;
};

void AT2::onWrapOpen(int wrapToken)
{
	//  Thread's copy / WrAP's copy of Alias Table.
	m_aliasTableHash->onWrapOpen(wrapToken);
};


#define checkReturnCode(rc)	(assert(rc == 0))

void AT2::onWrapClose(int wrapToken, WrapLogger *log)
{
	//  Thread's copy / WrAP's copy of Alias Table.
	m_aliasTableHash->onWrapClose(wrapToken, log);

	//  TODO test num open is zero!!!
	//  if m_aliasTableHash->m_numOpen == 0 then...
	if (m_aliasTableHash->GetNumItems() > (m_aliasTableHash->GetMaxSize()/m_thresh))
	{
		atprintf("F");
		Lock(&_aliasTableMutex);
		if (m_aliasTableHash == m_atA)
		{
			long s = getNsTime();
			while (m_retireB)
			{
				//  Wait for a notification of an empty table.
				Wait(&_emptyCondition, &_aliasTableMutex);
			}
			totalwait += getNsTime() - s;
			m_retireA = 1;
			m_aliasTableHash = m_atB;
			Signal(&_notifyRetire);
			SynchronousWait(&_emptyCondition, &_aliasTableMutex);
			Unlock(&_aliasTableMutex);
		}
		else  // It's atb
		{
			long s = getNsTime();
			while (m_retireA)
			{
				//  Wait for a notification of an empty table.
				Wait(&_emptyCondition, &_aliasTableMutex);
			}
			totalwait += getNsTime() -s ;
			m_retireB = 1;
			m_aliasTableHash = m_atA;
			Signal(&_notifyRetire);
			SynchronousWait(&_emptyCondition, &_aliasTableMutex);
			Unlock(&_aliasTableMutex);
		}
		m_aliasTableHash->clearHash();
		//  No sync since we own it.
		////
		AttachPrimaryCore;
	}
};

void *AT2::read(void *ptr, int size)
{
	//  Thread's copy / WrAP's copy of Alias Table.
	void *v = m_aliasTableHash->read(ptr, size);
	if (v == NULL)
	{
		v = otherTable->read(ptr, size);
	}
	if (v == NULL)
		return ptr;
	return v;
};

void AT2::write(void *ptr, void *src, int size)
{
	assert(0);
	//  Thread's copy / WrAP's copy of Alias Table.
	m_aliasTableHash->write(ptr, src, size);
};

uint64_t AT2::load(void *ptr)
{
	//  Thread's copy / WrAP's copy of Alias Table.
	void *v = m_aliasTableHash->GetItemAddress((uint64_t)ptr);
	if (v == NULL)
	{
		v = otherTable->GetItemAddress((uint64_t)ptr);
	}
	if (v == NULL)
		return *(uint64_t*)ptr;
	return *(uint64_t*)v;
}

void AT2::store(void *ptr, uint64_t value, int size)
{
	//  Thread's copy / WrAP's copy of Alias Table.
	m_aliasTableHash->store(ptr, value, size);
};

void AT2::restoreAllElements()
{
	Lock(&_aliasTableMutex);
	if (m_aliasTableHash == m_atA)
	{
		long s = getNsTime();
		while (m_retireB)
		{
			//  Wait for a notification of an empty table.
			Wait(&_emptyCondition, &_aliasTableMutex);
		}
		totalwait += getNsTime() - s;
		m_retireA = 1;
	}
	if (m_aliasTableHash == m_atB)
	{
		long s = getNsTime();
		while (m_retireA)
		{
			//  Wait for a notification of an empty table.
			Wait(&_emptyCondition, &_aliasTableMutex);
		}
		totalwait += getNsTime() - s;
		m_retireB = 1;
	}
	Signal(&_notifyRetire);
	Wait(&_emptyCondition, &_aliasTableMutex);
	m_atA->clearHash();
	m_atB->clearHash();
	Unlock(&_aliasTableMutex);
	//m_atA->restoreAllElements();
	//m_atB->restoreAllElements();
};

