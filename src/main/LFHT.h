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

#ifndef __LFHT_H
#define __LFHT_H

#include <stdlib.h>
#include <stdint.h>

class LFHT
{
public:
	LFHT(int size, int mt = 1);
	~LFHT();
	uint64_t GetItem(uint64_t key, int *size = NULL);
	void *GetItemAddress(uint64_t key);
	void SetItem(uint64_t key, uint64_t value, int size);
	int GetMaxSize();
	int GetNumItems();
	void clear();

protected:
	struct Entry
	{
	    uint64_t key;
	    uint64_t value;
	    int size;
	};
	Entry *m_entries;
	int m_arraySize;
	volatile int m_numItems;
	//  flag for multi-threaded
	int m_mt;

#define BLOOM	1

#ifdef BLOOM
#define BloomSize (1<<10)
#define BloomBits 10

	unsigned long BloomField[BloomSize];

	inline int getBloom(uint64_t ptr)
	{
		return (ptr >> 2) & ((BloomSize << 6) - 1);
	};

	inline int checkBloom(uint64_t ptr)
	{
		int i = getBloom(ptr);
		return (BloomField[i >> 6]&(1<<(i&63)));
	};

	inline void setBloom(uint64_t ptr)
	{
		int i = getBloom(ptr);
		BloomField[i >> 6] |= (1 << (i&63));
	};

	inline void clearBloom()
	{
		for (int i = 0; i < BloomSize; i++)
			BloomField[i] = 0;
	}
#else
#ifdef SIMPLEBLOOM
#define BloomSize 4
#define BloomBits 10

	unsigned long BloomField[BloomSize];

	inline int getBloom(uint64_t ptr)
	{
		return (ptr >> 2) & ((BloomSize << 6) - 1);
	};

	inline int checkBloom(uint64_t ptr)
	{
		int i = getBloom(ptr);
		return (BloomField[i >> 6]&(1<<(i&63)));
	};

	inline void setBloom(uint64_t ptr)
	{
		int i = getBloom(ptr);
		BloomField[i >> 6] |= (1 << (i&63));
	};

	inline void clearBloom()
	{
		for (int i = 0; i < BloomSize; i++)
			BloomField[i] = 0;
	}
#else
#define clearBloom()
#define checkBloom(x)	1
#define setBloom(x)
#endif
#endif
};

#endif
