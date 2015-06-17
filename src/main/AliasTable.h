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

#ifndef __AliasTable_h
#define __AliasTable_h

#include <stdint.h>
#include "WrapLogManager.h"
#include "AliasTableBase.h"

using namespace std;

class AliasTable: public AliasTableBase
{
public:
	AliasTable(WrapLogManager *logManager);
	~AliasTable();

	//void resetTable();

	void onWrapOpen(int wrapToken);
	void onWrapClose(int wrapToken, WrapLogger *log);

	void *read(void *ptr, int size);
	void write(void *ptr, void *src, int size);

	uint64_t load(void *ptr);
	void store(void *ptr, uint64_t value, int size);

	//  Methods used by the wrap processing thread.
	void compareAndFlag(void *ptr, void *cmp, int size);
	void compareAndFlag(void *ptr, uint64_t value);

	const char *getDetails();

	//  Restore all elements.
	void restoreAllElements();

private:

	enum nodeInfo {DELETABLE = 1, POINTER = 2, VALUE = 4};

	struct node_t
	{
		void *key;
		void *value;
		int flag;
		//  Next in chain.
		node_t *next;
	};

	struct nodeLocks_t
	{
		struct node_t node;

		//  R/W Lock.
		pthread_rwlock_t lock;
	};

	//  Find and allocate
	node_t *find(void *ptr, int hash);
	node_t *put(void *ptr, int hash, void *value);

	//  Free the table.
	void freeTable();

	WrapLogManager *_logManager;
	nodeLocks_t *head;
	int size;
	int numElements;
	//int numLogged;
	int numOpen;
};

#endif
