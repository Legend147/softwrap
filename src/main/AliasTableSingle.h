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

#ifndef __AliasTableSingle_h
#define __AliasTableSingle_h

using namespace std;
#include "wrap.h"

class AliasTableSingle
{
public:
	AliasTableSingle(int size);
	~AliasTableSingle();

	void resetTable();

#define getHash(ptr)	((((unsigned long)ptr) >> 2) & ((1<<14)- 1))


	inline void onWrite(void *ptr, void *src, int size, WRAPTOKEN w)
	{
	char *alias = (char *)getAlias(ptr);
	if (alias == NULL)
	{
		alias = _data + _dataOffset;
		_dataOffset += sizeof(TableEntry) + size;
		assert(_dataOffset < _dataSize);
		int index = getHash(ptr);
		TableEntry *h = _hash[index];
		TableEntry *aliasEntry = (TableEntry*)alias;
		aliasEntry->destinationPtr = ptr;
		aliasEntry->size = size;
		aliasEntry->nextBucket = h;
		_hash[index] = aliasEntry;
	}
	memcpy(alias+sizeof(TableEntry), src, size);
};

   inline void *onRead(void *ptr, int size, WRAPTOKEN w)
{
	char *alias = getAlias(ptr);
	if (alias == NULL)
		return ptr;
	return alias + sizeof(TableEntry);
};


	int processTable();

private:
	char *_data;
	int _dataSize;
	int _dataOffset;



	inline char *getAlias(void *ptr)
{
	//int index = getHash(ptr);
	//printf("i=%d ptr=%p\n", index, ptr);
	TableEntry *h = _hash[getHash(ptr)];

	while (true)
	{
		if (h == NULL)
			return NULL;
		assert(h->destinationPtr != NULL);
		if (h->destinationPtr == ptr)
			return (char *)h;
		//  Else check the next bucket.
		h = h->nextBucket;
	}
	assert(0);
	return NULL;
};

#define MapSize (1<<14)
	typedef struct _entry
	{
		void *destinationPtr;
		struct _entry *nextBucket;
		int size;
	} TableEntry;
	TableEntry **_hash;

#define BloomSize (1<<10)
#define BloomBits 10

	unsigned long BloomField[BloomSize];

	inline int getBloom(void *ptr)
	{
		return ((unsigned long)ptr >> 2) & ((BloomSize << 6) - 1);
	};

	inline int checkBloom(void *ptr)
	{
		int i = getBloom(ptr);
		return (BloomField[i >> 6]&(1<<(i&63)));
	};

	inline void setBloom(void *ptr)
	{
		int i = getBloom(ptr);
		BloomField[i >> 6] |= (1 << (i&63));
	};

};

#endif
