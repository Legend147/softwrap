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

#include <limits.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "LFHT.h"

inline static uint64_t integerHash(uint64_t h)
{
	h ^= h >> 2;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

inline static uint64_t integerHash2(uint64_t Ptr)
{
	uint32_t Value = (uint32_t)Ptr;
	Value = ~Value + (Value << 15);
	Value = Value ^ (Value >> 12);
	Value = Value + (Value << 2);
	Value = Value ^ (Value >> 4);
	Value = Value * 2057;
	Value = Value ^ (Value >> 16);
	return Value;
}

/*inline static uint64_t integerHash(uint64_t h)
{
	return (h >> 2) & ((BloomSize << 6) - 1);
}
 */
//#define integerHash(x)	(x>>2)

inline static uint32_t integerHash4(uint64_t h)
{
	//	printf("%p ", (void*)h);
	uint32_t a = h >> 2;
	a = (a^0xdeadbeef) + (a<<4);
	a = a ^ (a>>10);
	a = a + (a<<7);
	a = a ^ (a>>13);
	//	printf("%d\n", a&((1<<13) -1));
	return a;
}

inline uint64_t cas(uint64_t *ptr, uint64_t oldval, uint64_t newval)
{
	return __sync_val_compare_and_swap(ptr, oldval, newval);
	/*
	uint64_t t = *ptr;
	if (t == oldval)
		*ptr = newval;
	return t;
	*/
}

#define load_64_relaxed(ptr)	*ptr
#define store_64_relaxed(ptr, val)	*ptr = val
#define add_fetch_mt(ptr, val)   __sync_fetch_and_add(ptr, val)
#define add_fetch_single(ptr, val)  *ptr=*ptr+val
//__sync_fetch_and_add (ptr, val)
//__atomic_add_fetch(ptr, val, __ATOMIC_SEQ_CST)
//  TODO cas not traditional on intel!!
//#define cas(ptr, oldval, newval)  __sync_val_compare_and_swap(ptr, oldval, newval)

//#define load_64_relaxed(x)	mint_load_64_relaxed(x)
//#define store_64_relaxed(var, val)	mint_store_64_relaxed(var, val)
//mint_compare_exchange_strong_32_relaxed

LFHT::LFHT(int size, int mt)
{
	m_entries = (Entry*)malloc(size * sizeof(Entry));
	m_arraySize = size;
	m_numItems = 0;
	m_mt = mt;
	clear();
}

//  This should be performed by a thread containing a lock.
void LFHT::clear()
{
	for (int i = 0; i < m_arraySize; i++)
	{
		m_entries[i].key = 0;
		m_entries[i].value = 0;
		m_entries[i].size = 0;
	}
	clearBloom();
}

LFHT::~LFHT()
{
	for (int i = 0; i < m_arraySize; i++)
	{
		assert(m_entries[i].key == 0);
	}
	free(m_entries);
}

int LFHT::GetNumItems()
{
	if (m_mt)
		return add_fetch_mt(&m_numItems, 0);
	else
		return add_fetch_single(&m_numItems, 0);
}

int LFHT::GetMaxSize()
{
	return m_arraySize;
}

uint64_t LFHT::GetItem(uint64_t key, int *size)
{
	for (uint64_t idx = integerHash(key); ; idx++)
	{
		idx &= m_arraySize - 1;

		uint64_t probedKey = load_64_relaxed(&m_entries[idx].key);
		if (probedKey == key)
		{
			if (size != NULL)
				*size = m_entries[idx].size;
			return load_64_relaxed(&m_entries[idx].value);
		}
		if (probedKey == 0)
		{
			if (size != NULL)
				*size = 0;
			return 0;
		}
	}
}

void *LFHT::GetItemAddress(uint64_t key)
{
	if (checkBloom(key))
		for (uint64_t idx = integerHash(key); ; idx++)
		{
			idx &= (m_arraySize - 1);

			uint64_t probedKey = m_entries[idx].key;//load_64_relaxed(&m_entries[idx].key);
			if (probedKey == key)
			{
				return &m_entries[idx].value;
			}
			if (probedKey == 0)
				return NULL;
			//printf("key=%ld probed=%ld idx=%ld\n", key, probedKey, idx);
		}
	else
		return NULL;
}

void LFHT::SetItem(uint64_t key, uint64_t value, int size)
{
	/*
	static int sets = 0;
	printf("s=%d\n", sets);
	sets = sets+1;
	 */
	setBloom(key);
	for (uint64_t idx = integerHash(key);; idx++)
	{
		idx &= (m_arraySize - 1);

		// Load the key that was there.
		uint64_t probedKey = load_64_relaxed(&m_entries[idx].key);
		if (probedKey != key)
		{
			// The entry was either free, or contains another key.
			if (probedKey != 0)
				continue;           // Usually, it contains another key. Keep probing.

			//m_entries[idx].key = key;

			 // The entry was free. Now let's try to take it using a CAS.
			 uint64_t prevKey;
			 if (m_mt)
				 prevKey = cas(&m_entries[idx].key, 0, key);
			 else if (m_entries[idx].key == 0)
			 {
				 prevKey = m_entries[idx].key;
				 m_entries[idx].key = key;
			 }

			 if ((prevKey != 0) && (prevKey != key))
				 continue;       // Another thread just stole it from underneath us.
			 // Either we just added the key, or another thread did.
			 //  Increment the number of items since the key was added.
			 if (prevKey == 0)
			 {
				 if (m_mt)
					 add_fetch_mt(&m_numItems, 1);
				 else
					 add_fetch_single(&m_numItems, 1);
			 }
		}

		// Store the value in this array entry.
		store_64_relaxed(&m_entries[idx].value, value);
		//  Store the size in this array entry.
		store_64_relaxed(&m_entries[idx].size, size);

		return;
	}
}

