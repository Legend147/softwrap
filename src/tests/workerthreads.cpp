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

int arraySize, numThreads, rwRatio, wrapSize, numWrapsPerThread, delay;
int *data;
pthread_rwlock_t *locks;


void *test(void *v)
{
	int i, j, index, reads;

	for (i=0; i < numWrapsPerThread; i+=1)
	{
		WRAPTOKEN w = wrapOpen();

		for (j = 0; j < wrapSize; j++)
		{
			index = random() % arraySize;

			pthread_rwlock_wrlock(locks+index);
			wrapStore32(data+index, i, w);
			//wrapWrite(data+index, &i, sizeof(int), w);
			//printf("data[%d](%p)=%d ", index, p, *p);
			pthread_rwlock_unlock(locks+index);

			for (reads = 0; reads < rwRatio; reads++)
			{
				index = random()%arraySize;
				pthread_rwlock_rdlock(locks+index);
				wrapLoad32(data+index, w);
				//wrapRead(data+(random()%arraySize), sizeof(int), w);
				pthread_rwlock_unlock(locks+index);
			}

			if (delay > 0)
				doMicroSecDelay(delay);
		}
		wrapClose(w);
	}
	return NULL;
}


void usage(char *c)
{
	fprintf(stderr, "Usage: %s numThreads arraySize rwRatio wrapSize numWraps delayBetween [wrapType [options]]\n", c);
	fprintf(stderr, "\tRun a random array update of numWraps each of wrapSize access in each wrap.\n" \
					"\tWhere numThreads is the number of threads each doing numWraps/numThreads wraps each.\n" \
					"\tEach thread writes to a random area and can read from a random area.\n" \
					"\tFine grained locking is used - a read/write lock per element.\n");
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
	arraySize = getIntArg(argv[2]);
	rwRatio = getIntArg(argv[3]);
	wrapSize = getIntArg(argv[4]);
	int numWraps = getIntArg(argv[5]);
	delay = getIntArg(argv[6]);

	if ((numThreads < 0) || (arraySize < 0) || (delay < 0) || (rwRatio < 0) || (wrapSize < 0) || (numWraps < 0))
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


	data = (int *)pmalloc(arraySize * sizeof(int));
	locks = (pthread_rwlock_t *)pmalloc(arraySize * sizeof(pthread_rwlock_t));

	for (int i = 0; i < arraySize; i++)
	{
		data[i] = random();
		pthread_rwlock_init(locks + i, NULL);
	}

	StartPhysicalTracer();
	startStatistics();

	printf("Array size=%d integers with start address: %p\n", arraySize, data);
	numWrapsPerThread = numWraps / numThreads;
	printf("wraps per thread = %d\n", numWrapsPerThread);

	//  Create the test threads
	pthread_t *testers = (pthread_t *)malloc(numThreads * sizeof(pthread_t));
	for (int i = 0; i < numThreads; i++)
	{
		pthread_create(&testers[i], NULL, test, NULL);
	}

	for (int i = 0; i < numThreads; i++)
	{
		pthread_join(testers[i], NULL);
	}
	free(testers);

	//pfree(data);

	EndPhysicalTracer();

	printf("%s \t%s \t%s \t%s \t", argv[0], argv[1], argv[2], argv[3]);
	printStatistics(stdout);

	return 0;
}

