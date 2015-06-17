/*BEGIN_LEGAL
Intel Open Source License

Copyright (c) 2002-2012 Intel Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
//
// @ORIGINAL_AUTHOR: Artur Klauser
//

/*! @file
 *  This file contains an ISA-portable PIN tool for functional simulation of
 *  instruction+data TLB+cache hieraries
 */

/**
 * Modified for wrap.
 */

typedef UINT32 CACHE_STATS; // type of cache hit/miss counters

void cycle(int c);
void memoryRead(ADDRINT addr, int size);
void memoryWrite(void *addr, int size);

#include <assert.h>
#include "pin_cache_wrap.H"
#include "debug.h"

	namespace ITLB
	{
	// instruction TLB: 4 kB pages, 32 entries, fully associative
	const UINT32 lineSize = 4*KILO;
	const UINT32 cacheSize = 32 * lineSize;
	const UINT32 associativity = 32;
	const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

	const UINT32 max_sets = cacheSize / (lineSize * associativity);
	const UINT32 max_associativity = associativity;

	typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
	}
	LOCALVAR ITLB::CACHE itlb("ITLB", ITLB::cacheSize, ITLB::lineSize, ITLB::associativity);

	namespace DTLB
	{
	// data TLB: 4 kB pages, 32 entries, fully associative
	const UINT32 lineSize = 4*KILO;
	const UINT32 cacheSize = 32 * lineSize;
	const UINT32 associativity = 32;
	const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

	const UINT32 max_sets = cacheSize / (lineSize * associativity);
	const UINT32 max_associativity = associativity;

	typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
	}
	LOCALVAR DTLB::CACHE dtlb("DTLB", DTLB::cacheSize, DTLB::lineSize, DTLB::associativity);

	namespace IL1
	{
	// 1st level instruction cache: 32 kB, 64 B lines, 4-way associative
	const UINT32 cacheSize = 32*KILO;
	const UINT32 lineSize = 64;
	const UINT32 associativity = 4;
	const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_NO_ALLOCATE;

	const UINT32 max_sets = cacheSize / (lineSize * associativity);
	const UINT32 max_associativity = associativity;

	typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
	}
	LOCALVAR IL1::CACHE il1("L1 Instruction Cache", IL1::cacheSize, IL1::lineSize, IL1::associativity);

	namespace DL1
	{
	const UINT32 cacheSize = 32*KILO;
	const UINT32 lineSize = 64;
	const UINT32 associativity = 8;
	const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

	const UINT32 max_sets = cacheSize / (lineSize * associativity);
	const UINT32 max_associativity = associativity;

	//typedef CACHE_DIRECT_MAPPED(max_sets, allocation) CACHE;
	typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
	}
	LOCALVAR DL1::CACHE dl1("L1 Data Cache", DL1::cacheSize, DL1::lineSize, DL1::associativity);

	namespace UL2
	{
	// 2nd level unified cache: 256K, 64 B lines, 8 way
	const UINT32 cacheSize = 256*KILO;
	const UINT32 lineSize = 64;
	const UINT32 associativity = 8;
	const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

	const UINT32 max_sets = cacheSize / (lineSize * associativity);
	const UINT32 max_associativity = associativity;

	typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
	}
	LOCALVAR UL2::CACHE ul2("L2 Unified Cache", UL2::cacheSize, UL2::lineSize, UL2::associativity);

	namespace UL3
	{
	// 3rd level unified cache: 16 MB, 64 B lines, 16 way
	const UINT32 cacheSize = 16*MEGA;
	const UINT32 lineSize = 64;
	const UINT32 associativity = 16;
	const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

	const UINT32 max_sets = cacheSize / (lineSize * associativity);
	const UINT32 max_associativity = associativity;

	typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
	}
	LOCALVAR UL3::CACHE ul3("L3 Unified Cache", UL3::cacheSize, UL3::lineSize, UL3::associativity);


class CacheSim
{
public:
	const static int l1cycles = 1;//2;
	const static int ul2cycles = 10;
	const static int ul3cycles = 40;

	void printStats()
	{
		std::cerr << itlb;
		std::cerr << dtlb;
		std::cerr << il1;
		std::cerr << dl1;
		std::cerr << ul2;
		std::cerr << ul3;
	}

