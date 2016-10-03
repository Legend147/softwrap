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

#ifndef __WrapArray_H
#define __WrapArray_H


#include "wrap.h"
#include "WrapVar.h"
#include <stdio.h>

#define SIZE_INCR	4096

template<class T>
class WrapArray
{
private:
	WrapVar<T *> _pa;
	WrapVar<int> _length;
	WrapVar<int> _nextIndex;

	inline void resize(int size)
	{
		size+=SIZE_INCR;
		printf("resize to size=%d\n", size);
		WRAPTOKEN w = wrapOpen();
		T *pnew = (T*)pmalloc(size * sizeof(T));
		T *pold = _pa.get(w);
		int nexti = _nextIndex.get(w);

		for (int i = 0; i < nexti; i++)
		{
			T told;
			wrapRead(&told, &pold[i], sizeof(T), w);
			wrapWrite(&pnew[i], &told, sizeof(T), w);
		}
		printf("copied\n");
		char c = 0;
		for (int j = nexti *sizeof(T); j < (int)(size*sizeof(T)); j++)
			wrapWrite((char *)pnew + j, &c, 1, w);
		_length.set(size, w);
		_pa.set(pnew, w);  //  Wrap the save of the new data pointer.
		pfree(pold);
		wrapClose(w);
	}

public:

	inline WrapArray(int size = SIZE_INCR)
	{
		WRAPTOKEN w = wrapOpen();
		//  _pa, _length, and _nextIndex should be created by the constructor.
		//  Allocate the space for the array.
		T *arr = (T *)pmalloc(size * sizeof(T));
		//  Zero out the array.
		char c = 0;
		for (int i = 0; i < (int)(size * sizeof(T)); i++)
			wrapWrite((char *)arr + i, &c, 1, w);
		_pa.set(arr, w);
		_length.set(size, w);
		_nextIndex.set(0, w);
		//printf("In WrapArray(size=%d) _length %p = %d _nextIndex %p = %d\n", size, _length, *_length, _nextIndex, *_nextIndex);
		wrapClose(w);
	}


	inline ~WrapArray()
	{
		T *arr = _pa.get();
		pfree(arr);
		/*
		pfree(_pa);
		pfree(_length);
		pfree(_nextIndex);
		*/
	}

	inline T& operator[](int index)
	{
		//printf("In operator[%d] _length %p _nextIndex %p \n", index, _length, _nextIndex);
		if (index >= _length.get())
		{
			resize(index);
		}
		//printf("%d %d\n", *_length, *_nextIndex);
		if (index >= _nextIndex.get())
		{
			WRAPTOKEN w = wrapOpen();
			printf("writing next index\n");
			//int ii = index+1;
			_nextIndex.set(index+1, w);
			wrapClose(w);
		}

		return *(_pa.get() + index);
	}

	inline void add(T &val)
	{
		WRAPTOKEN w = wrapOpen();
		int nexti = _nextIndex.get(w);
		if (nexti == _length.get(w))
		{
			resize(nexti);
		}
		wrapWrite(_pa.get(w) + nexti, &val, sizeof(T), w);
		_nextIndex.set(nexti+1, w);
		wrapClose(w);
	}

	inline T get(int index)
	{
		if (index >= _length.get())
		{
			resize(index);
		}
		T tnew;
		wrapRead(&tnew, _pa.get() + index, sizeof(T), NULL);
		return tnew;
	}

	inline void set(int index, T value)
	{
		WRAPTOKEN w = wrapOpen();
		wrapWrite(&operator[](index), &value, sizeof(T), w);
		wrapClose(w);
	}

	inline int size()
	{
		return _length.get();
	}


};

#endif  //  __WrapArray_H

