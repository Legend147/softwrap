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

/*
 * memtool.c
 *
 *  Author: ergiles
 */

#include "memtool.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include "emmintrin.h"


static bool useprintfhelper = false;

//  We are going to model the memory buffer with an array.
#define MODELBUFFER
//  Each thread will get a copy of the memory buffer.
#define THREADLOCALBUFFER

bool isDebug()
{
	static bool initialized = false;

	if (!initialized)
	{
		initialized = true;
		char *s = getenv("Debug");
		if ((s != NULL) && (strcmp(s, "1") == 0))
			useprintfhelper = true;
	}

	return useprintfhelper;
}

int getInitSCMtwr()
{
	char *s = getenv("SCM_TWR");
	if ((s != NULL) && (strcmp(s, "") != 0))
	{
		return atoi(s);
	}
	return 0;
}


static int _scmTwrNanoSec = getInitSCMtwr();

void setSCMtwr(int nanosec)
{
	_scmTwrNanoSec = nanosec;
}

#ifdef MODELBUFFER

//  Local thread so we don't need locks.
#ifdef THREADLOCALBUFFER
#define TLS __thread
#define NTWRLOCK
#define NTRDLOCK
#define NTUNLOCK
#else
blahblahblah
pthread_rwlock_t ntlock;
pthread_rwlock_init(&ntlock, NULL);
#define NTWRLOCK	pthread_rwlock_wrlock(&ntlock)
#define NTRDLOCK	pthread_rwlock_rdlock(&ntlock)
#define NTUNLOCK	pthread_rwlock_unlock(&ntlock)
#endif

#define DefaultBufferSize	9
int TLS BuffSize=DefaultBufferSize;
struct rec
{
	unsigned long dest;
	long startTime;
};
struct rec TLS storeBuffer[1024];
int TLS head = 0;
int TLS tail = 0;
//unsigned long TLS lastDest = 0;


static __thread int _scmTwrFactor = 1;
void setSCMtwrFactor(int factor)
{
	_scmTwrFactor = factor;
	BuffSize = DefaultBufferSize * factor;
}

int getSCMtwr()
{
	int i = _scmTwrNanoSec / _scmTwrFactor;
	//printf("%d\n", i);
	return i;
}

void doMicroSecDelay(int microseconddelay)
{
	static struct timeval startTime, time;

	//  Delay
	//int res =
	gettimeofday(&startTime, NULL);
	//assert(res == 0);
	do {
		gettimeofday(&time, NULL);

	} while ((time.tv_sec - startTime.tv_sec) * 1000000 + (time.tv_usec - startTime.tv_usec) < microseconddelay);
}


void printfHelper ( const char * format, ... )
{
	if (isDebug())
	{
		va_list argptr;
		va_start(argptr, format);
		vprintf(format, argptr);
	}
}

void cpuid(unsigned info, unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx)
{
	__asm__(
			"cpuid;"                                            /* assembly code */
			:"=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx) /* outputs */
			 :"a" (info)                                         /* input: info into eax */
			  /* clobbers: none */
	);
}

int cpuidTest()
{
	unsigned int eax, ebx, ecx, edx;
	int i;

	for (i = 0; i < 6; ++i)
	{
		cpuid(i, &eax, &ebx, &ecx, &edx);
		printf("eax=%i: %#010x %#010x %#010x %#010x\n", i, eax, ebx, ecx, edx);
	}

	return 0;
}

void serialize()
{

	//  Statics so this doesn't go on the stack each call.
	static unsigned int eax, ebx, ecx, edx;

	//  Serializing instruction.
	//  Non-privileged so can be in user mode and only serializing instruction that can be executed in the kernel.
	//  http://objectmix.com/asm-x86-asm-370/69413-serializing-instructions.html
	cpuid(1, &eax, &ebx, &ecx, &edx);
}

void clflush(volatile void *p)
{
	asm volatile ("clflush (%0)" :: "r"(p));
}

uint64_t rdtsc()
{
	unsigned long a, d;
	asm volatile ("rdtsc" : "=a" (a), "=d" (d));
	return a | ((uint64_t)d << 32);
}

/*
void wtstore(void *dest, void *src, int size)
{
	Debug("wtstore %p %p %d\n", dest, src, size);
	if (dest != src)
	{
		//memcpy(dest, src, size);
		streamingStore(dest, src, size);
	}
}
 */
/*
long diffTime(struct timeval *end, struct timeval *start)
{
	return (end->tv_sec - start->tv_sec)*1000000 + (end->tv_usec - start->tv_usec);
}

long getlongtime()
{
	static struct timeval time;
	long l = 1000000;
	gettimeofday(&time, NULL);
	l = (time.tv_sec & 2047) * l + time.tv_usec;
	return l;
}
 */

