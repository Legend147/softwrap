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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "wrap.h"
#include "memtool.h"
#include "tracer.h"

int arraySize;
int numThreads, threadMemSize, rwRatio, wrapSize, numWraps, delay;
//int *data;

long elapsed(struct timeval start)
{
    struct timeval t;
	if (gettimeofday(&t, NULL) < 0)
	{
		perror("Error getting the time of day");
		abort();
	}
	//printf("%lu %u\n", t.tv_sec, (unsigned)t.tv_usec);

	/*  Calculate the latency in us.  */
	long us = ((t.tv_sec - start.tv_sec) * 1000000) + (t.tv_usec - start.tv_usec);
	return us;
}

void *test(void *nn)
{
	int n = (uint64_t)nn;
	int i,j,index;
	int reads;
	int *data = (int *)pmalloc(arraySize * sizeof(int));
	printf("Array size=%d integers with start address: %p\n", arraySize, data);


	for (i=0; i < numWraps; i+=1)
	{
		attachThreadToCore(6 - (n % 6));

		WRAPTOKEN w = wrapOpen();

//		int wrapsize = (random() % maxWrapSize) + 1;
//				maxWrapSize / 2;
		for (j = 0; j < wrapSize; j++)
		{
			index = random() % threadMemSize;
			index += n*threadMemSize;

			//int *p = &(data[index]);
			//wrapWrite(data+index, &i, sizeof(int), w);
			//memcpy(data+index, &i, sizeof(int));
			//printf("data[%d](%p)=%d ", index, p, *p);
			//record(p, STORE | WRAPPED | PERSISTENT);
			//count ++;

			wrapStore32(data+index, i, w);

			for (reads = 0; reads < rwRatio; reads++)
			{
				//wrapRead(data+(random()%arraySize), sizeof(int), w);
				wrapLoad32(data+(random()%arraySize), w);
			}

			if (delay > 0)
			{
				struct timeval start;
				gettimeofday(&start, NULL);
				while (elapsed(start) < delay);
			}
		}
		wrapClose(w);
	}
	//printf("Done %d\n", n);
	pfree(data);

	return NULL;
}


void usage(char *c)
{
	fprintf(stderr, "Usage: %s numThreads threadMemSize rwRatio wrapSize numWraps delay [wrapType [options]]\n", c);
	fprintf(stderr, "\tRun a random array update of numWraps of wrapSize access in each wrap with delay in between.\n" \
					"\tWhere numThreads is the number of threads.\n" \
					"\tEach thread writes to an assigned area, but can read from any thread's array area.\n" \
					"\tthreadMemSize is the size of the memory array for update by a thread.\n");
	fprintf(stderr, "\tWhere wrapType is one of:\n");
	printWrapImplTypes(stderr);
	fprintf(stderr, "\tFor type %d the options are:\n", getWrapImplType());
	printWrapImplOptions(stderr);
}

int getIntArg(char *c)
{
	int val = atoi(c);
	if (val == 0)
	{
		if (c[0] != '0')
			return -1;
	}
	return val;
}

int main(int argc, char *argv[])
{
	if ((argc < 6) || (argc > 9))
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	//mtarray numThreads threadMemSize rwRatio maxWrapSize numWraps [wrapType [options]
	numThreads = getIntArg(argv[1]);
	threadMemSize = getIntArg(argv[2]);
	rwRatio = getIntArg(argv[3]);
	wrapSize = getIntArg(argv[4]);
	numWraps = getIntArg(argv[5]);
	delay = getIntArg(argv[6]);

	if ((numThreads < 0) || (threadMemSize < 0) || (delay < 0) || (rwRatio < 0) || (wrapSize < 0) || (numWraps < 0))
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	int wrapType;
	int wrapOptions;
	if (argc >= 8)
	{
		wrapType = getIntArg(argv[7]);
		if (wrapType < 0)
		{
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
		if (wrapType == Wrap_Software)
		{
			setAliasTableWorkers(numThreads);
		}
		setWrapImpl((WrapImplType)wrapType);
		if (argc >= 9)
		{
			wrapOptions = getIntArg(argv[8]);
			if (wrapOptions < 0)
			{
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			setWrapImplOptions(wrapOptions);
		}
	}

	//  Display memory attributes.
	printMemoryAttributes();

	arraySize = numThreads * threadMemSize;

	StartPhysicalTracer();
	startStatistics();

	//data = (int *)pmalloc(arraySize * sizeof(int));
	//printf("Array size=%d integers with start address: %p\n", arraySize, data);

	//  Create the test threads
	pthread_t *testers = (pthread_t *)malloc(numThreads * sizeof(pthread_t));
	for (int i = 0; i < numThreads; i++)
	{
		pthread_create(&testers[i], NULL, test, (void*)(int64_t)i);
	}

	printf("joining\n");
	for (int i = 0; i < numThreads; i++)
	{
		pthread_join(testers[i], NULL);
	}
	free(testers);

	EndPhysicalTracer();

	printf("%s \t%s \t%s \t%s \t", argv[0], argv[1], argv[2], argv[3]);
	printStatistics(stdout);

	wrapCleanup();

	return 0;
}

