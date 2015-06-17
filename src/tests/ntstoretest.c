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

#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include "memtool.c"


int *data;
int size;
int NSamples, NStream, nanoDelay, DoRand;
#define samples 20000
pthread_barrier_t barrierOne, barrierTwo;

void setSCMtwr(int nanosec);

void streamInt(int *p, int *src)
{
	/*
	 * 	void _mm_stream_si32(int *p, int a)
	 * 	Stores the data in a to the address p without polluting the caches. If the cache
	 * 	line containing address p is already in the cache, the cache will be updated.
	 */
	_mm_stream_si32(p, *src);
}

int test(void *v)
{
	int j, k, r;
	long l = 0;
	long start, end;

	//  Set the delay.
	setSCMtwr(nanoDelay);

	for (j = 0; j < NSamples; j++)
	{
		//printf(".");
		fflush(stdout);
		start = getNsTime();
		for (k = 0; k < NStream; k++)
		{
			if (DoRand)
				r = rand() % size;
			else
				r = (j * NStream + k) % size;
			if (nanoDelay == 0)
				streamInt(data + r, &r);
			else
				ntstore(data+r, &r,4);
		}
		// Don't measure the p_msync time.
		//p_msync();
		l += getNsTime() - start;

		//  Barrier 1.
		if (v!=NULL)
			pthread_barrier_wait (&barrierOne);

		//  Sync.
		p_msync();
		doMicroSecDelay(100);

		//  Barrier 2.
		if (v!=NULL)
			pthread_barrier_wait (&barrierTwo);
	}
	//	return l/2;
	//	return (int)((l * 1000)/nsamples);
	//printf("%d\n", (int)l);
	return (int)(l/1000);
}

void *mttest(void *v)
{
	//*(int *)v = test();
	//printf("%d\n", *(int*)v);
	//pthread_exit(v);
	*(int *)v = test(v);
	//printf("%d\n", test());
}

int mttester(int nsamples, int i, int delay, int dorand, int numThreads)
{
	NSamples = nsamples;
	NStream = i;
	nanoDelay = delay;
	DoRand = dorand;
	//  Create the test threads
	pthread_t *testers = (pthread_t *)malloc(numThreads * sizeof(pthread_t));
	int *values = (int *)malloc(numThreads * sizeof(int));
	for (int i = 0; i < numThreads; i++)
	{
		pthread_create(&testers[i], NULL, mttest, (void*)(values + i));
	}

	int total = 0;
	int *valuep;
	for (int i = 0; i < numThreads; i++)
	{
		pthread_join(testers[i], NULL);
		//printf("%d ", i);
		total += values[i];
		//printf("%d\n", *valuep);
	}
	free(testers);
	free(values);
	usleep(1000000);
	return total;
}

int tester(int nsamples, int nstream, int delay, int dorand)
{
	NSamples = nsamples;
	NStream = nstream;
	nanoDelay = delay;
	DoRand = dorand;
	return test(0);
}

void domttests(int n, int numingroup, int delay, int nthreads)
{
	int i = nthreads;
	printf("%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d\n", numingroup, n, i, i*numingroup,
				mttester(n, numingroup, 0, 1, i),  mttester(n, numingroup, 0, 0, i),
				mttester(n, numingroup, delay, 1, i),  mttester(n, numingroup, delay, 0, i));
}


void dotests(int nsamples, int i, int delay)
{
	printf("%d \t%d", i, nsamples);


	//printf(" \t%d\n", test(nsamples, i, 0, 0));
	/*
	printf(" \t%d \t%d \t%d \t%d\n", test(nsamples, i, 0, 1), test(nsamples, i, 0, 0),
			test(nsamples, i, 4, 1), test(nsamples, i, 4, 0));
	 */
	printf(" \t%d \t%d \t%d \t%d\n", tester(nsamples, i, 0, 1), tester(nsamples, i, 0, 0),
			tester(nsamples, i, delay, 1), tester(nsamples, i, delay, 0));
	//printf(" \t%d \t%d\n", test(nsamples, i, 4, 1), test(nsamples, i, 4, 0));
}

void usage(char *s)
{
	fprintf(stderr, "Usage: %s arraySize scmWriteTime [maxNumThreads writeGroupSize]\n", s);
	fprintf(stderr, "   This measures groups of ntstores with the specified SCM write time.\n");
	fprintf(stderr, "       %s scmTime\n", s);
	fprintf(stderr, "   This measures the average time for cpuid instruction.\n");
}

extern void serialize();

int main(int argc, char *argv[])
{
	//  Take this as a command line arg to avoid optimizations.
	if (argc == 2)
	{
		int i = atoi(argv[1]);
		if (i < 0)
		{
			usage(argv[0]);
			exit(1);
		}
		long l = 0;
		long ntstoretime = 0;
		setSCMtwr(i);
		struct timeval start, end, dummy;
		for (i = 0; i < 1000; i++)
		{
			int *j=(int*)malloc(8192000);
			int k;
			long st = getNsTime();
			for (int k = 0; k < 1000; k++)
			{
				ntstore(&j[rand()%(8192000/4)], &k, 4);
				//_mm_stream_si32(&j[rand()%(8192000/4)], 99);
			}
			ntstoretime += (getNsTime() - st);
			st = getNsTime();
			//printf(".");
			//  TODO
			//gettimeofday(&dummy, NULL);
			p_msync();
			l += (getNsTime() - st);
			doMicroSecDelay(1000);
			free(j);
		}
		double d = ntstoretime;
		d /= 1000000;
		printf("avgntstore= %f  pcommit= %ld\n", d, l/1000);
		return 0;
	}

	if ((argc < 3) || (argc > 5))
	{
		usage(argv[0]);
		exit(1);
	}
	size = atoi(argv[1]);
	if (size <= 0)
	{
		usage(argv[0]);
		exit(1);
	}
	int delay = atoi(argv[2]);
	if (delay < 0)
	{
		usage(argv[0]);
		exit(1);
	}

	int n = 1000;
	data = (int*)malloc(sizeof(int)*size);

	int i;
	//  Init the array to minimize cache effects.
	for (i = 0; i < size; i++)
	{
		streamInt(data + i, &i);
	}
	int nthreads = 1;
	if (argc > 3)
	{
		nthreads = atoi(argv[3]);
		assert(nthreads > 0);

		if (nthreads > 1)
		{
			if (argc != 5)
			{
				usage(argv[0]);
				exit(1);
			}
			int numingroup = atoi(argv[4]);
			assert(numingroup > 0);
			printf("num \tns \tth \ttsamp \tdr \tds \tsr \tss\n");
			for (int i = 1; i <= nthreads; (i<6)?i+=1:((i<10)?i+=2:i+=10))
			{
				pthread_barrier_init (&barrierOne, NULL, i);
				pthread_barrier_init (&barrierTwo, NULL, i);
				domttests(n, numingroup, delay, i);
				pthread_barrier_destroy (&barrierOne);
				pthread_barrier_destroy (&barrierTwo);
			}
			return 0;
		}
	}

	printf("num \tns \tdr \tds \tsr \tss\n");
	for (i = 2; i < 20; i+=2)
		dotests(n, i, delay);
	for (i = 20; i < 321; i+=10)
		dotests(n, i, delay);
	return 0;
}
