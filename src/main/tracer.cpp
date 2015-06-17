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

#include "tracer.h"
#include "memtool.h"
#include "virt2phys.h"
#include "strings.h"

//  Gets the specified bit and returns its value 0 or 1.
#define GETBIT(x, bit)		((x >> bit) & 1)
//  Initializes a number with the specified bit.
#define BITWISE(x, bit)		(x << bit)
//  Strip out only the bit in interest.
#define STRIPBIT(x, bit)	(x & (1 << bit))


unsigned long physicalToActual(unsigned long address)
{
	unsigned long actual = address & (0xFFF << 24);
	actual -= (64 << 24);

	/*
	  For middle 12 bits:
	  First 2 bits (23, 22) same
	  Next 3 bits (21, 20, 19) take first bit (21) and put at end (20, 19, 21)
	  Next 3 bits (18, 17, 16) same
	  Last 4 bits (15-12) last bit (12) first
	*/
	//  First 2 bits (23, 22) same
	actual |= STRIPBIT(address, 23) | STRIPBIT(address, 22);
	//  Next 3 bits (21, 20, 19) take first bit (21) and put at end (20, 19, 21)
	actual |= STRIPBIT(address, 21) >> 2;
	actual |= STRIPBIT(address, 20) << 1;
	actual |= STRIPBIT(address, 19) << 1;
	//  Next 3 bits (18, 17, 16) same
	actual |= STRIPBIT(address, 18) | STRIPBIT(address, 17) | STRIPBIT(address, 16);
	//  Last 4 bits (15-12) last bit (12) first
	actual |= STRIPBIT(address, 15) >> 1;
	actual |= STRIPBIT(address, 14) >> 1;
	actual |= STRIPBIT(address, 13) >> 1;
	actual |= STRIPBIT(address, 12) << 3;

	actual |= address & 0xFFF;
	return actual;

}

#ifdef TRACER

Virt2Phys VirtualToPhysical;
struct timeval startTime;

#include <sys/time.h>
#include <stdio.h>

void StartPhysicalTracer()
{

	fprintf(stderr, "Press [Enter] to start the test....\n"); getchar();

	//  Get and flush a tracer for program start.
	//  Use our mallocAligned since it will align.
	void *v = mallocAligned(sizeof(long));
	long *l = (long *)v;
	for (int i = 0; i < 10000; i++)
	{
		*l = rand();
		clflush(l);
		msync();
	}

	printf("PROGRAM START %p PHYSICAL MEMORY ADDRESS TOKEN %p\n", v, VirtualToPhysical.GetPhysicalAddress(v));
	if (gettimeofday(&startTime, NULL) < 0)
	{
		perror("Error getting the time of day");
		abort();
	}
}

void EndPhysicalTracer()
{

	//  Get and flush a tracer for program end.
	//  Use our mallocAligned since it will align.
	void *v = mallocAligned(sizeof(long));
	long *l = (long *)v;
	for (int i = 0; i < 10000; i++)
	{
		*l = rand();
		clflush(l);
		msync();
	}

	printf("PROGRAM END %p PHYSICAL MEMORY ADDRESS TOKEN %p\n", v, VirtualToPhysical.GetPhysicalAddress(v));

	//  Determine execution time.
	struct timeval endTime;
	if (gettimeofday(&endTime, NULL) < 0)
	{
		perror("Error getting the time of day");
	}
	else
	{
		long ms = ((endTime.tv_sec - startTime.tv_sec) * 1000000) +
		(endTime.tv_usec - startTime.tv_usec);
		double d = (double)ms / 1000000.0;
		printf("Execution time %f\n", d);
	}
	fprintf(stderr, "Done with program.  End tracer.\n");
	fflush(stderr);
}

void persistentNotifyTracer(void *v, size_t size)
{
	printf("pmalloc virtual pointer = %p with size = %lu\n", v, size);
	char *c = (char *)v;
	//  Lets write zeros to make sure it's in physical memory and we can obtain a physical address.
	bzero(v, size);

	//  Have to print out page by page.  Hopefully it will be contiguous, but have to do this anyway.
	for (int sz = size; sz > 0; sz -= PAGESIZE)
	{
		int n = (sz>PAGESIZE)?PAGESIZE:sz;
		clflush((void *)c);
		msync();
		printf("SCM ALLOCATED %p %d\n", VirtualToPhysical.GetPhysicalAddress((void *)(c)), n);
		c += n;
	}
}

int pfreeNotifyTracer(void *ptr)
{
	return 1;
}

#else


void StartPhysicalTracer()
{

}

void EndPhysicalTracer()
{

}

void persistentNotifyTracer(void *v, size_t size)
{

}

int pfreeNotifyTracer(void *ptr)
{
	return 0;
}

#endif
