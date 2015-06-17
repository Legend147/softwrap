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

#include "WrapMap.h"
#include "wrap.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	WrapMap<long, long> map;
	int length = 1024;

	int i = 0;
	for (i = 0; i < length; i++)
	{
		printf("%d\n", i);
		fflush(stdout);
		map.Put(i, i);
	}
	//return 0;

	printf("remapping\n");
	for (i = 0; i < length; i++)
	{
		map.Put(i, i*2);
	}

	printf("getting\n");
	for (i = 0; i < length; i++)
	{
		printf("%d->%ld\n", i, map.Get(i));
	}

	printf("removing\n");
	for (i = 0; i < length; i++)
	{
		printf("%d\n", i);
		map.Remove(i);
	}

	/*
	for (i = length; i < 2*length; i++)
	{
		printf("add %d\n", i);
		a.add(i);
	}
	for (;i < 3*length; i++)
	{
		printf("set %d %d\n", i, i);
		a.set(i, i);
	}
	for (i = 0; i < 3*length; i++)
	{
		printf("a[%d]=%d\n", i, wrapReadInt(&a[i],0));
	}
	*/
}
