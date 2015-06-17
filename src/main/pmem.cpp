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

#include "pmem.h"
#include <stdio.h>
#include <vector>

struct chunk {void *start; int size;};

std::vector<struct chunk> pmem;

int isInPMem(void *v)
{
	for (unsigned int i = 0; i < pmem.size(); i++)
	{
		long start = (long)pmem[i].start;
		long lv = (long)v;

		if ((start <= lv) && ((start + pmem[i].size) >= lv))
			return 1;
	}
	//printf("not in pmem %p, %d entries\n", v, (int)pmem.size());
	return 0;
}

int find(void *v)
{
	for (unsigned int i = 0; i < pmem.size(); i++)
	{
		if (v == pmem[i].start)
			return i;
	}
	return -1;
}

void freedPMem(void *v)
{
	printf("freed %p\n", v);
	pmem.erase(pmem.begin() + find(v));
}

void allocatedPMem(void *start, int size)
{
	printf("allocated %p %d\n", start, size);
	pmem.push_back(chunk());
	pmem[pmem.size()-1].start = start;
	pmem[pmem.size()-1].size = size;
}
