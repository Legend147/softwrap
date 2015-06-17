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

#include "WrapVar.h"

typedef WrapVar<int> WrapInt;

struct PS
{
	WrapInt l[20];
	//static constexpr WrapInt i = 5;
};
int main(void)
{
	startStatistics();

	WrapInt *i = (WrapInt*)pmalloc(sizeof(WrapInt));
	//int i;

	*i = 5;
	int j;
	j = *i;

	printf("i=%d\n", (int)*i);


	*i+=1;
	*i = *i - 1;

	printf("i=%d\n", (int)*i);

	if (*i==j)
	{
		printf("i==j\n");

	}
	else
		printf("not equal\n");

	PS *s = (PS*)pmalloc(sizeof(PS));
	s->l[2] = 2;
	printf("s->l[2] = %d\n", (int)s->l[2]);

	WrapInt *k = (WrapInt*)pmalloc(sizeof(WrapInt) * 10);
	k[0] = 1234;

	printf("k[0]=%d\n", (int)k[0]);

	k[8] = 5;
	k[4] = 3;

	printf("trying intermediate int method:\n");
	int a = k[4];
	k[8] = a;  //  Should be 3.
	printf("Done, should be 3.\n");
	printf("k[8] = %d\n", (int)k[8]);

	printf("trying direct method:\n");
	k[2] = k[0];  //  Should be 1234.
	printf("Done, should be 1234.\n");
	printf("k[2] = %d\n", (int)k[2]);

	WrapInt e = 5;
	WrapInt f = 6;
	printf("Trying copy\n");
	e = f;

	printStatistics(stdout);

	return 0;
}
