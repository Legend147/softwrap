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
#include "AT2.h"
#include "memtool.h"

#define atprintf(x) //printf(x); fflush(stdout)
//bool __sync_bool_compare_and_swap (type *ptr, type oldval type newval, ...)
/*
#define CAS __sync_bool_compare_and_swap
int lock = 0;
#define openSpinLock 	while(!CAS(&lock, 0, 1));
#define closeSpinLock	lock = 0;
*/

// Use pthreads
pthread_mutex_t _aliasTableMutex;
pthread_cond_t  _notifyRetire;
pthread_cond_t  _emptyCondition;
#if 1
#define Lock	pthread_mutex_lock
#define Unlock	pthread_mutex_unlock
#define Wait	pthread_cond_wait
#define Signal 	pthread_cond_signal
#else
#define Lock(x)
#define Unlock(x)
#define Wait(x,y)
#define Signal(x)
#endif


pthread_t _retireThread;
pthread_attr_t _attr;
bool _doExit = false;

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

void AT2::retireThread()
{
	while (!_doExit)
	{
		//  Acquire lock.
		Lock(&_aliasTableMutex);
		//  Wait for signal on state.
		while ((m_atAState != Full) && (m_atBState != Full) && !_doExit)
		{
			//  Wait for a notification of a full table or exit.
			Wait(&_notifyRetire, &_aliasTableMutex);
		}
		if (_doExit)
		{
			Unlock(&_aliasTableMutex);
			pthread_exit(NULL);
		}
		if (m_atAState == Full)
		{
			atprintf("rA");
			m_atAState = Retiring;
			Unlock(&_aliasTableMutex);
			m_atA->restoreAllElements();
			Lock(&_aliasTableMutex);
			m_atAState = Empty;
			Signal(&_emptyCondition);
			Unlock(&_aliasTableMutex);
		}
		if (m_atBState == Full)
		{
			atprintf("rB");
			m_atBState = Retiring;
			Unlock(&_aliasTableMutex);
			m_atB->restoreAllElements();
			Lock(&_aliasTableMutex);
			m_atBState = Empty;
			Signal(&_emptyCondition);
			Unlock(&_aliasTableMutex);
		}
	}
}

AT2::AT2(WrapLogManager *manager, int size, int thresh)
{
	printf("AT2\n");
	printf("AliasTable Size = %d\n", size);
	printf("AliasTable Threshold = %d\n", thresh);
	m_thresh = thresh;
	m_logManager = manager;
	//  Allocate the alias tables.
	m_atA = new AliasTableHash(manager, size);
	m_atB = new AliasTableHash(manager, size);
	//  Set table A to active.
	m_atAState = Active;
	m_atBState = Empty;
	//  Initialize mutex and condition variable.
	pthread_mutex_init(&_aliasTableMutex, NULL);
	pthread_cond_init (&_emptyCondition, NULL);
	pthread_cond_init (&_notifyRetire, NULL);
	//  Thread hasn't opened a wrap yet.
	m_aliasTableHash = NULL;

	pthread_attr_init(&_attr);
	pthread_attr_setdetachstate(&_attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&_retireThread, &_attr, retireThreadFunc, this);
};

AT2::~AT2()
{
	printf("~AT2\n");
	/* Clean up and exit */
	Lock(&_aliasTableMutex);
	_doExit = true;
	printf("signaling\n");
	Signal(&_notifyRetire);
	Unlock(&_aliasTableMutex);
	printf("joining\n");
	pthread_join(_retireThread, NULL);
	/*
	  //pthread_attr_destroy(&attr);
	  //pthread_mutex_destroy(&count_mutex);
	  //pthread_cond_destroy(&count_threshold_cv);
	  //pthread_exit(NULL);
	 */
	printf("restoring\n");
	restoreAllElements();
	delete m_atA;
	delete m_atB;
};

void AT2::onWrapOpen(int wrapToken)
{
	//  Acquire lock.
	Lock(&_aliasTableMutex);
	//  Wait for signal on state.
	while ((m_atAState != Active) && (m_atAState != Empty) &&
			(m_atBState != Active) && (m_atBState != Empty))
	{
		printf("waiting for empty or active table!\n");
		//  Wait for an empty or active table.
		Wait(&_emptyCondition, &_aliasTableMutex);
	}

	if (m_atAState == Active)
		m_aliasTableHash = m_atA;
	else if (m_atBState == Active)
		m_aliasTableHash = m_atB;
	else if (m_atAState == Empty)
	{
		m_atAState = Active;
		m_aliasTableHash = m_atA;
	}
	else if (m_atBState == Empty)
	{
		m_atBState = Active;
		m_aliasTableHash = m_atB;
	}
	//  Thread's copy / WrAP's copy of Alias Table.
	m_aliasTableHash->onWrapOpen(wrapToken);

	//  Release lock.
	Unlock(&_aliasTableMutex);
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
		//  Acquire lock.
		Lock(&_aliasTableMutex);
		//  Mark full.
		if (m_aliasTableHash == m_atA)
			m_atAState = Full;
		else if (m_aliasTableHash == m_atB)
			m_atBState = Full;
		else
			assert(0);
		//  Signal retire thread.
		Signal(&_notifyRetire);
		//  Release lock.
		Unlock(&_aliasTableMutex);
	}
	m_aliasTableHash = NULL;
};

void *AT2::read(void *ptr, int size)
{
	//  Thread's copy / WrAP's copy of Alias Table.
	return m_aliasTableHash->read(ptr, size);
};

void AT2::write(void *ptr, void *src, int size)
{
	//  Thread's copy / WrAP's copy of Alias Table.
	m_aliasTableHash->write(ptr, src, size);
};

void *AT2::load(void *ptr)
{
	//  Thread's copy / WrAP's copy of Alias Table.
	return m_aliasTableHash->load(ptr);
};

void AT2::store(void *ptr, uint64_t value, int size)
{
	//  Thread's copy / WrAP's copy of Alias Table.
	m_aliasTableHash->store(ptr, value, size);
};

void AT2::restoreAllElements()
{
	m_atA->restoreAllElements();
	m_atB->restoreAllElements();
};

