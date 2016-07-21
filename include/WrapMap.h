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

#ifndef __WrapMap_H
#define __WrapMap_H

// Configurable table size constant
const int TABLE_SIZE = 4096;

#include "wrap.h"
#include "WrapVar.h"
#include <string.h>

// Hash node class
template<class K, class V>
class HashNode {
public:
    inline K key(WRAPTOKEN w = 0)
    {
    	K temp;
    	wrapRead(&temp, &_key, sizeof(K), w);
    	return temp;
    }
    inline V value(WRAPTOKEN w = 0)
    {
    	V temp;
    	wrapRead(&temp, &_value, sizeof(V), w);
    	return temp;
    }
    HashNode* next(WRAPTOKEN w = 0)
    {
        HashNode *t;
        wrapRead(&(t), &_next, sizeof(HashNode*), w);
        return t;
    }
    inline void setValue(V value, WRAPTOKEN w)
    {
    	wrapWrite(&_value, &value, sizeof(V), w);
    }
    inline void setKey(K key, WRAPTOKEN w)
    {
    	wrapWrite(&_key, &key, sizeof(K), w);
    }
    inline void setNext(HashNode* next, WRAPTOKEN w)
    {
    	wrapWrite(&_next, &next, sizeof(HashNode*), w);
    }

private:
    // Key-value pair
    K _key;
    V _value;
    HashNode* _next;
};


// Hash map class
template<class K, class V>
class WrapMap
{

typedef HashNode<K,V> HashEntry;

public:
	WrapMap(int size = TABLE_SIZE)
	{
		WRAPTOKEN w = wrapOpen();
		_table.set((HashEntry **)pmalloc(sizeof(HashEntry *)*size), w);
		_size.set(size, w);
		long v = 0;
		for (int i = 0; i < (int)(size / sizeof(long)); ++i)
		{
			wrapWrite(_table.get(w) + i, &v, sizeof(long), w);
		}
		wrapClose(w);
	}

	~WrapMap()
	{
		int size = _size.get();
		for (int i = 0; i < size; ++i)
		{
			HashEntry* entry;
			wrapRead(&entry, _table.get()+i, sizeof(HashEntry*), 0);

			while (entry != 0)
			{
				HashEntry* prev = entry;
				entry = entry->next();
				pfree(prev);
			}
		}
		pfree(_table.get());
	}

	inline int HashFunc(int key)
	{
		return key % TABLE_SIZE;
	}

	inline V Get(K key, WRAPTOKEN w = 0)
	{
		int hash_val = HashFunc(key);
		HashEntry* entry;
		wrapRead(&entry, _table.get(w)+hash_val, sizeof(HashEntry*), w);

		while (entry != 0)
		{
			if (entry->key(w) == key)
			{
				return entry->value(w);
			}
			entry = entry->next(w);
		}
		return -1;
	}

	inline void Put(K key, V value, WRAPTOKEN wt = 0)
	{
		//fprintf(stderr, "P");
		int hash_val = HashFunc(key);
		HashEntry* prev = 0;

		WRAPTOKEN w = wt;
		if (wt == 0)
			w = wrapOpen();

		HashEntry* entry;
		wrapRead(&entry, _table.get(w)+hash_val, sizeof(HashEntry*), w);

		//printf("entry=%p hashval=%d _table=%p\n", entry, hash_val, _table.get(w));

		while (entry != 0 && entry->key(w) != key)
		{
			prev = entry;
			entry = entry->next(w);
		}

		if (entry == 0)
		{
			//printf("A\n");
			entry = (HashEntry *)pmalloc(sizeof(HashEntry));
			entry->setKey(key, w);
			entry->setValue(value, w);
			entry->setNext(NULL, w);
			//printf("B\n");
			if (prev == 0)
			{
				//printf(". entry=%p\n", entry);
				wrapWrite(_table.get(w) + hash_val, &entry, sizeof(HashEntry *), w);
			}
			else
			{
				prev->setNext(entry, w);
			}
		}
		else
		{
			//printf("C\n");
			//printf("entry key=%d value=%d newvalue=%d\n", key, entry->value(w), value);
			entry->setValue(value, w);
		}
		if (wt == 0)
			wrapClose(w);
	}

	inline void Remove(K key, WRAPTOKEN wt = 0)
	{
		int hash_val = HashFunc(key);
		WRAPTOKEN w = wt;
		if (wt == 0)
			w = wrapOpen();
		HashEntry* entry;
		wrapRead(&entry, _table.get(w)+hash_val, sizeof(HashEntry*), w);

		HashEntry* prev = 0;
		while (entry != 0)
		{
			if (entry->key(w) == key)
			{
				break;
			}
			prev = entry;
			entry = entry->next(w);
		}
		if (entry != 0)
		{
			if (prev == 0)
			{
				HashEntry *n = entry->next(w);
				wrapWrite(_table.get(w) + hash_val, &n, sizeof(HashEntry *), w);
			}
			else
			{
				prev->setNext(entry->next(w), w);
			}
			pfree(entry);
		}
		if (wt == 0)
			wrapClose(w);
	}

private:
	// Hash table
	WrapVar<HashEntry**> _table;
	WrapVar<int> _size;
};

#endif
