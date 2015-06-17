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

//  This is for super blasting non-atomic writes to scm to verify wrap class overhead is not much.
#ifdef NA
#define wrapOpen()	5
#define wrapClose(x)	p_msync();
#define	wrapStore32(x,y,z)	ntstore(x,&y,4); z=1;
#endif

void trace(int *data, int arraySize, int wrapSize, int nSeconds, long rate, int reuse)
{
	int j,index;
	long count = 0;
	long elapsed = 0;
	long ns = nSeconds;
	ns *= 1000000000;
	long txStartTime, txEndTime, txElapsed;
	long txTotalTime = 0;
	long mintxtime = 1<<30;
	long maxtxtime = 0;
	long t1, t2, t3;
	long totalWrapOpen = 0;
	long totalWrapWrite = 0;
	long totalWrapClose = 0;
	int doPrint = (getenv("WrapTimeData")==NULL)?0:1;
	long IgnoreFirst = 0;

	//  Reuse
	int *reuseIndexes = (int*)malloc(wrapSize*sizeof(int));
	for (int i=0; i < wrapSize; i++)
	{
		reuseIndexes[i] = random() % arraySize;
	}

	AttachPrimaryCore;

	double d = 0.0;
	assert(nSeconds >= 3);
	long start, recordStart;
	start = getNsTime();
	recordStart = 0;
	printf("start\n");
	do
	{
		d = (count-IgnoreFirst) * (1000000000.0 / rate) + recordStart;
		t1 = getNsTime();
		if (d > t1)
			d = t1;
		//printf("%f %ld, count=%ld ignore=%ld\n", d, recordStart, count, IgnoreFirst);
		txStartTime = d;
		WRAPTOKEN w = wrapOpen();
		t2 = getNsTime();
		for (j = 0; j < wrapSize; j++)
		{
			if ((reuse > 0) && (j < ((reuse * wrapSize) / 100)))
			{
				//printf("reuse[%d]=%d ", j, reuseIndexes[j]);
				index = reuseIndexes[j];
			}
			else
			{
				//printf(".");
				index = random() % arraySize;
			}
			wrapStore32(data+index, j, w);
		}
		t3 = getNsTime();
		wrapClose(w);
		txEndTime = getNsTime();
		txElapsed = txEndTime - txStartTime;
		if (txElapsed < 0)
		{
			printf("count=%ld IgnoreFirst=%ld d=%f t1=%ld end=%ld  recordStart=%ld\n", count, IgnoreFirst, d, t1, txEndTime, recordStart);
			if (rand()%100 > 90)
				exit(-1);
		}
		count++;
		AttachPrimaryCore;
		do
		{
			elapsed = getNsTime() - start;
		}
		while (count > ((double)rate * (double)elapsed) / 1000000000.0);

		//  Ignore first values.
		if (elapsed > ns/3)
		{
			if (recordStart == 0)
			{
				recordStart = getNsTime();
				IgnoreFirst++;
			}
			else
			{
				txTotalTime += txElapsed;
				if (txElapsed < mintxtime)
					mintxtime = txElapsed;
				if (txElapsed > maxtxtime)
					maxtxtime = txElapsed;
				totalWrapOpen += (t2 - t1);
				totalWrapWrite += t3 - t2;
				totalWrapClose += txEndTime - t3;
				if (doPrint)
				{
					//printf("time=%ld st=%ld tt=%ld elapsed= %ld  ma= %f\n", txEndTime, txStartTime, txTotalTime, txElapsed, (double)txTotalTime/(count-IgnoreFirst));
					printf("t= %ld txElapsed= %ld  AvgTxTime= %f  %s\n", txEndTime, txElapsed, (double)txTotalTime/(count-IgnoreFirst), getAliasDetails());
				}

			}
		}
		else
		{
			IgnoreFirst++;
		}
	}
	while (elapsed < ns);
	printf("txEndTime - now=%ld\n", getNsTime()-txEndTime);

	double txps = count - IgnoreFirst;
		txps *= 1000000000;
		wrapFlush();
		txps /= (getNsTime() - recordStart);



	double avgtime = txTotalTime;
	avgtime /= (count - IgnoreFirst);

	printf("\n%f txps\n", txps);
	printf("%f avgtxtime\n", avgtime);
	printf("%ld mintxtime\n", mintxtime);
	printf("%ld maxtxtime\n", maxtxtime);
	d = totalWrapOpen;
	d /= (count - IgnoreFirst);
	printf("%f avgservice\n", (double)(totalWrapOpen + totalWrapClose + totalWrapWrite) / (count-IgnoreFirst));
	printf("%f avgWrapOpen\n", d);
	printf("%ld avgWrapWrite\n", totalWrapWrite / (count - IgnoreFirst));
	printf("%ld avgWrapClose\n", totalWrapClose / (count - IgnoreFirst));
}

void maxtrace(int *data, int arraySize, int wrapSize, int reuse)
{
	int j, index;
	long start = 0;

	//  Reuse
	int *reuseIndexes = (int*)malloc(wrapSize*sizeof(int));
	for (int i=0; i < wrapSize; i++)
	{
		reuseIndexes[i] = random() % arraySize;
	}

	printf("start\n");
	AttachPrimaryCore;

	for (int i=0; i < 1000000; i++)
	{
		if (i == 250000)
			start = getNsTime();
		WRAPTOKEN w = wrapOpen();
		for (j = 0; j < wrapSize; j++)
		{
			if ((reuse > 0) && (j < ((reuse * wrapSize) / 100)))
				index = reuseIndexes[j];
			else
				index = random() % arraySize;
			wrapStore32(data+index, j, w);
		}
		wrapClose(w);
		//p_msync();
	}

	double txps = 749999;
	txps *= 1000000000;
	wrapFlush();
	txps /= (getNsTime() - start);

	printf("\n%f txps\n", txps);
}


void usage(char *c)
{
	fprintf(stderr, "Usage: %s wrapSize nSeconds rate reusePercent [wrapType [options]]\n", c);
	fprintf(stderr, "\tRun an array update of size wrapSize access in each wrap for at least nSeconds.\n" \
					"\tThe rate is the arrival rate of transactions per second.\n" \
					"\tIf rate is zero, then transactions are added continuously.\n");
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
	if ((argc < 5) || (argc > 8))
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	int wrapSize = getIntArg(argv[1]);
	int nSeconds = getIntArg(argv[2]);
	int rate = getIntArg(argv[3]);
	int reuse = getIntArg(argv[4]);

	if ((wrapSize < 0) || (nSeconds < 0) || (rate < 0) || (reuse < 0))
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	int wrapType;
	int wrapOptions;
	if (argc >= 6)
	{
		wrapType = getIntArg(argv[5]);
		if (wrapType < 0)
		{
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
		setWrapImpl((WrapImplType)wrapType);
		if (argc >= 7)
		{
			wrapOptions = getIntArg(argv[6]);
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

	//  100 MB Array 25M int entries.
	int arraySize = 25 << 20;

	StartPhysicalTracer();
	startStatistics();

	int *data = (int *)pmalloc(arraySize * sizeof(int));
	printf("Array size=%d integers with start address: %p\n", arraySize, data);
	assert(data > 0);

	if (rate == 0)
		maxtrace(data, arraySize, wrapSize, reuse);
	else
		trace(data, arraySize, wrapSize, nSeconds, rate, reuse);

	EndPhysicalTracer();

	printf("%s \t%s \t%s \t%s \t%s\n", argv[0], argv[1], argv[2], argv[3], argv[4]);
	printStatistics(stdout);

	wrapCleanup();
	pfree(data);

	return 0;
}

