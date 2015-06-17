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

#include <stdio.h>
#include <pthread.h>
#include "wrap.h"

void *test(void *n)
{
  int *i = (int*)malloc(4);
  printf("Address i is %p\n", i);
  *i = 4;

  int *p = (int*)pmalloc(4);
  printf("Persistent Address p is %p\n", p);

  int w = wrapOpen();
  printf("Wrap token = %d\n", w);
  int eight = 8;
  wrapWriteInt(p, eight, w);
  wrapClose(w);
  
  //assert(8 == wrapLoad32(p, 0));
 // *p = 6;

  free(i);
  pfree(p);

  printf("Complete\n");
  return NULL;
}

int main(int argc, char *argv[])
{
	int ntesters = 1;

	setWrapImpl(Wrap_Software);

	/*
	int j = -1;
	uint32_t uj;
	int ujj;
	uj = j;
	ujj = uj;
	printf("%d %u %d\n", j, uj, ujj);
	*/

	if (argc > 2)
	{
		printf("Usage: %s [nthreads]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if (argc==2)
	{
		ntesters = atoi(argv[1]);
		if (ntesters <= 0)
		{
			printf("Invalid value for nthreds.\n");
			exit(EXIT_FAILURE);
		}
	}
	pthread_t *testers = (pthread_t *)malloc(ntesters * sizeof(pthread_t));

	for (int i = 0; i < ntesters; i++)
	{
		pthread_create(&testers[i], NULL, test, NULL);
	}

	for (int i = 0; i < ntesters; i++)
	{
		pthread_join(testers[i], NULL);
	}
	free(testers);
	pthread_exit(0);
	return 0;
}
