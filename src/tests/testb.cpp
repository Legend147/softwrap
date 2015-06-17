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
#include <sys/time.h>

#include <fstream>
#include <iostream>
#include <iomanip>

//  #include <ext/hash_set>
#include "btree.h"
#include <assert.h>
#include "palloc.h"

/// Time is measured using gettimeofday()
inline double timestamp()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec * 0.000001;
}

/// Traits used for the speed tests, BTREE_DEBUG is not defined.
template<int _innerslots, int _leafslots>
struct btree_traits_speed
{
	static const bool selfverify = false;
	static const bool debug = false;

	static const int leafslots = _innerslots;
	static const int innerslots = _leafslots;
};

const int randseed = 34234235;

//  Tokens for wrap begin and end.
int *WRAP_BEGIN_TOKEN = new int();
int *WRAP_END_TOKEN = new int();

template<class T>
class Tester
{
	T m_t;
public:

	Tester(int method, int isize, int tsize, int trans)
	{
		double ts1, ts2, ts3;

		ts1 = timestamp();

		init(isize);

		ts2 = timestamp();

		switch (method)
		{
		case 2:
			std::cerr << "Find" << std::endl;
			find(tsize, trans);
			break;
		case 3:
			std::cerr << "Insert/Find/Delete" << std::endl;
			ifd(tsize, trans);
			break;
		case 1:
		case 0:
		default:
			std::cerr << "Insert" << std::endl;
			insert(tsize, trans);
		}
		ts3 = timestamp();
		std::cerr << std::fixed << std::setprecision(10) << "Init=" << (ts2 - ts1) << " RunTime=" << (ts3-ts2) << std:: endl << std::flush;
	}

	void init(int isize)
	{
		srand(randseed);

		//  Initialize
		for (unsigned int i = 0; i < isize; i++)
			m_t.insert(rand());
		assert(m_t.size() == isize);
	}

	void ifd(int tsize, int ntransactions)
	{
		for (int i = 0; i < ntransactions; i++)
		{
			*WRAP_BEGIN_TOKEN = i;
			for (int j = 0; j < tsize; j++)
			{
				int r = rand();
				m_t.insert(r);
				//int s = rand();
				//m_t.find(s);
				//m_t.erase(m_t.lower_bound(s / 2));
				m_t.erase(r);
			}
			*WRAP_END_TOKEN = i;
		}
	}

	void find(int tsize, int ntransactions)
	{
		for (int i = 0; i < ntransactions; i++)
		{
			*WRAP_BEGIN_TOKEN = i;
			for (int j = 0; j < tsize; j++)
			{
				int s = rand();
				m_t.find(s);
			}
			*WRAP_END_TOKEN = i;
		}
	}

	void insert(int tsize, int ntransactions)
	{
		for (int i = 0; i < ntransactions; i++)
		{
			*WRAP_BEGIN_TOKEN = i;
			for (int j = 0; j < tsize; j++)
			{
				int r = rand();
				m_t.insert(r);
			}
			*WRAP_END_TOKEN = i;
		}
	}
};

int main(int argc, char*argv[])
{
	int *i = new int();
	std::cerr << "PROGRAM_START=" << i << std::endl;
	std::cout << i << std::endl;

	/*
	std::vector<int, pstd::PersistentAllocator<int> > v;

	for (argc = 0; argc < 10; argc++)
		v.push_back(100);

	std::set<int, std::less<int>, pstd::PersistentAllocator<int> > s;
	*/

	//printf("%d\n", argc);

	if (argc < 6)
	{
		fprintf(stderr, "Usage: %s benchmark method initialSize transactionSize numberTransactions\n", argv[0]);
		fprintf(stderr, "\tBenchmark is one of:  1-Set    2-MultiSet    3-B-Tree    4-B+Tree\n");
		fprintf(stderr, "\tMethod is one of:  1-Insert    2-Find    3-Insert/Find/Delete\n");
		exit(-1);
	}

	int benchmark = atoi(argv[1]);
	int method = atoi(argv[2]);
	int isize = atoi(argv[3]);
	int tsize = atoi(argv[4]);
	int ntrans = atoi(argv[5]);

	std::cout << WRAP_BEGIN_TOKEN << std::endl << WRAP_END_TOKEN << std::endl;
	std::cerr << "WRAP_BEGIN_TOKEN=" << WRAP_BEGIN_TOKEN << std::endl << "WRAP_END_TOKEN=" << WRAP_END_TOKEN << std::endl;


	std::cerr << "B-Tree" << std::endl;
	//typedef stx::btree<int, int, std::pair<int, int>, std::less<int>, struct btree_traits_speed<10, 10>, true, pstd::PersistentAllocator<int> > bt;
	Tester< stx::btree<int,int> > v3(method, isize, tsize, ntrans);
}
