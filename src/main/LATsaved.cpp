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
#include "string.h"
#include "pmem.h"

__thread struct LAT::Entry LAT::m_entries[4096];
__thread char LAT::m_data[16384];
__thread int LAT::m_lastData = 0;
__thread int LAT::m_lastIndex = 0;

LAT::LAT(WrapLogManager *manager, int size)
{
	m_lastIndex = 0;
	m_lastData = 0;
	printf("LAT\n");
	// @TODO not configurable
	printf("AliasTable Size = 4096 %d NOT CONFIGURABLE\n", size);
}

LAT::~LAT()
{
	restoreAllElements();
}


size_t LAT::readInto(void *ptr, const void *src, size_t size)
{
	memcpy(ptr, src, size);

	//  TODO, optimize here.
	//  Copy any aliased data.
	if (isInPMem(ptr))
	{
		for (int i = m_lastIndex; i >= 0; i--)
		{
			if (isContained(m_entries[m_lastIndex].ptr, m_entries[m_lastIndex].size, ptr, size))
			{

			}
		}
	}
	return size;
}

void LAT::write(void *ptr, void *src, int size)
{
	if (isInPMem(ptr))
	{
		m_entries[m_lastIndex].key = ptr;
		m_entries[m_lastIndex].size = size;

		if (size <= 8)
		{
			//m_entries[m_lastIndex].value = *((uint64_t*)src);
			memcpy(&(m_entries[m_lastIndex].value), src, size);
		}
		else
		{
			m_entries[m_lastIndex].value = (uint64_t)&(m_data[m_lastData]);
			m_lastData += size;
		}
		m_lastIndex++;
		printf("LAT write %p %p %d %ld\n", ptr, src, size, *(uint64_t*)src);
	}
	else
	{
		memcpy(ptr, src, size);
	}
}

void *LAT::load(void *ptr)
{
	if (isInPMem(ptr))
	{
		for (int i = m_lastIndex; i >= 0; i--)
		{
			if (isContained(m_entries[m_lastIndex].ptr, m_entries[m_lastIndex].size, ptr, size))
			{

			}
		}
	}
	else
	{
		return ptr;
	}
}

void LAT::store(void *ptr, uint64_t value, int size)
{
	if (isInPMem(ptr))
	{
		m_entries[m_lastIndex].key = ptr;
		m_entries[m_lastIndex].size = size;
		assert(size <= 8);
		memcpy(&(m_entries[m_lastIndex].value), &value, size);

		m_lastIndex++;

		printf("LAT store %p %ld %d\n", ptr, value, size);
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
	printf("LAT copy %p %p %d %ld\n",m_entries[i].key, (void*)m_entries[i].value, m_entries[i].size, m_entries[i].value);

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
	for (int i = 0; i < m_lastIndex; i++)
		writebackEntry(i);
	sfence();
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
