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
#include "wrap.h"
#include "memtool.h"
#include "tracer.h"

int IntsPerBlock = getCacheLineSize() / sizeof(int);
int arraySize;
int wrapType;
int wrapOptions;

void init(int *p, int size, int hotCache)
{
	WRAPTOKEN w = wrapOpen();
	int i;
	for (i=0; i  < size; i++)
		wrapWrite((p + i), &i, sizeof(int), w);
	wrapClose(w);

//  TODO for hot cache will need to have started stats before here!

	int *f = p;
	if (!hotCache)
	{
		printf("Flusing cache.\n");
		for (i = 0; i < size; i+=IntsPerBlock)
		{
			f = p + i;
			//printf("clfush(%p)\n", f);
			clflush(f);
		}
	}
	p_msync();
}


void validate(int *data, int nwraps, int wrapsize, int inc, int option)
{
	int i,j,index;
	int count = 0;

	for (i=0; i < nwraps; i+=1)
	{
		printf("+");

		WRAPTOKEN w = wrapOpen();

		for (j = 0; j < wrapsize; j++)
		{
			printf(".");

			if (option == 0)
			{
				index = ((i * wrapsize) + j) * IntsPerBlock;
			}
			else if (option == 1)
			{
				index = count;
			}
			else if (option == 2)
			{
				index = random() % arraySize;
			}
			else
				assert(0);

			//int *p = &(data[index]);
			wrapWrite(data+index,&i, sizeof(int), w);
			//memcpy(data+index, &i, sizeof(int));
			//printf("data[%d](%p)=%d ", index, p, *p);
			//record(p, STORE | WRAPPED | PERSISTENT);
			count ++;
		}
		printf("-\n");
		wrapClose(w);
	}

	printf("Validate");
	/*
	for (j = 0; j < wrapsize; j++)
	{
		printf(".");

		if (option == 0)
		{
			index = j * IntsPerBlock;
		}
		else if (option == 1)
		{
			index = count;
		}
		else
			assert(0);
		printf("data[%d]=%d\n", index, data[index]);
		assert(data[index] == 1);
	}
	*/
}


void usage(char *c)
{
	//fprintf(stderr, "Usage: %s hotCache wrapSize numWraps [wrapType [options]]\n", c);
	fprintf(stderr, "Usage: %s option wrapSize numWraps [wrapType [options]]\n", c);
	fprintf(stderr, "\tRun an array validate and restore of numWraps of wrapSize access in each wrap.\n" \
					"\tWhere option is 0-cacheline 1-sequential 2-random(with fixed size of 1024)\n");
			//"\tTODO - Where hotCache is a flag for pre-loading the cache before starting statistics.\n"
			//"\tIf 1 then the cache is hot, and 0 for cold cache (invalidates cache after array init).\n");
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
	if ((argc < 4) || (argc > 6))
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	int arrayTraceOption = getIntArg(argv[1]);
	int wrapSize = getIntArg(argv[2]);
	int numWraps = getIntArg(argv[3]);

	if ((arrayTraceOption < 0) || (wrapSize < 0) || (numWraps < 0))
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	if (argc >= 5)
	{
		wrapType = getIntArg(argv[4]);
		if (wrapType < 0)
		{
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
		setWrapImpl((WrapImplType)wrapType);
		if (argc >= 6)
		{
			wrapOptions = getIntArg(argv[5]);
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

	if (arrayTraceOption == 0)
		arraySize = numWraps * wrapSize * IntsPerBlock;
	else if (arrayTraceOption == 1)
		arraySize = numWraps * wrapSize;
	else if (arrayTraceOption == 2)
		arraySize = 1024;
	else
		assert(0);

	StartPhysicalTracer();
	startStatistics();

	int *data = (int *)pmalloc(arraySize * sizeof(int));
	for (int i = 0; i < arraySize; i++)
	{
		data[i] = 1;
	}
	printf("Array size=%d integers with start address: %p\n", arraySize, data);

	//init(data, arraysize, hotCache);

	validate(data, numWraps, wrapSize, IntsPerBlock, arrayTraceOption);
	pfree(data);

	EndPhysicalTracer();

	printf("%s \t%s \t%s \t%s \t", argv[0], argv[1], argv[2], argv[3]);
	printStatistics(stdout);

	return 0;
}

