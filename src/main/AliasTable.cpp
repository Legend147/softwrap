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

#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "AliasTable.h"
#include "memtool.h"
#include "debug.h"
#include "avl.h"

#define SIZE (8192 << 8)

//#define getHash(x) ((((uint64_t)x >> 2)) & (511))
inline unsigned getHash(void *v)
{
	uint64_t key = (uint64_t)v;
	key += ~(key << 32);
	key ^= (key >> 22);
	key += ~(key << 13);
	key ^= (key >> 8);
	key += (key << 3);
	key ^= (key >> 15);
	key += ~(key << 27);
	key ^= (key >> 31);
	return static_cast<unsigned>(key) % SIZE;
}

#define atmalloc malloc

#define checkReturnCode(rc)	(assert(rc == 0))

AliasTable::AliasTable(WrapLogManager *logManager)
{
	_logManager = logManager;
	size = SIZE;
	numElements = 0;
	//numLogged = 0;
	numOpen = 0;

	head = (nodeLocks_t*)calloc(size, sizeof(nodeLocks_t));
	int i = 0;
	for (i = 0; i < size; i ++)
	{
		pthread_rwlock_init(&(head[i].lock), NULL);
	}
};

AliasTable::~AliasTable()
{
	__sync_synchronize();
	if (1)  //(numOpen == 0)  // && (numLogged >= numElements))
	{
		restoreAllElements();
		_logManager->removeAllLogs();
	}
	else
	{
		_logManager->restoreAllLogs();
	}

	freeTable();
};

void AliasTable::freeTable()
{
	printf("AliasTableSize= \t%d\n", numElements);
	//  Free and destroy all locks, nodes, and data.
	int i = 0;
	for (i = 0; i < size; i ++)
	{
		//  Free the lock.
		pthread_rwlock_destroy(&(head[i].lock));

		node_t *n = &(head[i].node);
		if ((n->flag & POINTER) > 0)
		{
			//assert(0);
			free(n->value);
		}
		n = n->next;
		while (n != NULL)
		{
			if ((n->flag & POINTER) > 0)
				free(n->value);
			node_t *p = n;
			n = n->next;
			free(p);
		}
	}

	//  Free array.
	free(head);
}

//  TODO this is unlocked for now!
void AliasTable::restoreAllElements()
{
	printf("restoring all elements!\n");

	int i = 0;
	int t = 0;
	for (i = 0; i < size; i++)
	{
		//  Would do a lock here.
		//  lock.
		//printf("i=%d\n", i);

		node_t *n = &(head[i].node);
		node_t *nhead = n;
		while (n != NULL)
		{
			if (n->key != NULL)
			{
				Debug("n->flag=%d\n", n->flag);

				if (n->flag & VALUE)
				{
					if (n->flag >> 3 == 8)
					{
						//*(uint64_t *)n->key = (uint64_t)n->value;
						ntstore(n->key, &(n->value), 8);
					}
					else if (n->flag >> 3 == 4)
					{
						//*(uint32_t *)n->key = (uint64_t)n->value;
						ntstore(n->key, &(n->value), 4);
					}
					else
					{
						assert(0);
					}
					Debug("m\n");
				}
				else
				{
					//memcpy(n->key, n->value, n->flag >> 3);
					ntstore(n->key, n->value, n->flag >> 3);

					//  Free
					//free(n->value);
					Debug("v\n");
				}
				t++;
			}
			node_t *p = n;
			n = n->next;
			if (p != nhead)
			{
				Debug("ff");
				free(p);
			}
			else
			{
				p->flag = 0;
				p->key = NULL;
				p->next = NULL;
				p->value = 0;
			}
			//if (n != NULL)
				//printf("X");
		}
		//  Release lock.
		//  unlock.
	}
	p_msync();
	printf("\ntotalRestored= \t%d\n", t);
}

void AliasTable::onWrapOpen(int wrapToken)
{
	__sync_add_and_fetch(&numOpen, 1);
};


void AliasTable::onWrapClose(int wrapToken, WrapLogger *log)
{
	__sync_sub_and_fetch(&numOpen, 1);
};

void *AliasTable::read(void *ptr,  int size)
{
	void *base = getBasePtr(ptr);
	int hash = getHash(base);
	int rc = pthread_rwlock_rdlock(&(head[hash].lock));
	checkReturnCode(rc);

	node_t *n = find(base, hash);

	rc = pthread_rwlock_unlock(&(head[hash].lock));
	checkReturnCode(rc);

	if (n == NULL)
		return ptr;
	assert((n->flag >> 3) >= ((char*)ptr-(char*)base) + size);
	return (char*)n->value + ((char*)ptr - (char*)base);
};