long getSec()
{
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	return t.tv_sec;
}

long getUsTime()
{
	struct timeval t;
	//  Get start of first wrap.
	if (gettimeofday(&t, NULL) < 0)
	{
		perror("Error getting the time of day");
		abort();
	}
	return (t.tv_sec * 1000000) + t.tv_usec;
}

long startsec = getSec();

long getNsTime()
{
	struct timespec t;
	long l = 1000000000;
	clock_gettime(CLOCK_REALTIME, &t);
	l = (t.tv_sec-startsec) * l + t.tv_nsec;
	return l;
}


long _totalTime = 0;
int _totalTics = 0;
unsigned long _startTicTime = 0;

void tic()
{
	_startTicTime = getUsTime();
	_totalTics++;
}
double toc()
{
	/*  Calculate the latency in us.  */
	long t = getUsTime() - _startTicTime;
	_totalTime += t;
	return t;
}
int pinTicRate(unsigned long maxTicsPerSecond)
{
	_totalTics++;
	if (_startTicTime == 0)
	{
		_startTicTime = getUsTime();
		return 0;
	}
	long l;
	int i = -1;
	while (((l=getUsTime()) - _startTicTime) < (1000000/maxTicsPerSecond))
	{
		i++;
	}
	_startTicTime = l;
	return i;
}


int ticRate(unsigned long maxTicsPerSecond)
{
	static int y = 0;

	int x = pinTicRate(maxTicsPerSecond);
	y = x;
	return y;
}

void ticReset(int doPrint)
{
	if (doPrint)
	{
		double d = _totalTime;
		d /= _totalTics;
		//d /= 1000;
		printf("AverageTicToc thread= NA  averageCycles= NA  totalCycles= NA  totalTicTocs= %d  " \
			"totalTimeUS= %lu  averageTimeUS= %f\n", _totalTics, _totalTime, d);
	}
	_totalTime = 0;
	_totalTics = 0;
	_startTicTime = 0;
}




