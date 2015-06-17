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

#include <string>
#include <stdlib.h>
#include "wrap.h"
#include "memtool.h"
#include "WrapBTree.h"
//#include "btree.h"

int doRandom = (getenv("WRAPRAND")==NULL)?0:1;
const int randseed = 34234235;
long inum = 0;

/// Traits used for the speed tests, BTREE_DEBUG is not defined.
template<int _innerslots, int _leafslots>
struct btree_traits_speed
{
	//static const bool selfverify = false;
	//static const bool debug = false;

	static const int leafslots = _innerslots;
	static const int innerslots = _leafslots;
};


typedef stx::btree<int, int> bt;
bt *m_t;

void init(unsigned int isize)
{
	m_t = new bt();

	if (doRandom)
		printf("Random\n");
	else
		printf("Sequential\n");

	srand(randseed);

	//  Initialize
	WRAPOPEN;
	for (unsigned int i = 0; i < isize; i++)
	{
		if (doRandom)
			m_t->insert(rand());
		else
			m_t->insert(inum++);
	}
	assert(m_t->size() == isize);
	WRAPCLOSE;
}

void ifd(int tsize, int ntransactions)
{
	long r;
	for (int i = 0; i < ntransactions; i++)
	{
		WRAPOPEN;
		for (int j = 0; j < tsize; j++)
		{
			if (doRandom)
				r = rand();
			else
				r = inum++;
			m_t->insert(r);
			m_t->erase(r);
		}
		WRAPCLOSE;
	}
}

void find(int tsize, int ntransactions)
{
	long r;
	for (int i = 0; i < ntransactions; i++)
	{
		//printf("Transaction %d\n", i);
		WRAPOPEN;
		for (int j = 0; j < tsize; j++)
		{
			if (doRandom)
				r = rand();
			else
				r = rand()%inum;
			m_t->find(r);
		}
		WRAPCLOSE;
	}
}

void insert(int tsize, int ntransactions)
{
	long r;
	for (int i = 0; i < ntransactions; i++)
	{
		//printf("Transaction %d\n", i);
		WRAPOPEN;
		for (int j = 0; j < tsize; j++)
		{
			if (doRandom)
				r = rand();
			else
				r = inum++;
			m_t->insert(r);
		}
		WRAPCLOSE;
	}
}

void insertDetailed(int tsize, int ntransactions)
{
	long startTime = getNsTime();
	long r = 0;

	for (int i = 1; i <= ntransactions; i++)
	{
		//printf("Transaction %d\n", i);
		WRAPOPEN;
		for (int j = 0; j < tsize; j++)
		{
			if (doRandom)
				r = rand();
			else
				r = inum++;
			m_t->insert(r);
			// printf("%ld\n", r);
		}
		WRAPCLOSE;
		if (((i <= 10000) && ((i/1000) * 1000 == i)) ||
				((i/50000) * 50000 == i))
		{
			//wrapFlush();
			printf("Transaction= \t%d \ttotalTime= \t%ld\n", i, getNsTime() - startTime);
		}
	}
}

void testBTree(int method, unsigned isize, int tsize, int trans)
{
	long ts1, ts2, ts3;

	ts1 = getNsTime();

	init(isize);

	ts2 = getNsTime();

	switch (method)
	{
	case 2:
		printf("Find:\n");
		find(tsize, trans);
		break;
	case 3:
		printf("Insert/Find/Delete\n");
		ifd(tsize, trans);
		break;
	case 4:
		printf("Insert\n");
		insertDetailed(tsize, trans);
		break;
	case 1:
	case 0:
	default:
		printf("Insert\n");
		insert(tsize, trans);
	}
	ts3 = getNsTime();
	printf("Init= %ld \tRunTime= %ld\n", (ts2 - ts1), (ts3-ts2));

#ifdef BTREE_DEBUG
	std::cout << "\n\n\n\nTREE:\n\n";
	m_t->print(std::cout);
#endif
}

void usage(char *s)
{
	fprintf(stderr, "Usage: %s method initialSize transactionSize numberTransactions [wrapType [options]]\n", s);
	fprintf(stderr, "\tMethod is one of:  1-Insert   2-Find   3-Insert/Find/Delete   4-InsertDetailed\n");
	fprintf(stderr, "\tUser environment variable WRAPRAND is one then random is performed.\n");
	fprintf(stderr, "\tWhere wrapType is one of:\n");
	printWrapImplTypes(stderr);
	fprintf(stderr, "\tFor type %d the options are:\n", getWrapImplType());
	printWrapImplOptions(stderr);
}

int main(int argc, char*argv[])
{
	if (argc < 6)
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	int method = atoi(argv[1]);
	int isize = atoi(argv[2]);
	int tsize = atoi(argv[3]);
	int ntrans = atoi(argv[4]);
	if (argc >= 6)
	{
		int wrapType = atoi(argv[5]);
		if (wrapType < 0)
		{
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
		setWrapImpl((WrapImplType)wrapType);
		if (argc >= 7)
		{
			int wrapOptions = atoi(argv[6]);
			if (wrapOptions < 0)
			{
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			setWrapImplOptions(wrapOptions);
		}
	}

	startStatistics();

	testBTree(method, (unsigned)isize, tsize, ntrans);

	printStatistics(stdout);
}
