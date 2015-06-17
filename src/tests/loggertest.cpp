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

#include <assert.h>
#include <strings.h>
#include "WrapLogger.h"

void inttest()
{
	char *c = (char*)malloc(10<<10);

	WrapLogger w;
	w.initialize(c, 8192);
	w.wrapLogOpen(1);

	int i[2048];
	bzero(i, 8192);

	for (int j = 0; j < 2048; j++)
	{
		w.addWrapLogEntry(&(i[j]), &j, 4, 1);
		assert(i[j] == 0);
	}

	w.wrapLogClose();
	int restored = w.restoreWrapLogEntries(c);
	printf("restored = %d\n", restored);

	for (int j = 0; j < 2048; j++)
	{
		printf("i[%d] = %ld\n", j, i[j]);
		assert(i[j] == j);
	}
	printf("passed tests.\n");
}

void longinttest()
{
	char *c = (char*)malloc(10<<10);

	WrapLogger w;
	w.initialize(c, 8192);
	w.wrapLogOpen(1);

	long int i[1024];
	bzero(i, 1024*8);

	long int t;
	for (long int j = 0; j < 1024; j++)
	{
		t = j << 2;
		w.addWrapLogEntry(&(i[j]), &t, 8, 1);
		assert(i[j] == 0);
	}

	w.wrapLogClose();
	int restored = w.restoreWrapLogEntries(c);
	printf("restored = %d\n", restored);

	for (long int j = 0; j < 1024; j++)
	{
		printf("i[%d] = %ld\n", j, i[j]);
		assert(i[j] == (j<<2));
	}
	printf("passed tests.\n");
}

int main()
{
	inttest();
	longinttest();
	return 0;
}