// core_id = 0, 1, ... n-1, where n is the system's number of cores
int attachThreadToCore(int core_id)
{
	//int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
	//if (core_id < 0 || core_id >= num_cores)
	// return EINVAL;
	//printf("numCores= %d\n", num_cores);

	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(core_id, &cpuset);

	pthread_t current_thread = pthread_self();
	sched_setaffinity(current_thread, sizeof(cpu_set_t), &cpuset);
	return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

void doNanoSecDelayDepricated(int d)
{
	/*
	timespec deadline;
	deadline.tv_sec = 0;
	deadline.tv_nsec = d;
	long s = getNsTime();
	clock_nanosleep(CLOCK_REALTIME, 0, &deadline, NULL);
	printf("%ld \n", getNsTime() - s);
	 */

	int r = 1;
	long s = getNsTime();
	while (getNsTime() - s < d)
	{
		r++;
	}
}

void nanoSecDelayUntil(long l)
{
	int r = 1;
	while (getNsTime() < l)
	{
		r++;
	}
}


int TLS numntstores = 0;
int TLS numpmsyncs = 0;
int TLS numwritecomb = 0;
int getNumNtStores() {return numntstores;}
int getNumPMSyncs() {return numpmsyncs;}
int getNumWriteComb() {return numwritecomb;}


void processBuffer(long t)
{
	if (head == tail)
		return;

	//int i;
	while ((t - storeBuffer[head].startTime) > (getSCMtwr()))
	{
		storeBuffer[head].dest = 0;
		head = (head + 1) % BuffSize;
		if (head == tail)
			return;
	}
}

void flushBuffer()
{
	//printf("F");
	NTWRLOCK;
	while (head != tail)
	{
		processBuffer(getNsTime());
	}
	NTUNLOCK;
}

#endif


#ifndef MODELBUFFER
static unsigned long __thread _lastCacheLine = 0;
#endif

void streamingStoreDelay2(unsigned long dest, int isNewCl, long l)
{
	dest >>= 6;

	//  Create empty space
	while (1)
	{
		if (head == tail)
			break;
		if ((l - storeBuffer[head].startTime) > getSCMtwr())
			head = (head+1)%BuffSize;
		else
			break;
	}
	for (int thead = head; thead != tail; thead = (thead+1)%BuffSize)
	{
		//printf(".");
		if (storeBuffer[thead].dest == dest)
		{
			numwritecomb++;
			return;
		}
	}
	//  If buffer is full then process and adjust start time.
	while (((tail + 1) % BuffSize) == head)
	{
		//printf("F");
		processBuffer(l);
		l = getNsTime();
	}
	//  Add to buffer.
	storeBuffer[tail].dest = dest;
	storeBuffer[tail].startTime = l;
	if (head != tail)
	{
		//  Adjust the start time if faster than the end time of the previous write plus write time.
		int t = tail - 1;
		if (tail==0)
			t = BuffSize-1;
		if ((l - storeBuffer[t].startTime) < (getSCMtwr()))
		{
			storeBuffer[tail].startTime = storeBuffer[t].startTime + getSCMtwr();
		}
	}
	//  Increment the tail.
	tail = (tail + 1) % BuffSize;
}
void streamingStoreDelay(unsigned long dest, int isNewCl, long l)
{
	assert(getSCMtwr() > 0);

#ifndef MODELBUFFER
	if (isNewCl)
		nanoSecDelayUntil(getSCMtwr() + (startNsWrTime));
#endif

#ifdef MODELBUFFER
	dest >>= 6;

#if 0
	if (dest == lastDest)
	{
		numwritecomb++;
		return;
	}
	lastDest = dest;
#endif


	for (int i = 0; i < BuffSize; i++)
	{
		if ((dest == storeBuffer[i].dest) &&
					((i != head)  || ((i == head) &&
			(l < (storeBuffer[i].startTime+getSCMtwr())))))
		{
			//printf("C");
			numwritecomb++;
			return;
		}
	}
	NTWRLOCK;

	//  If buffer is full then process and adjust start time.
	while (((tail + 1) % BuffSize) == head)
	{
		processBuffer(l);
		l = getNsTime();
	}

	//  Add to buffer.
	storeBuffer[tail].dest = dest;
	storeBuffer[tail].startTime = l;
	if (head != tail)
	{
		//  Adjust the start time if faster than the end time of the previous write plus write time.
		int t = tail - 1;
		if (tail==0)
			t = BuffSize-1;
		if ((l - storeBuffer[t].startTime) < (getSCMtwr()))
		{
			storeBuffer[tail].startTime = storeBuffer[t].startTime + getSCMtwr();
		}
	}
	//  Increment the tail.
	tail = (tail + 1) % BuffSize;
	NTUNLOCK;
#endif
}

void streamingStore(int *p, int a)
{
	int isNewCl = 0;
	long startNsWrTime = 0;
	/*
	 * 	void _mm_stream_si32(int *p, int a)
	 * 	Stores the data in a to the address p without polluting the caches. If the cache
	 * 	line containing address p is already in the cache, the cache will be updated.
	 */
#ifdef MODELBUFFER
	if (getSCMtwr() > 0)
		startNsWrTime = getNsTime();
	_mm_stream_si32(p, a);
#else
	*p = a;

	unsigned long dest = (unsigned long)p;
	dest >>= 6;
	if (_lastCacheLine == dest)
	{
		numwritecomb++;
	}
	else
	{
		if (_lastCacheLine != 0)
		{
			isNewCl = 1;
			startNsWrTime = getNsTime();
			clflush((void *)(_lastCacheLine << 6));
		}
		_lastCacheLine = dest;
	}
#endif

	if (getSCMtwr() > 0)
		streamingStoreDelay((unsigned long)p, isNewCl, startNsWrTime);
}


void ntstore(void *dest, void *src, int size)
{
	Debug("ntstore %p %p %d\n", dest, src, size);
	if (dest != src)
	{
		/*
		if (size < 4)
		{
			//  TODO align
			unsigned short d = *(unsigned short*)dest;
//printf("d=%d\n", d);
//fflush(stdout);
//d = *(unsigned short*)src;
//printf("src=%d\n", d);
//fflush(stdout);
//			memcpy(dest, src, size);

		 *(unsigned short*)dest = d;
			clflush(dest);
			char *c = (char *)dest;
			char *s = (char *)src;
		 *c = *s;
		 *(c+1)=*(s+1);
			numntstores++;
			return;
		}
		*/
		//memcpy(dest, src, size);
		//return;


		int *d=(int*)dest;
		int *s=(int*)src;
		int i;
		for (i = 0; i < size>>2; i++)
		{
			//printf("%p %d\n",s,i);
			//printf("%p=%d\n", s, *s);
			streamingStore(d, *s);
			//memcpy(d+i, s+i, 4);
			numntstores++;
			d+=1;
			s+=1;
		}
		if ((size & 3) > 0)
		{
			char c[4];
			char *dc = (char *)d;
			char *sc = (char *)s;
			for (i = (size & 3); i < 4; i++)
				c[i] = *(dc+i);
			for (i = 0; i < (size & 3); i++)
				c[i] = *(sc+i);
			streamingStore(d, *(int*)(&c));
			numntstores++;
		}
	}
}

//  For now, take the size and value as well and model as a streaming store.
void clwb(void *dest, void *src, int size)
{
	ntstore(dest, src, size);
}

/*void ntObjectStore(void *dest, void *src, int size)
{
	int s = 0;
	for (int i = 0; i < (size>>6); i++)
	{
		ntstore((void*)((char*)dest + s), (void*)((char*)src + s), 64);
		s += 64;
	}
	ntstore((void*)((char*)dest +s), (void*)((char*)src + s), size - s);
}*/

void cacheflush()
{
	Debug("cacheflush\n");
	assert(0);
}

void msync()
{
	Debug("msync\n");
	_mm_mfence();
	assert(0);
}

void sfence()
{
	_mm_sfence();
}

void pcommit()
{
	p_msync();
}

void p_msync()
{
	//Debug("p_msync\n");
	numpmsyncs++;

	//  Fence.
	sfence();

	if (getSCMtwr() < 0)
	{
		return;
	}
	else
	{
		//  Serializing instruction.
		serialize();

		//  Delay
		if (getSCMtwr() > 0)
		{
#ifdef MODELBUFFER
			flushBuffer();
#else
			if (_lastCacheLine != 0)
			{
				t = getNsTime();
				clflush((void *)(_lastCacheLine << 6));
				nanoSecDelayUntil(t + getSCMtwr());
				_lastCacheLine = 0;
			}
#endif
		}
	}
}

void *mallocAligned(size_t size)
{
	static int mask = ~(CACHELINESIZE - 1);
	size = ((size - 1) & mask) + CACHELINESIZE;

	//	Debug("%d\n", (int)size);
	//void *v = malloc(size);
	void *v;
	int err = posix_memalign(&v, CACHELINESIZE, size);
	if (err)
		fprintf(stderr, "mallocAligned error %d\n", err);
	return v;
}



/*
PAGESIZE                           4096
PAGE_SIZE                          4096
LEVEL1_ICACHE_SIZE                 32768
LEVEL1_ICACHE_ASSOC                8
LEVEL1_ICACHE_LINESIZE             64
LEVEL1_DCACHE_SIZE                 32768
LEVEL1_DCACHE_ASSOC                8
LEVEL1_DCACHE_LINESIZE             64
LEVEL2_CACHE_SIZE                  6291456
LEVEL2_CACHE_ASSOC                 24
LEVEL2_CACHE_LINESIZE              64
LEVEL3_CACHE_SIZE                  0
LEVEL3_CACHE_ASSOC                 0
LEVEL3_CACHE_LINESIZE              0
LEVEL4_CACHE_SIZE                  0
LEVEL4_CACHE_ASSOC                 0
 */
void printMemoryAttributes(void)
{
	printf("Int_Size \t%ld\n", sizeof(int));
	printf("Page_Size \t%ld\n", sysconf(_SC_PAGESIZE)); /* _SC_PAGE_SIZE is OK too. */
#ifdef _SC_LEVEL1_DCACHE_LINESIZE
	printf("Cache_LineSize \t%ld\n", sysconf (_SC_LEVEL1_DCACHE_LINESIZE));   /*  getconf LEVEL1_DCACHE_LINESIZE  */
#else
	printf("Cache_linesize is undetermined!!\n");
#endif
}

int getCacheLineSize()
{
#ifdef _SC_LEVEL1_DCACHE_LINESIZE
	return sysconf (_SC_LEVEL1_DCACHE_LINESIZE);
#else
	return 64;
#endif
}

#ifndef GET_CACHE_LINE_SIZE_H_INCLUDED
#define GET_CACHE_LINE_SIZE_H_INCLUDED

// Author: Nick Strupat
// Date: October 29, 2010
// Returns the cache line size (in bytes) of the processor, or 0 on failure

#include <stddef.h>
size_t cache_line_size();

#if defined(__APPLE__)

#include <sys/sysctl.h>
size_t cache_line_size() {
	size_t line_size = 0;
	size_t sizeof_line_size = sizeof(line_size);
	sysctlbyname("hw.cachelinesize", &line_size, &sizeof_line_size, 0, 0);
	return line_size;
}

#elif defined(_WIN32)

#include <stdlib.h>
#include <windows.h>
size_t cache_line_size() {
	size_t line_size = 0;
	DWORD buffer_size = 0;
	DWORD i = 0;
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION * buffer = 0;

	GetLogicalProcessorInformation(0, &buffer_size);
	buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(buffer_size);
	GetLogicalProcessorInformation(&buffer[0], &buffer_size);

	for (i = 0; i != buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
		if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == 1) {
			line_size = buffer[i].Cache.LineSize;
			break;
		}
	}

	free(buffer);
	return line_size;
}

#elif defined(linux)

#include <stdio.h>
size_t cache_line_size() {
	FILE * p = 0;
	p = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
	unsigned int i = 0;
	if (p) {
		if (fscanf(p, "%d", &i) < 0)
			i = 0;
		fclose(p);
	}
	return i;
}

#else
#error Unrecognized platform
#endif

#endif
