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

#define OBJECT_SIZE 1024
#define UPDATE_SIZE 16

struct obj
{
	char fields[OBJECT_SIZE/UPDATE_SIZE][UPDATE_SIZE];
};

int arraySize;

void trace(int wrapType, obj **data, int nwraps, int wrapsize, int objectUpdate)
{
	int i,j;

	for (i=0; i < nwraps; i+=1)
	{
		printf("+");

		WRAPTOKEN w = wrapOpen();

		for (j = 0; j < wrapsize; j++)
		{
			printf(".");

			char up[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890123456789";

			if (wrapType == 0)
			{
				obj *n = (obj *)pmalloc(sizeof(obj));
				//  NOTE WE ARE DOING AN OPTIMAL COPY, ONLY COPYING BYTES NOT CHANGED
				//  Save the new data.
				for (int k = 0; k < objectUpdate; k++)
				{
					ntstore(n->fields[k], up, UPDATE_SIZE);
				}
				//  Copy the unchanged bytes.
				for (int k = objectUpdate; k < OBJECT_SIZE/UPDATE_SIZE; k++)
				{
					ntstore(n->fields[k], data[j]->fields[k], UPDATE_SIZE);
				}
				//  Make sure all writes have completed.
				p_msync();
				//  Pointer switch.
				ntstore(data[j], n, sizeof(n));
				p_msync();
			}
			else
			{
				for (int k = 0; k < objectUpdate; k++)
				{
					wrapWrite(data[j]->fields[k], up, UPDATE_SIZE, w);
				}
			}
		}
		printf("-\n");
		wrapClose(w);
	}
}


void usage(char *c)
{
	fprintf(stderr, "Usage: %s objectUpdate wrapSize numWraps [wrapType [options]]\n", c);
	fprintf(stderr, "\tRun an array update of numWraps of wrapSize access in each wrap.\n" \
					"\tWhere an object of size 1k is updated in segments of 64 bytes each.\n" \
					"\tWhere objectUpdate is the number of 64 byte blocks (1-16) to update in each element update.\n" \
					"\twrapType of NoAtomicity will do pointer switch method.\n");
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

	int objectUpdate = getIntArg(argv[1]);
	int wrapSize = getIntArg(argv[2]);
	int numWraps = getIntArg(argv[3]);

	if ((objectUpdate <= 0) || (objectUpdate > (OBJECT_SIZE/UPDATE_SIZE)))
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

	StartPhysicalTracer();
	startStatistics();

	obj **data = (obj **)pmalloc(wrapSize * sizeof(obj*));
	//  NOTE If we have a large array with multiple update paths, you can't do a pointer switch.
	for (int i = 0; i < wrapSize; i++)
	{
		data[i] = (obj *)pmalloc(sizeof(obj));
	}
	printf("Array size=%d objects with start address: %p\n", wrapSize, data);

	trace(wrapType, data, numWraps, wrapSize, objectUpdate);
	pfree(data);

	EndPhysicalTracer();

	printf("%s \t%s \t%s \t%s \t", argv[0], argv[1], argv[2], argv[3]);
	printStatistics(stdout);

	return 0;
}

