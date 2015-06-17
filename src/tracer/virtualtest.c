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
#include <unistd.h>

#define PAGE_FLAG(x,bit)	(x & ((long)1<<bit))>>bit
#define PAGE_PRESENT(x)	PAGE_FLAG(x,63)
#define PAGE_SWAPPED(x)	PAGE_FLAG(x,62)
#define PFN(x)	(x & (((long)1<<55) - 1))
#define PAGE_SHIFT(x)	((x & (((long)1<<61)-1)) >> 55)

int main(void)
{
  pid_t pid = getpid();

  printf("pid=%d\n", pid);
  
  long ll = 819101234;
  long *l = &ll;
  long *nl;

  printf("%d %p\n", pid, l);

  int j;

  char fname[128];
  sprintf(fname, "/proc/%d/pagemap", pid);
  FILE *f = fopen(fname, "r");
  if (f)
  {
	unsigned long pagesize = getpagesize();
	long ofs = (long)((long)l / pagesize);
	unsigned long b;
	fseek(f, ofs << 3, SEEK_SET);
	fread(&b, 8, 1, f);
	nl = (long *)(PFN(b)<<PAGE_SHIFT(b)) + ((unsigned long)l & (pagesize - 1));
	printf("%p -> %ld  pfn = %ld and page shift = %lu and pointer = %p\n", l, *l, PFN(b), PAGE_SHIFT(b), nl);
	
	FILE *ff = fopen("/dev/fmem", "r");
	if (ff)
	{
		unsigned long nl2 = (unsigned long)nl;	
		long l2;
		unsigned long fs = fseek(ff, nl2, SEEK_SET);
		unsigned long fr = fread(&l2, 8, 1, ff);
		printf("fs=%ld fr=%ld val=%ld\n", fs, fr, l2);
		fseek(ff, (nl2>>PAGE_SHIFT(b))<<PAGE_SHIFT(b), SEEK_SET);
		

		scanf("%d",&j);

		/*
		int i;
		for (i = 0; i < pagesize; i+=8)
		{
			fread(&l2, 8, 1, ff);
			printf("%ld\n", l2);	
		}
		 */
	}
	else
	{
		fprintf(stderr, "Couldn't open /dev/mem\n");
	}
  }
  else
  {
	fprintf(stderr, "Couldn't open file.");
  }
return 0;
}
