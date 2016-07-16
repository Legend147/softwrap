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
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "AT3.h"
#include "memtool.h"
#include "unistd.h"

#define atprintf(x) //printf(x); fflush(stdout)
#define Debug(...)

//bool __sync_bool_compare_and_swap (type *ptr, type oldval type newval, ...)
/*
#define CAS __sync_bool_compare_and_swap
volatile int lock = 0;
#define openSpinLock 	while(!CAS(&lock, 0, 1));
#define closeSpinLock	lock = 0;
 */

#if 1
#include "avl.h"
#else
#define putBasePtr(ptr, size)
#define adjBasePtrSize(ptr, size)
#define getBasePtr(ptr)		ptr
#endif

// Use pthreads
#if 1
#define Lock	pthread_mutex_lock
#define Unlock	pthread_mutex_unlock
#define Wait(x,y)	atprintf("X-Wait\n");	pthread_cond_wait(x,y)
#define Signal(x) 	atprintf("X-Signal\n"); pthread_cond_signal(x)
#define Broadcast(x)	atprintf("X-Bcast\n"); pthread_cond_broadcast(x)
#else
#define Lock(x)
#define Unlock(x)
#define Wait(x,y)	usleep(1)
#define Signal(x)
#endif

#define SynchronousWait(x,y)
//Wait(x,y)

#define otherTable ((m_aliasTableHash == m_atA)?m_atB:m_atA)

void *AT3::retireThreadFunc(void *t)
{
	((AT3*)t)->retireThread();
	return NULL;
}

const char *AT3::getDetails()
{
	static char c[80];

	sprintf(c, "atasize= %d   atbsize= %d", m_atA->GetNumItems(), m_atB->GetNumItems());
	return c;
}

int AT3::m_numWorkerThreads = 1;
void AT3::setNumWorkerThreads(int n)
{
	m_numWorkerThreads = n;
}

__thread AliasTableHash *AT3::m_aliasTableHash = NULL;
__thread bool AT3::m_readOtherTable = false;

AT3::AT3(WrapLogManager *manager, int size, int thresh)
{
	printf("AT3\n");
	printf("AliasTable Size = %d\n", size);
	printf("AliasTable Threshold = %d\n", thresh);

	_doExit = false;
	m_thresh = thresh;
	m_logManager = manager;
	totalwait = 0;
	//  Allocate the alias tables.
	m_atA = new AliasTableHash(manager, size);
	m_atB = new AliasTableHash(manager, size);
	//  Initialize mutex and condition variable.
	pthread_mutex_init(&_aliasTableMutex, NULL);
	pthread_cond_init (&_notifyRetire, NULL);
	pthread_cond_init (&_notifyEmpty, NULL);
	m_stateA = Empty;
	m_stateB = Empty;
	m_otherReadersA = 0;
	m_otherReadersB = 0;
	m_numA = 0;
	m_numB = 0;

	pthread_attr_init(&_attr);
	pthread_attr_setdetachstate(&_attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&_retireThread, &_attr, retireThreadFunc, this);

	__sync_synchronize();
	AttachPrimaryCore;

	usleep(100);
};

//  TODO assume for now on a write we won't go over!!!!!!!!!!

#define crossedThreshold	(m_aliasTableHash->GetNumItems() > (m_aliasTableHash->GetMaxSize()/m_thresh))
#define thisState		((m_aliasTableHash==m_atA)?m_stateA:m_stateB)

void AT3::onWrapOpen(int wrapToken)
{
	//  Default don't read the other table.
	m_readOtherTable = false;
	atprintf("O\n");

	//  Get the alias table.
	Lock(&_aliasTableMutex);
	while (m_aliasTableHash == NULL)
	{
		if ((m_stateA == Active) || (m_stateA == Empty))
		{
			m_stateA = Active;
			m_aliasTableHash = m_atA;
		}
		else if ((m_stateB == Active) || (m_stateB == Empty))
		{
			m_stateB = Active;
			m_aliasTableHash = m_atB;
		}
		else
		{
			atprintf("waiting on empty\n");
			Wait(&_notifyEmpty, &_aliasTableMutex);
		}

	}
	if (m_aliasTableHash == m_atA)
	{
		m_numA++;
		if ((m_stateB == Active) || (m_stateB == WaitWriters) || (m_stateB == Retire))
		{
			m_otherReadersB++;
			m_readOtherTable = true;
		}
		atprintf("AAAAAA\n");
	}
	else
	{
		m_numB++;
		if ((m_stateA == Active) || (m_stateA == WaitWriters) || (m_stateA == Retire))
		{
			m_otherReadersA++;
			m_readOtherTable = true;
		}
		atprintf("BBBBBBBB\n");
	}
	// assert(thisState == Active);
	Unlock(&_aliasTableMutex);
	//  Thread's copy / WrAP's copy of Alias Table.
	m_aliasTableHash->onWrapOpen(wrapToken);
};