	LOCALFUN void evict(CACHE_TAG evicted)
	{
		unsigned long l = (ADDRINT)evicted;
		void *p = (void *)( l * UL3::lineSize);
		//Debug("E %lx %p\n", l, p);
		//  TODO do we write the whole cache line or what's the granularity?
		memoryWrite(p, UL3::lineSize);
	}

	LOCALFUN int Ul3Access(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType, CACHE_TAG evictedFromL2)
	{
			cycle(ul3cycles - ul2cycles - l1cycles - 1);

			if (evictedFromL2 != 0)
			{
				ul3.MarkDirty(evictedFromL2);
			}

			CACHE_TAG evicted;

			// second level unified cache
			const BOOL ul3Hit = ul3.AccessSingleLine(addr, accessType, &evicted);

			if (!ul3Hit)
			{
				//printf("-%p \n", (void*)addr);
				memoryRead(addr, size);
				if (evicted != 0)
				{
					//Debug("E3 %p\n", (void *)(((unsigned long)evicted)*UL3::lineSize));
					assert(!ul3Hit);
					evict(evicted);
				}
			}
			else
			{
				assert(evicted == (CACHE_TAG)0);
			}
			return 1;
	}


	LOCALFUN int Ul2Access(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType, CACHE_TAG evictedFromL1)
	{
		cycle(ul2cycles - l1cycles - 1);

		if (evictedFromL1 != 0)
		{
			ul2.MarkDirty(evictedFromL1);
		}

		CACHE_TAG evicted;

		// second level unified cache
		const BOOL ul2Hit = ul2.AccessSingleLine(addr, accessType, &evicted);
		if (evicted != 0)
		{
			//Debug("e2-%p-", (void*)addr);
			assert(!ul2Hit);
		}

	    // third level unified cache
	    if ( ! ul2Hit)
	    	return Ul3Access(addr, size, accessType, evicted);

	    return 1;
	}


	//  TODO loads/stores can cross cache lines!!!

	LOCALFUN int InsRef(ADDRINT addr)
	{
		cycle(l1cycles - 1);

		CACHE_TAG evicted;

	    const UINT32 size = 1; // assuming access does not cross cache lines
	    const ACCESS_TYPE accessType = ACCESS_TYPE_LOAD;

	    // ITLB
	    itlb.AccessSingleLine(addr, accessType, &evicted);

	    // first level I-cache
	    const BOOL il1Hit = il1.AccessSingleLine(addr, accessType, &evicted);
	    assert((ADDRINT)evicted == 0);

	    // second level unified Cache
	    if ( ! il1Hit)
	    	return Ul2Access(addr, size, accessType, evicted);
	    return 1;
	}


	LOCALFUN int MemRefSingle(ADDRINT ar, UINT32 size, ACCESS_TYPE accessType)
	{
		assert(size <= 64);
		cycle(l1cycles - 1);

		CACHE_TAG evicted = 0;

		//ADDRINT ar = (ADDRINT)addr;
		//printf(" %p %lu %p ", addr, ar, (void *)ar);

		// DTLB
	    dtlb.AccessSingleLine(ar, ACCESS_TYPE_LOAD, &evicted);

	    // first level D-cache
	    const BOOL dl1Hit = dl1.AccessSingleLine(ar, accessType, &evicted);

	    if (evicted != 0)
	    {
	    	//Debug("e1-%p-", (void *)addr);
	    	assert(!dl1Hit);
	    }

	    // second level unified Cache
	    if ( ! dl1Hit)
	    	return Ul2Access(ar, size, accessType, evicted);
	    return 1;
	}


	LOCALFUN VOID clflush(ADDRINT addr)
	{
		//Debug("flush!");
		cycle(ul3cycles);

		//  Flush itlb, il1, and dtlb.
		bool flushed = itlb.ClFlush(addr);
		assert(!flushed);
		flushed = il1.ClFlush(addr);
		assert(!flushed);
		dtlb.ClFlush(addr);

		//  Flush in order of closest to memory to processor.  Closest to proc will have latest dirty copy if any.
		bool a = ul3.ClFlush(addr);
		bool b = ul2.ClFlush(addr);
		bool c = dl1.ClFlush(addr);

		if (a || b || c)
		{
			memoryWrite((void *)addr, UL3::lineSize);
			Debug("F");
		}

	}

};

