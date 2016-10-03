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

#include "LAT.h"
#include "memtool.h"
#include "debug.h"
#include "string.h"
#include "pmem.h"

#if 0
#define latPMemCheck(ptr) isInPMem(ptr)
#else
#define latPMemCheck(ptr)	1
#endif

#define LAT_READ_OPT 1

__thread struct LAT::Entry LAT::m_entries[MAX_LAT_ENTRIES];
__thread char LAT::m_data[MAX_LAT_DATA];
__thread int LAT::m_lastData = 0;
__thread int LAT::m_lastIndex = 0;

#define DebugLAT 0

LAT::LAT(WrapLogManager *manager, int size)
{
	m_lastIndex = 0;
	m_lastData = 0;
	char *c = getenv("DelayWriteBack");

	m_delayWriteBack = !((c==NULL) || (0!=strcmp("1", c)));
	printf("LAT\n");
	// @TODO not configurable
	printf("AliasTable Size = %d %d NOT CONFIGURABLE\n", MAX_LAT_ENTRIES, size);
	printf("LAT DelayWriteBack= %d\n", m_delayWriteBack);
}

LAT::~LAT()
{
	restoreAllElements();
}


#define min(a,b) ((a<b)?a:b)


size_t LAT::readInto(void *ptr, const void *src, size_t size)
{
	memcpy(ptr, src, size);

	//  Copy any aliased data.
	if (latPMemCheck((void*)src))
	{
		if (!m_bloom.checkBloomRange((uint64_t)src, size))
			return size;

		uint64_t entryPtr;
		size_t entrySize;

#ifdef LAT_READ_OPT
		for (int i = m_lastIndex-1; i >= 0; i--)
		{
			entryPtr = (uint64_t)m_entries[i].key;
			entrySize = m_entries[i].size;
			if (((uint64_t)src >= entryPtr) && (((uint64_t)src+size) <= (entryPtr+entrySize)))
			{
				//  Destination has no offset is the src calculate the offset to entry pointer.
				uint64_t offset = (uint64_t)src - entryPtr;
				uint64_t alias;
				if (entrySize <= 8)
					alias = (uint64_t)&(m_entries[i].value);
				else
					alias = m_entries[i].value;
				memcpy(ptr, (void*)(alias+offset), size);
				return size;
			}
			else if ((((uint64_t)ptr + size) >= entryPtr) &&
					 ((uint64_t)ptr < (entryPtr+entrySize)))
				break;

		}
#endif

#ifdef DebugLAT
		Debug("LAT readInto %p %p %ld\n", ptr, src, size);
		Debug("val = %lx\n", *(uint64_t*)src);
#endif

		for (int i = 0; i < m_lastIndex; i++)
		{
			entryPtr = (uint64_t)m_entries[i].key;
			entrySize = m_entries[i].size;

#ifdef DebugLAT
			//Debug("%d %p %ld\n", i, (void*)entryPtr, entrySize);
#endif

			//  TODO, can prob remove this case.
			if ((entryPtr == (uint64_t)src) && (size <= entrySize))
			{
				if (entrySize <= 8)
					memcpy(ptr, &(m_entries[i].value), min(entrySize, size));
				else
					memcpy(ptr, (void*)(m_entries[i].value), min(entrySize, size));
			}
			//  TODO if removing above put <= here
			else if ((entryPtr <= (uint64_t)src) && ((entryPtr+entrySize) >= (uint64_t)src))
			{
				//  Destination has no offset is the src calculate the offset to entry pointer.
				uint64_t offset = (uint64_t)src - entryPtr;
				uint64_t alias;
				if (entrySize <= 8)
					alias = (uint64_t)&(m_entries[i].value);
				else
					alias = m_entries[i].value;
				/*
					memcpy(ptr, &(m_entries[i].value) + offset, min(entrySize-offset, size));
				else
					memcpy(ptr, (void*)(m_entries[i].value + offset), min(entrySize-offset, size));
				 */
				memcpy(ptr, (void*)(alias+offset), min(entrySize-offset, size));

#ifdef DebugLAT
				Debug("from %lx, copied %ld %ld %x\n", *(uint64_t*)alias,min(size, entrySize-offset), offset, *(char*)ptr);
#endif

			}
			else if ((entryPtr >= (uint64_t)src) && (entryPtr < ((uint64_t)src+size)))
			{
				//  It has some portion contained.
				//  Find the start of the overlap.
				//  Find destination offset.
				uint64_t destoffset = entryPtr - (uint64_t)src;
				int adj = min(size - destoffset, entrySize);

				if (entrySize <= 8)
					memcpy((void*)((uint64_t)ptr+destoffset), &(m_entries[i].value), adj);
				else
					memcpy((void*)((uint64_t)ptr+destoffset), (void*)(m_entries[i].value), adj);

#ifdef DebugLAT
				Debug("from %p %ld copied %d\n", ptr, destoffset, adj);
#endif
			}
		}
	}
#ifdef DebugLAT
	assert(memcmp(ptr, src, size) == 0);
#endif

	return size;
}

