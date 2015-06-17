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

#include "virt2phys.h"
#include "wrap.h"
#include "memtool.h"

void determineActual(long *p, int n)
{
	long l;
	for (long i = 0; i < (n*1000); i++)
	{
		*p = i;
		clflush(p);
		l = *p;
		if (l % 1000 == 0)
			printf(".");
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	Virt2Phys v;
	int num = 16;
	long **p;

	if (argc == 1)
		num = 16;
	if (argc == 2)
		num = atoi(argv[1]);
	if ((num <= 0) || (argc > 2))
	{
		fprintf(stderr, "Usage: %s [numberpointers]\n", argv[0]);
		return -1;
	}
	p = (long**)malloc(sizeof(long*) * num);

	for (int i = 0; i < num; i++)
	{
		p[i] = (long*)mallocAligned(sizeof(long));
		for (int z = 0; z < 100; z++)
		{
			*(p[i]) = rand();
			clflush(p[i]);
			msync();
		}
		void *phys = v.GetPhysicalAddress(p[i]);

		printf("virtual pointer = %p   => \tphysical pointer = %p  \t%d\n", p[i], phys, i+1);

		//  Noise
		for (int j = 0; j < 1000; j++)
		{
			long *temp = (long *)mallocAligned(sizeof(long));
			*temp = 12345;
			clflush(temp);
		}
	}

	for (int x = 0; x < 100; x++)
	{
		for (int y = 0; y < num; y++)
		{
			determineActual(p[y], y+1);
		}
	}

	return 0;
}
