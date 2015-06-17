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

	threadToCore(11);

	double d = 0.0;
	assert(nSeconds >= 3);
	long start, recordStart;
	start = getNsTime();
	long IgnoreFirst = 0;
	recordStart = 0;
	printf("start\n");
	do
	{
		d = (count-IgnoreFirst) * (1000000000.0 / rate) + recordStart;
		long t1 = getNsTime();
		if (d > t1)
			d = t1;
		//printf("%f %ld, count=%ld ignore=%ld\n", d, recordStart, count, IgnoreFirst);
		txStartTime = d;


		WRAPTOKEN w = wrapOpen();

		TODO!

		wrapClose(w);


		txEndTime = getNsTime();
		txElapsed = txEndTime - txStartTime;
		count++;
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
	printf("%f avgservice\n", txE d / (count-IgnoreFirst));
}

void maxtrace(int *data, int arraySize, int wrapSize, int reuse)
{
	int j, index;
	long start = 0;


	printf("start\n");
	threadToCore(11);

	for (int i=0; i < 1000000; i++)
	{
		if (i == 250000)
			start = getNsTime();
		WRAPTOKEN w = wrapOpen();


		wrapClose(w);
	}

	double txps = 749999;
	txps *= 1000000000;
	wrapFlush();
	txps /= (getNsTime() - start);

	printf("\n%f txps\n", txps);
}


void usage(char *c)
{
	fprintf(stderr, "Usage: %s wrapSize nSeconds rate initSize [wrapType [options]]\n", c);
	fprintf(stderr, "\tRun an insert into a btree of wrapSize access in each wrap for at least nSeconds.\n" \
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
	int initSize = getIntArg(argv[4]);

	if ((wrapSize < 0) || (nSeconds < 0) || (rate < 0) || (initSize < 0))
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

	StartPhysicalTracer();
	startStatistics();

	//  init here

	if (rate == 0)
		maxtrace(data, arraySize, wrapSize);
	else
		trace(data, arraySize, wrapSize, nSeconds, rate);

	EndPhysicalTracer();

	printf("%s \t%s \t%s \t%s \t%s\n", argv[0], argv[1], argv[2], argv[3], argv[4]);
	printStatistics(stdout);

	wrapCleanup();

	return 0;
}