void LAT::write(void *ptr, void *src, int size)
{
#ifdef DebugLAT
	memcpy(ptr, src, size);
#endif

	if (latPMemCheck(ptr))
	{

		assert(m_lastIndex < MAX_LAT_ENTRIES);
		m_bloom.setBloomRange((uint64_t)ptr, size);

		m_entries[m_lastIndex].key = ptr;
		m_entries[m_lastIndex].size = size;
		void *alias;

		if (size <= 8)
		{
			//m_entries[m_lastIndex].value = *((uint64_t*)src);
			alias = &(m_entries[m_lastIndex].value);
		}
		else
		{
			assert((m_lastData+size) < MAX_LAT_DATA);
			alias = &(m_data[m_lastData]);
			m_entries[m_lastIndex].value = (uint64_t)alias;
			m_lastData += size;
		}
		//  Copy into alias.
		memcpy(alias, src, size);
		Debug("LAT write ptr= %p  src= %p  size= %d  val= %lx  aliasIndex= %d  alias= %p\n",
				ptr, src, size, *(uint64_t*)src, m_lastIndex, alias);
		m_lastIndex++;
	}
	else
	{
		memcpy(ptr, src, size);
	}
}

//  For primitives we return only the latest write and assert that it is entirely contained in a previous write.

uint64_t loader;

void *LAT::load(void *src)
{
	assert(0);

	if (latPMemCheck(src))
	{
		/*
		Debug("LAT load %p\n", src);

		uint64_t entryPtr;
		size_t entrySize;

		for (int i = m_lastIndex-1; i >= 0; i--)
		{
			entryPtr = (uint64_t)m_entries[i].key;
			entrySize = m_entries[i].size;
			Debug("%d %p %ld\n", i, (void*)entryPtr, entrySize);

			if ((entryPtr <= (uint64_t)src) && ((entryPtr+entrySize) > (uint64_t)src))
			{
				uint64_t offset = (uint64_t)src - entryPtr;

				void *alias;
				if (entrySize <= 8)
				{
					alias = &(m_entries[i].value) + offset;
				}
				else
				{
					alias = (void*)(m_entries[i].value + offset);
				}
				Debug("LAT load %p  aliasIndex= %d  alias= %p (size= %ld)\n", src, i, alias, entrySize);
				return alias;
			}
		}
		 */
		readInto(&loader, src, 8);
		return &loader;
	}
	else
	{
		return src;
	}
	return src;
}

