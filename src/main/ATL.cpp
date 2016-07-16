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

#include "ATL.h"
#include "WrapImpl.h"
#include "memtool.h"
#include "string.h"

//  TODO need to implement tls tables!!  This impl will only support one thread at a time.
//  Need a table per thread.

ATL::ATL(WrapLogManager *manager, int size, int thresh): AliasTableHash(manager, size, 0)
{
	m_options = -1;
	printf("ATL\n");
	printf("AliasTable Size = %d\n", size);
}

ATL::~ATL()
{
	restoreAllElements();
}

void ATL::onWrapOpen(int wrapToken)
{
	if (m_options == -1)
	{
		m_options = WrapImpl::getOptions();
	}
	AliasTableHash::onWrapOpen(wrapToken);
	//printf("o");
	//fflush(stdout);
}

void ATL::restoreToCacheHierarchy()
{
	for (int i = 0; i < m_arraySize; i++)
	{
		if (m_entries[i].key != 0)
		{
			if (m_entries[i].size == 4)
				*(int *)(m_entries[i].key) = m_entries[i].value;
			else if (m_entries[i].size == 8)
				*(uint64_t *)(m_entries[i].key) = m_entries[i].value;
			else if (m_entries[i].size == 2)
				*(short *)(m_entries[i].key) = m_entries[i].value;
			else if (m_entries[i].size == 1)
				*(char *)(m_entries[i].key) = m_entries[i].value;
			else
				assert(0);
				//memcpy((void*)(m_entries[i].key), (void*)(m_entries[i].value), m_entries[i].size);
		}
	}
	sfence();
	for (int i = 0; i < m_arraySize; i++)
	{
		if (m_entries[i].key != 0)
		{
			clwb((void*)m_entries[i].key, &(m_entries[i].value), m_entries[i].size);
			//  Clear.
			m_entries[i].key = 0;
			m_entries[i].value = 0;
		}
	}
	p_msync();
}

void ATL::onWrapClose(int wrapToken, WrapLogger *log)
{
	AliasTableHash::onWrapClose(wrapToken, log);
	/*
	if (m_options == 1)
	{
		assert(0);
	}
	else if (m_options == 2)
	{
		//  Restore
		restoreAllElements();
		clearHash();
	}
	else
	{
		restoreToCacheHierarchy();
	}
	*/
	//printf("c");
	//fflush(stdout);
	restoreAllElements();
	//clearHash();
}