void AT3::onWrapClose(int wrapToken, WrapLogger *log)
{
	//  Thread's copy / WrAP's copy of Alias Table.
	m_aliasTableHash->onWrapClose(wrapToken, log);

	int writers, otherreaders;
	atprintf("C\n");


	//  Fetch and sub.
	Lock (&_aliasTableMutex);
	{
		if (m_aliasTableHash == m_atA)
			writers = --m_numA;//__sync_sub_and_fetch(&m_numA, 1);
		else
			writers = --m_numB;//__sync_sub_and_fetch(&m_numB, 1);
		Debug("na=%d nb=%d\n", m_numA, m_numB);
		if (m_readOtherTable)
		{
			int state = -1;
			if (m_aliasTableHash == m_atA)
			{
				otherreaders = --m_otherReadersB;//__sync_sub_and_fetch(&m_otherReadersB, 1);
				state = m_stateB;
			}
			else
			{
				otherreaders = --m_otherReadersA;//__sync_sub_and_fetch(&m_otherReadersA, 1);
				state = m_stateA;
			}
			if ((otherreaders == 0) && (state == WaitReaders))
			{
				//  Signal the other table to go zero out and mark empty.
				//Lock(&_aliasTableMutex);
				//  TODO check to make sure writers is zero
				//if (m_aliasTableHash == m_atA)
					//m_stateB = Retire;
				//else
					//m_stateA = Retire;

				Signal(&_notifyRetire);
				atprintf("otherreaders zero\n");
				//Unlock(&_aliasTableMutex);
			}
		}

		if (!crossedThreshold)
		{
			atprintf("Notcrossed!\n");
			Unlock(&_aliasTableMutex);
			m_aliasTableHash = NULL;
			return;
		}
		else
			atprintf("Crossed!\n");

		if (writers == 0)
		{
			if (m_aliasTableHash == m_atA)
			{
				m_stateA = Retire;
			}
			else
			{
				m_stateB = Retire;
			}
			atprintf("signal ready to retire\n");
			Signal(&_notifyRetire);
		}
		else
		{
			if (m_aliasTableHash == m_atA)
				m_stateA = WaitWriters;
			else
				m_stateB = WaitWriters;
		}
		m_aliasTableHash = NULL;

	}
	Unlock(&_aliasTableMutex);
}


void *AT3::read(void *ptr, int size)
{
	atprintf("r\n");
	void *base = getBasePtr(ptr);
	Debug("read ptr=%p base=%p\n", ptr, base);

	//  Thread's copy / WrAP's copy of Alias Table.
	void *v = m_aliasTableHash->read(base, size);
	if ((v == NULL) && (m_readOtherTable))
	{
		v = otherTable->read(base, size);
	}
	if (v == NULL)
	{
		atprintf("returning original\n");
		return ptr;
	}
	Debug("returning alias %p\n", v);
	return v;
};

void AT3::write(void *ptr, void *src, int size)
{
	void *base = getBasePtr(ptr);
	int currentSize = -1;
	void *addr = (void*)m_aliasTableHash->GetItem((uint64_t)base, &currentSize);

	if (addr == NULL)
	{
		//  Allocate and put.
		addr = malloc(size);
		//assert(ptr == base);
		//  Thread's copy / WrAP's copy of Alias Table.
		m_aliasTableHash->store(base, (uint64_t)addr, size);
		putBasePtr(base, size);
		//__sync_add_and_fetch(&numElements, 1);
		//n->flag = POINTER;
		//n->flag = (size << 3) | POINTER;
		Debug("malloc size=%d alias for %p is %p\n", size, ptr, addr);
	}
	else
	{
		assert(currentSize > 0);
		Debug("size=%d \n", size);
		//  If current object size < write size + offset then allocate more space.
		int newsize = size + ((char*)ptr-(char*)base);
		if (currentSize < newsize)
		{
			//  Make sure end point is not contained in another object!
			assert(getBasePtr((char*)ptr + size - 1) == (char*)ptr + size - 1);
			atprintf("REALLOC!!\n");
			addr = realloc(base, newsize);
			assert(addr != NULL);
			//  Write the new value in the table.
			m_aliasTableHash->store(ptr, (uint64_t)addr, newsize);
			//  Adjust the size in the tree.
			adjBasePtrSize(base, newsize);
		}
	}
	//  Copy
	//assert(ptr == base);
	memcpy((char*)addr + ((char*)ptr-(char*)base), src, size);
	atprintf("copied\n");
};

uint64_t AT3::load(void *ptr)
{
	//  Thread's copy / WrAP's copy of Alias Table.
	void *v = m_aliasTableHash->GetItemAddress((uint64_t)ptr);
	if ((v == NULL) && (m_readOtherTable))
	{
		v = otherTable->GetItemAddress((uint64_t)ptr);
	}
	if (v == NULL)
		return *(uint64_t*)ptr;
	return *(uint64_t*)v;
}