void LAT::store(void *ptr, uint64_t value, int size)
{
#ifdef DebugLAT
	memcpy(ptr, &value, size);
#endif

	if (latPMemCheck(ptr))
	{
#ifdef LAT_READ_OPT
		uint64_t entryPtr;
		size_t entrySize;

		if (m_bloom.checkBloomRange((uint64_t)ptr, size))
			for (int i = m_lastIndex-1; i >= 0; i--)
			{
				entryPtr = (uint64_t)m_entries[i].key;
				entrySize = m_entries[i].size;
				if (((uint64_t)ptr >= entryPtr) && (((uint64_t)ptr+size) <= (entryPtr+entrySize)))
				{
					//  Totally contained.
					uint64_t offset = (uint64_t)ptr - entryPtr;
					uint64_t alias;
					if (entrySize <= 8)
						alias = (uint64_t)&(m_entries[i].value);
					else
						alias = m_entries[i].value;
					memcpy((void*)(alias+offset), ptr, size);
					return;
				}
				else if ((((uint64_t)ptr + size) >= entryPtr) &&
						 ((uint64_t)ptr < (entryPtr+entrySize)))
					break;
			}
#endif
		assert(m_lastIndex < MAX_LAT_ENTRIES);
		m_bloom.setBloomRange((uint64_t)ptr, size);

		m_entries[m_lastIndex].key = ptr;
		m_entries[m_lastIndex].size = size;
		assert(size <= 8);
		memcpy(&(m_entries[m_lastIndex].value), &value, size);

		Debug("LAT store ptr= %p  value= %lx  size= %d  aliasIndex= %d  alias= %p\n",
				ptr, value, size, m_lastIndex, &(m_entries[m_lastIndex].value));
		m_lastIndex++;
	}
	else
	{
		memcpy(ptr, &value, size);
	}
}


void LAT::copyEntryToCache(int i)
{
	if (m_entries[i].size <= 8)
	{
		memcpy((void*)(m_entries[i].key), (void*)&(m_entries[i].value), m_entries[i].size);
	}
	else
	{
		memcpy((void*)(m_entries[i].key), (void*)(m_entries[i].value), m_entries[i].size);
	}
	Debug("LAT copy %p %p %d %ld\n",m_entries[i].key, (void*)m_entries[i].value, m_entries[i].size, m_entries[i].value);

	return;

	//  TODO, can we get speedup.
	assert (m_entries[i].key != 0);
	if (m_entries[i].size == 4)
		*(int *)(m_entries[i].key) = m_entries[i].value;
	else if (m_entries[i].size == 8)
		*(uint64_t *)(m_entries[i].key) = m_entries[i].value;
	else if (m_entries[i].size == 2)
		*(short *)(m_entries[i].key) = m_entries[i].value;
	else if (m_entries[i].size == 1)
		*(char *)(m_entries[i].key) = m_entries[i].value;
	else
		memcpy((void*)(m_entries[i].key), (void*)(m_entries[i].value), m_entries[i].size);
}

void LAT::writebackEntry(int i)
{
	if (m_entries[i].size <= 8)
	{
		clwb((void*)(m_entries[i].key), (void*)&(m_entries[i].value), m_entries[i].size);
	}
	else
	{
		clwb((void*)(m_entries[i].key), (void*)(m_entries[i].value), m_entries[i].size);
	}
}

void LAT::restoreToCacheHierarchy()
{
	if (m_lastIndex == 0)
		return;  //  No items written.

	//  Some items to write.
	for (int i = 0; i < m_lastIndex; i++)
		copyEntryToCache(i);
	//  TODO won't need these fences when adding TLS since we have TSO.
	sfence();
	if (!m_delayWriteBack)
	{
		for (int i = 0; i < m_lastIndex; i++)
			writebackEntry(i);
		sfence();
	}
	for (int i = 0; i < m_lastIndex; i++)
	{
		m_entries[i].key = 0;
	}
	m_lastIndex = 0;
}

void LAT::restoreAllElements()
{
	restoreToCacheHierarchy();
}

void LAT::onWrapOpen(int wrapToken)
{
	m_lastIndex = 0;
	m_lastData = 0;
	m_bloom.clearBloom();
}

void LAT::onWrapClose(int wrapToken, WrapLogger *log)
{
	restoreToCacheHierarchy();
}

const char *LAT::getDetails()
{
	static char c[80];

	sprintf(c, "size= %d  data= %d", m_lastIndex, m_lastData);
	return c;
}
