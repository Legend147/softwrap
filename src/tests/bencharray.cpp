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

#include "WrapArray.h"
#include "wrap.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

void usage(char *c)
{
	fprintf(stderr, "Usage: %s initialSize transactionSize numberTransactions [wrapType [options]]\n", c);
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

	int initialSize = getIntArg(argv[1]);
	int transactionSize = getIntArg(argv[2]);
	int numberTransactions = getIntArg(argv[3]);

	if ((initialSize < 0) || (transactionSize < 0) || (numberTransactions < 0))
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	if (argc >= 5)
	{
		int wt = getIntArg(argv[4]);
		if (wt < 0)
		{
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
		setWrapImpl((WrapImplType)wt);
		if (argc >= 6)
		{
			int options = getIntArg(argv[5]);
			if (options < 0)
			{
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			setWrapImplOptions(options);
		}
	}

	WrapArray<int> array(initialSize);

	//  Initialize the size of the array.
	for (int i = 0; i < initialSize; i++)
	{

		//		array.add(i);
	}

	startStatistics();

	for (int i = 0; i < numberTransactions; i++)
	{
		WRAPTOKEN w = wrapOpen();
		for (int j = 0; j < transactionSize; j++)
		{
			//  clever trick we can do is that we know this data structure is wrapped, so no
			//  need to explicitly wrap the assignments.
			array.set(random()%initialSize, random());
		}
		wrapClose(w);
	}

	printf("%s \t%s \t%s \t%s \t", argv[0], argv[1], argv[2], argv[3]);
	printStatistics(stdout);

	return 0;
}