void AT3::store(void *ptr, uint64_t value, int size)
{
	//  Thread's copy / WrAP's copy of Alias Table.
	m_aliasTableHash->store(ptr, value, size);
	//atprintf("store\n");
};




void AT3::retireThread()
{
	AttachRetireCore;
	//printf("prio=%d\n", setpriority(PRIO_PROCESS, 0, -1));
	printf("AliasTable Worker Threads = %d\n", m_numWorkerThreads);
	setSCMtwrFactor(m_numWorkerThreads);

	while (!_doExit)
	{
		Lock(&_aliasTableMutex);

		//  Wait for signal on state.
		while ((m_stateA != Retire) && ((m_stateA != WaitReaders) || (m_otherReadersA != 0)) &&
			   (m_stateB != Retire) && ((m_stateB != WaitReaders) || (m_otherReadersB != 0)) && !_doExit)
		{
			atprintf("r-wait\n");
			//  Wait for a notification of a full table or exit.
			Wait(&_notifyRetire, &_aliasTableMutex);
		}
		atprintf("r-awoke\n");
		if ((m_stateA == Retire) || (m_stateA == WaitReaders))
		{
			if (m_stateA == Retire)
			{
				atprintf("r-retireA\n");
				Unlock(&_aliasTableMutex);
				m_atA->restoreAllElements();
				atprintf("r-restored\n");
				Lock(&_aliasTableMutex);
				if (m_otherReadersA > 0)
				{
					atprintf("r-otherReadersA\n");
					m_stateA = WaitReaders;
				}
			}
			if (m_otherReadersA == 0)
			{
				atprintf("r-noReadersAClearing\n");
				Unlock(&_aliasTableMutex);
				m_atA->clearHash();
				Lock(&_aliasTableMutex);
				m_stateA = Empty;
				Broadcast(&_notifyEmpty);
			}
		}
		if ((m_stateB == Retire) || (m_stateB == WaitReaders))
		{
			if (m_stateB == Retire)
			{
				atprintf("r-retireB\n");
				Unlock(&_aliasTableMutex);
				m_atB->restoreAllElements();
				atprintf("r-restored\n");
				Lock(&_aliasTableMutex);
				if (m_otherReadersB > 0)
				{
					atprintf("r-otherReadersB\n");
					m_stateB = WaitReaders;
				}
			}
			if (m_otherReadersB == 0)
			{
				atprintf("r-noReadersBClearing\n");
				Unlock(&_aliasTableMutex);
				m_atB->clearHash();
				__sync_synchronize();
				Lock(&_aliasTableMutex);
				m_stateB = Empty;
				Broadcast(&_notifyEmpty);
			}
		}
		Unlock(&_aliasTableMutex);
		AttachRetireCore;
	}
	if (_doExit)
	{
		printf("nt_at_store= %d \tp_at_msync= %d \twrite_at_comb= %d \t", \
				getNumNtStores(), getNumPMSyncs(), getNumWriteComb());

		//pthread_exit(NULL);
	}
}


//  --------------------







AT3::~AT3()
{
	printf("~AT3\n");
	printf("restoring\n");
	restoreAllElements();
	printf("totalwait= %ld\n", totalwait);
	/* Clean up and exit */
	Lock(&_aliasTableMutex);
	_doExit = true;
	__sync_synchronize();
	printf("signaling\n");
	Signal(&_notifyRetire);
	Unlock(&_aliasTableMutex);
	printf("joining\n");
	printf("%f  avgrestoretime\n", (m_atA->getAvgRestoreTime() + m_atB->getAvgRestoreTime()) / 2.0);
	printf("%ld  totalrestores\n", m_atA->m_totalRestores + m_atB->m_totalRestores);

	pthread_join(_retireThread, NULL);
	pthread_attr_destroy(&_attr);
	pthread_mutex_destroy(&_aliasTableMutex);
	pthread_cond_destroy(&_notifyEmpty);
	pthread_cond_destroy(&_notifyRetire);

	/** Make sure tables have flushed.  */
	if (m_stateA != Empty)
	{
		m_atA->restoreAllElements();
		m_atA->clearHash();
		m_stateA = Empty;
	}
	if (m_stateB != Empty)
	{
		m_atB->restoreAllElements();
		m_atB->clearHash();
		m_stateB = Empty;
	}
	delete m_atA;
	delete m_atB;
};



void AT3::restoreAllElements()
{
	Lock(&_aliasTableMutex);
	if (m_stateA != Empty)
		m_stateA = Retire;
	if (m_stateB != Empty)
		m_stateB = Retire;
	Signal(&_notifyRetire);
	while ((m_stateA != Empty) && (m_stateB != Empty))
	{
		printf("W\n");
		Wait(&_notifyEmpty, &_aliasTableMutex);
	}
	Unlock(&_aliasTableMutex);
};

