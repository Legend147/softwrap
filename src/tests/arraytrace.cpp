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

//4096
#define BlockSize 1024

int IntsPerLine = getCacheLineSize() / sizeof(int);
int IntsPerBlock = BlockSize / sizeof(int);
int arraySize;
int *shadowPaging = NULL;

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
		for (i = 0; i < size; i+=IntsPerLine)
		{
			f = p + i;
			//printf("clfush(%p)\n", f);
			clflush(f);
		}
	}
	p_msync();
}

void blockCow(int *data, int nwraps, int wrapsize, int arraySize)
{
	int i, j, index;
	//int delay = getDelay();
	arraySize = 20000;
	printf("IntsPerLine= %d  \tIntsPerBlock= %d  \tarraySize= %d\n", IntsPerLine, IntsPerBlock, arraySize);

	shadowPaging = (int *)pmalloc(arraySize * sizeof(int));
	long ns = getNsTime();
	for (i=0; i < nwraps; i+=1)
	{
		//printf("+");

		/*
		//  Copy old values.
		//  64 bytes per cache line * 16 cache lines
		//  16 ints per cache line (64 bytes) * 16 cache lines = IntsPerBlock ints (BlockSize bytes)
		for (k = 0; k < 16; k++)
		{
			ntstore(blockCowLog + (i*IntsPerBlock) + (k*16), data + (i*IntsPerBlock) + (k*16), 64);
			//if (k != 15)
				//doDelay(delay);
		}
		p_msync();

		for (j = 0; j < wrapsize; j++)
		{
			index = (i * IntsPerBlock) + ((IntsPerBlock * j) / wrapsize);
			ntstore(data + index, &j, sizeof(int));
		}
		*/

		for (j = 0; j < IntsPerBlock; j++)
		{
			index = (i * IntsPerBlock) + j;
			index %= arraySize;
			if ((j+1) % (IntsPerBlock / wrapsize) == 0)
			{
				//printf("j=%d\n", j);
				//  assign new data.
				data[index] = 124;
			}
			ntstore(shadowPaging + index, data+index, sizeof(int));
		}
		p_msync();
		// sim pointer flip.
		ntstore(shadowPaging, data, sizeof(int));
		p_msync();
	}
	printf("\n");
	ns = getNsTime() - ns;
	printf("BlockCopyTime= %ld \tAverageBlockCopy= %ld\n", ns, ns/nwraps);
}


void trace(int *data, int nwraps, int wrapsize, int inc, int option)
{
	int i,j,index;
	int count = 0;

	for (i=0; i < nwraps; i+=1)
	{
		//printf("+");

		WRAPTOKEN w = wrapOpen();

		for (j = 0; j < wrapsize; j++)
		{
			//printf(".");

			if (option == 0)
			{
				index = ((i * wrapsize) + j) * IntsPerLine;
			}
			else if (option == 1)
			{
				index = count;
			}
			else if (option == 2)
			{
				index = random() % arraySize;
			}
			else if (option == 3)
			{
				index = (i * IntsPerBlock) + ((IntsPerBlock * j) / wrapsize);
			}
			else
				assert(0);

			//int *p = &(data[index]);
			//wrapWrite(data+index, &i, sizeof(int), w);
			wrapStore32(data+index, i, w);
			//memcpy(data+index, &i, sizeof(int));
			//printf("data[%d](%p)=%d ", index, p, *p);
			//record(p, STORE | WRAPPED | PERSISTENT);
			count ++;
		}
		//printf("-\n");
		wrapClose(w);
	}
	printf("\n");
}


void usage(char *c)
{
	//fprintf(stderr, "Usage: %s hotCache wrapSize numWraps [wrapType [options]]\n", c);
	fprintf(stderr, "Usage: %s option wrapSize numWraps [wrapType [options]]\n", c);
	fprintf(stderr, "\tRun an array update of numWraps of wrapSize access in each wrap.\n" \
					"\tWhere option is one of:\n" \
					"\t0-cacheline 1-sequential 2-random(size is wrapsize*numWraps) \n" \
					"\t3-block sequential (evenly dist. across 4k block) or 4-blockcow(wrapType ignored, block 4k)\n");
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
	int wrapType;
	int wrapOptions;
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
		arraySize = numWraps * wrapSize * IntsPerLine;
	else if (arrayTraceOption == 1)
		arraySize = numWraps * wrapSize;
	else if (arrayTraceOption == 2)
		arraySize = wrapSize*numWraps;
		//arraySize = dmax(BlockSize, wrapSize * numWraps);
	else if (arrayTraceOption == 3)
		arraySize = BlockSize /sizeof(int) * numWraps;
	else if (arrayTraceOption == 4)
		arraySize = (BlockSize / sizeof(int))*numWraps;
	else
		assert(0);

	StartPhysicalTracer();
	startStatistics();

	arraySize = 20000;

	int *data = (int *)pmalloc(arraySize * sizeof(int));
	printf("Array size=%d integers with start address: %p\n", arraySize, data);

	//init(data, arraysize, hotCache);

	if (arrayTraceOption == 4)
		blockCow(data, numWraps, wrapSize, arraySize);
	else
		trace(data, numWraps, wrapSize, IntsPerLine, arrayTraceOption);

	EndPhysicalTracer();

	printf("%s \t%s \t%s \t%s \t", argv[0], argv[1], argv[2], argv[3]);
	printStatistics(stdout);

	wrapCleanup();
	pfree(data);
	//  Shadow Paging cleanup.
	if (shadowPaging != NULL)
		pfree(shadowPaging);

	return 0;
}

