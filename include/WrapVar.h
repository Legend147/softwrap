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

#ifndef __WrapVar_H
#define __WrapVar_H

#include "wrap.h"

#ifdef DEBUGWRAPVAR
#include "debug.h"
#define db	Debug
#else
#define db(x)
#endif

template<class T>
class WrapVar
{
private:
	T _ptr;
//	int _shadow;
public:
	WrapVar(T const other);
	WrapVar(const WrapVar<T> &other);
	WrapVar<T>& operator=(const WrapVar<T> &other);
	inline WrapVar() {};
	inline ~WrapVar() {};
	void set(T val, WRAPTOKEN w);
	void set(T val);
	T get(WRAPTOKEN w=0) const;

	WrapVar<T>& operator=(const T &rhs);

	WrapVar<T>& operator+=(T rhs);
	operator T() const
		{
		db("cast");
		return get();
		}
	WrapVar<T>& operator+=(const WrapVar<T>& rhs);
	WrapVar<T>& operator++();
	T operator++(int);
	WrapVar<T>& operator-=(const WrapVar<T>& rhs);
	WrapVar<T>& operator--();
	T operator--(int);
	//const T operator+(const T &other);
	//const WrapVar<T> operator+(const WrapVar<T> &other) const;

	//const T operator-(const T &other);
	//const WrapVar<T> operator-(const WrapVar<T> &other) const;

	inline T* getRef(WRAPTOKEN w=0)
	{
		assert(0);
		if (w == 0)
			{
				w = wrapOpen();
				T *t = getRef(w);
				wrapClose(w);
				return t;
			}

			db("getRef");
//			if (sizeof (T) == 4)
	//			return (T*)wrap
			return (T *)wrapRead((void *)&_ptr, sizeof(T), w);
	}
	inline T& operator*() const
	        {
			return getRef(0);
	         }

	        /// Dereference the iterator. Do not use this if possible, use key()
	        /// and data() instead. The B+ tree does not stored key and data
	        /// together.
	       /* inline T* operator->() const
	        {
	        	WRAPTOKEN w = wrapOpen();
	        	T *t = (T *)wrapRead((void *)&_ptr, sizeof(T), w);

	        	wrapClose(w);
	        	return t;
	        }*/

	/*
	//  Pointer Fun
	WrapVar<T> operator*();
	const WrapVar<T> operator*() const;
	WrapVar<T> operator->();
	const WrapVar<T> operator->() const;
	*/
};

/*
template<class T>
WrapVar<T>::WrapVar()
{
	db("Default constructor\n");
//	_ptr = (T *)pmalloc(sizeof(T));
};

template<class T>
WrapVar<T>::~WrapVar()
{
	db("Default destructor");
//	pfree(_ptr);
}
*/

template<class T>
void WrapVar<T>::set(T val, WRAPTOKEN w)
{
	db("set(T val, w)");
	if (sizeof(T) == 4)
	{
		uint64_t tval = (uint64_t)val;
		wrapStore32(&_ptr, (uint32_t)tval, w);
	}
	else if (sizeof(T) <= 8)
	{
		wrapStore64(&_ptr, (uint64_t)val, w);
	}
	else
	{
		assert(0);
		wrapWrite(&_ptr, &val, sizeof(T), w);
	}
};

template<class T>
void WrapVar<T>::set(T val)
{
	db("set");
	WRAPTOKEN w = wrapOpen();
	set(val, w);
	wrapClose(w);
}

template<class T>
T WrapVar<T>::get(WRAPTOKEN w) const
{
	if (w == 0)
	{
		w = wrapOpen();
		T t = get(w);
		wrapClose(w);
		return t;
	}

	db("get");
	if (sizeof(T) == 8)
	{
		return (T)wrapLoad64((void*)&_ptr, w);
	}
	else if (sizeof(T) == 4)
	{
		//  This makes gcc happy.
		void *v;
		v = (void*)&(_ptr);
		uint64_t u = wrapLoad32(v, w);
		return (T)u;
	}
	assert(0);
	T val = *(T *)wrapRead((void *)(&_ptr), sizeof(T), w);
	return val;
}

/*
 * Overload operators!
 */
template<class T>
WrapVar<T>& WrapVar<T>::operator=(const T &rhs)
{
	//  Check for self assignment.
	db("operator=");
	set(rhs);
	return *this;
}


template<class T>
WrapVar<T>::WrapVar(T const other)
{
	//_ptr = (T *)pmalloc(sizeof(T));
	set(other);
}

template<class T>
WrapVar<T>::WrapVar(const WrapVar<T> &other)
{
	db("WrapVar<T>::WrapVar(WrapVar<T> const &other\n");
	//_ptr = (T *)pmalloc(sizeof(T));
	set(other.get());
}



template<class T>
WrapVar<T>& WrapVar<T>::operator=(const WrapVar<T> &other)
{
	db("OTHER!\n");
	set(other.get());
	return *this;
}


template<class T>
WrapVar<T>& WrapVar<T>::operator+=(T rhs)
{
	db("operator+=");
	set(get() + rhs);
	return *this;
}

/*
template<class T>
T& WrapVar<T>::operator T() const
{
	db("operator T()");
	return get();
}
*/

template<class T>
WrapVar<T>& WrapVar<T>::operator++()
{
	db("operator++");
	set(get() + 1);
	return *this;
}

template<class T>
T WrapVar<T>::operator++(int)
{
	T t = get();
	operator++();
	return t;
}

template<class T>
WrapVar<T>& WrapVar<T>::operator+=(const WrapVar<T>& rhs)
{
	db("operatorrhswrap+=");
	set(get() + rhs.get());
	return *this;
}

template<class T>
WrapVar<T>& WrapVar<T>::operator--()
{
	db("operator--");
	set(get() - 1);
	return *this;
}

template<class T>
T WrapVar<T>::operator--(int)
{
	T t = get();
		operator--();
		return t;
}

template<class T>
WrapVar<T>& WrapVar<T>::operator-=(const WrapVar<T>& rhs)
{
	db("operatorrhswrap-=");
	set(get() - rhs.get());
	return *this;
}


/*
template<class T>
const T WrapVar<T>::operator+(const T &other)
{
	db("operator+");
	return get() + other;
}
*/
/*
template<class T>
const T WrapVar<T>::operator-(const T &other)
{
	db("operator+");
	return get() - other;
}
*/

/*
template<class T>
WrapVar<T> WrapVar<T>::operator*()
{
	return 0;
//	return get();
}

template<class T>
const WrapVar<T> WrapVar<T>::operator*() const
{
	return 0;
//	return get();
}

template<class T>
WrapVar<T> WrapVar<T>::operator->()
{
	return 0;
	//return get();
}

template<class T>
const WrapVar<T> WrapVar<T>::operator->() const
{
	return 0;
	//return get();
}
*/

#endif
