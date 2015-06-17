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

#ifndef __AT3_h
#define __AT3_h

#include <stdint.h>
#include "WrapLogManager.h"
#include "AliasTableBase.h"
#include "AliasTableHash.h"

using namespace std;

class AT3: public AliasTableBase
{
public:
	AT3(WrapLogManager *manager, int size, int thresh);
	~AT3();
	void onWrapOpen(int wrapToken);
	void onWrapClose(int wrapToken, WrapLogger *log);

	void *read(void *ptr, int size);
	void write(void *ptr, void *src, int size);

	uint64_t load(void *ptr);
	void store(void *ptr, uint64_t value, int size);

	const char *getDetails();

	//  Restore all elements.
	void restoreAllElements();

	static void setNumWorkerThreads(int n);

private:
	WrapLogManager *m_logManager;

	//  Two Alias Tables managed by this super class.
	AliasTableHash *m_atA;
	AliasTableHash *m_atB;
	//  Size threshold.
	int m_thresh;

	typedef enum {Active, WaitWriters, Retire, WaitReaders, Empty} AliasState;
	AliasState m_stateA;
	AliasState m_stateB;
	int m_otherReadersA;
	int m_otherReadersB;
	int m_numA;
	int m_numB;

	//  Thread's copy of Alias Table pointer.
	//typedef AliasTableHash * atp;
	//static atp __thread h;
	static __thread AliasTableHash *m_aliasTableHash;
	static __thread bool m_readOtherTable;

	static void *retireThreadFunc(void *t);
	void retireThread();

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
	pthread_cond_t  _notifyEmpty;

	pthread_t _retireThread;
	pthread_attr_t _attr;
	 bool _doExit;
	long totalwait;
	static int m_numWorkerThreads;
};

#endif
