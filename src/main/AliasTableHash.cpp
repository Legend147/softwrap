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

#include <assert.h>
#include "AliasTableHash.h"
#include "memtool.h"
#include <unistd.h>
#include <string.h>


AliasTableHash::AliasTableHash(WrapLogManager *manager, int size, int mt): LFHT(size, mt)
{
	//m_logManager = manager;
	m_restoreTime = 0;
	m_logTime = 0;
	m_totalRestores = 0;
	m_maxWrapToken = -1;
	__sync_synchronize();
};

AliasTableHash::~AliasTableHash()
{
	//  TODO some assert that all the elements are empty.
};

void AliasTableHash::clearHash()
{
	clear();
}

double AliasTableHash::getAvgRestoreTime()
{
	double d = m_restoreTime + m_logTime;
	d /= m_totalRestores;
	return d;
}

const char *AliasTableHash::getDetails()
{
	static char c[80];

	sprintf(c, "size= %d", m_numItems);
	return c;
}

void AliasTableHash::onWrapOpen(int wrapToken)
{
	//__sync_add_and_fetch(&m_numOpen, 1);
	//m_numOpen++;
	//printf("numOpen=%d  size=%d items=%d\n", m_numOpen, m_arraySize, m_numItems);
};

//int m_in = 0;
void AliasTableHash::onWrapClose(int wrapToken, WrapLogger *log)
{
	//printf("m_in=%d ", m_in);
	//fflush(stdout);
	//m_logQueue.push_back(log);
	//__sync_sub_and_fetch(&m_numOpen, 1);
	//m_numOpen--;

	//  TODO convert to atomic test and set!!!
	m_maxWrapToken = dmax(wrapToken, m_maxWrapToken);
};

void *AliasTableHash::read(void *ptr, int size)
{
	void *v = (void*)GetItem((uint64_t)ptr);

	return v;
};

void AliasTableHash::write(void *ptr, void *src, int size)
{
	//assert(0);

	//m_in++;
	//printf("w");
	//fflush(stdout);
	SetItem((uint64_t)ptr, (uint64_t)src, size);
};

void *AliasTableHash::load(void *ptr)
{
	//assert(0);
	//return (uint64_t)GetItem((uint64_t)ptr);
	void *v = GetItemAddress((uint64_t)ptr);

	return v;
};

void AliasTableHash::store(void *ptr, uint64_t value, int size)
{
	//m_in++;
	SetItem((uint64_t)ptr, value, size);
};


void AliasTableHash::restoreAllElements()
{
	m_totalRestores++;
	long t = getNsTime();
	//printf("r");
	//m_in = 0;
	for (int i = 0; i < m_arraySize; i++)
	{
		if (m_entries[i].key != 0)
		{
			//  Streaming Store.
			if (m_entries[i].size == 4)
			{
				uint32_t v = m_entries[i].value;
				ntstore((void*)(m_entries[i].key), &v, 4);
			}
			else if (m_entries[i].size == 2)
			{
				//unsigned short u = m_entries[i].value;
				//ntstore((void*)(m_entries[i].key), &u, 2);
				assert(0);
			}
			else
			{
				//  TODO FOR NOW MUST CHECK TO MAKE SURE ITS A 8 byter!  TODO TODO Object encode size into table
				//assert(m_entries[i].size == 8);
				if (m_entries[i].size > 8)
				{
					//printf("restoring\n");
					//  TODO
					//ntstore((void*)(m_entries[i].key), (void*)(m_entries[i].value), m_entries[i].size);
					memcpy((void*)(m_entries[i].key), (void*)(m_entries[i].value), m_entries[i].size);
					//printf("copied\n");
				}
				else
					ntstore((void*)(m_entries[i].key), &(m_entries[i].value), m_entries[i].size);
			}
			//  Loads from other processors can be reordered with respect to these stores so sfence.
			//  Maybe look at storing all to home, then zeroing out.
			asm volatile("sfence" : : : "memory");  //now use sfence();
			m_entries[i].key = 0;
	//		m_numItems--;
			//			printf("%d ", m_numItems);
		}

	}
	//	usleep(1030);

	//p_msync();


	/*
	 for (int i = 0; i < m_arraySize; i++)
	{
		if (m_entries[i].key != 0)
		{
			//  Streaming Store.
			m_entries[i].key = 0;
			m_numItems--;
		}
	}
	__sync_synchronize();
	*/

	long t2 = getNsTime();
	m_restoreTime += (t2 - t);

	/*
	while (!m_logQueue.empty())
	{
		WrapLogger *log = m_logQueue.front();
		m_logManager->removeLog(log);
		//if (log != NULL)
			//delete log;
		m_logQueue.pop_front();
	}
	 */

	//  TODO LOG REMOVAL!!

	/*
	p_msync();
	for (int i = 0; i < m_arraySize; i++)
	{
		if (m_entries[i].key != 0)
		{
			//  Streaming Store.
			m_entries[i].key = 0;
			m_numItems--;
			//			printf("%d ", m_numItems);
		}
	}
	*/
	m_logTime += (getNsTime() - t2);
	m_numItems = 0;
	clearBloom();
	p_msync();

};