void AliasTable::write(void *ptr, void *src, int size)
{
	void *base = getBasePtr(ptr);
	int hash = getHash(base);
	int rc = pthread_rwlock_wrlock(&(head[hash].lock));
	checkReturnCode(rc);
	//__sync_add_and_fetch(&numLogged, 1);

	node_t *n = find(base, hash);
	if (n == NULL)
	{
		//  Allocate and put.
		void *v = atmalloc(size);
		n = put(base, hash, v);
		putBasePtr(ptr, size);
		//__sync_add_and_fetch(&numElements, 1);
		//n->flag = POINTER;
		n->flag = (size << 3) | POINTER;
	}
	else
	{
		//  If current object size < write size + offset then allocate more space.
		int newsize = size + ((char*)ptr-(char*)base);
		if (n->flag >> 3 < newsize)
		{
			//  Make sure end point is not contained in another object!
			assert(getBasePtr((char*)ptr + size - 1) == (char*)ptr + size - 1);
			printf("REALLOC!!\n");
			n->value = realloc(n->value, newsize);
			n->flag = (newsize << 3) | POINTER;
			//  Adjust the size in the tree.
			adjBasePtrSize(base, newsize);
		}
	}
	//  Copy
	//assert(ptr == base);
	memcpy((char*)n->value + ((char*)ptr-(char*)base), src, size);

	rc = pthread_rwlock_unlock(&(head[hash].lock));
	checkReturnCode(rc);
};

void *AliasTable::load(void *ptr)
{
	int hash = getHash(ptr);
	int rc = pthread_rwlock_rdlock(&(head[hash].lock));
	checkReturnCode(rc);

	node_t *n = find(ptr, hash);

	rc = pthread_rwlock_unlock(&(head[hash].lock));
	checkReturnCode(rc);

	//  TODO FIX
	if (n == NULL)
	{
		return ptr;
	}
	return &(n->value);
};

void AliasTable::store(void *ptr, uint64_t value, int size)
{
	int hash = getHash(ptr);
	int rc = pthread_rwlock_wrlock(&(head[hash].lock));
	checkReturnCode(rc);
	//__sync_add_and_fetch(&numLogged, 1);

	node_t *n = find(ptr, hash);
	if (n == NULL)
	{
		n=put(ptr, hash, (void *)value);
		//__sync_add_and_fetch(&numElements, 1);
	}
	else
	{
		n->value = (void *)value;
	}
	n->flag = (size << 3) | VALUE;

	rc = pthread_rwlock_unlock(&(head[hash].lock));
	checkReturnCode(rc);
};

//  Methods used by the wrap processing thread.
void AliasTable::compareAndFlag(void *ptr, void *cmp, int size)
{
	int hash = getHash(ptr);
	int rc = pthread_rwlock_wrlock(&(head[hash].lock));
	checkReturnCode(rc);

	node_t *n = find(ptr, hash);
	if (n != NULL)
	{
		//  Compare
		if (memcmp(n->value, cmp, size) == 0)
			n->flag |= DELETABLE;
	}
	rc = pthread_rwlock_unlock(&(head[hash].lock));
	checkReturnCode(rc);
};

//  Methods used by the wrap processing thread.
void AliasTable::compareAndFlag(void *ptr, uint64_t value)
{
	int hash = getHash(ptr);
	int rc = pthread_rwlock_wrlock(&(head[hash].lock));
	checkReturnCode(rc);

	node_t *n = find(ptr, hash);
	if (n != NULL)
	{
		if (n->value == (void *)value)
			n->flag |= DELETABLE;
	}
	rc = pthread_rwlock_unlock(&(head[hash].lock));
	checkReturnCode(rc);
};

const char *AliasTable::getDetails()
{
	static char c[80];
	sprintf(c, "size= %d", numElements);
	return c;
}


//  Find and allocate.
AliasTable::node_t *AliasTable::find(void *ptr, int hash)
{
	assert(ptr != NULL);
	node_t *n = &(head[hash].node);
	while (n->key != ptr)
	{
		n = n->next;
		if (n == NULL)
			break;
	}
	return n;
};

//  Put.
AliasTable::node_t *AliasTable::put(void *ptr, int hash, void *value)
{
	assert(ptr != NULL);

	//  TODO Sync add!
	numElements++;

	if (head[hash].node.key == NULL)
	{
		head[hash].node.key = ptr;
		head[hash].node.value = value;
		head[hash].node.flag = VALUE;
		return &(head[hash].node);
	}

	//  Allocate and put.
	node_t *n = (node_t *)atmalloc(sizeof(node_t));
	n->key = ptr;
	n->value = value;
	n->flag = VALUE;
	n->next = head[hash].node.next;
	head[hash].node.next = n;
	return n;
};

