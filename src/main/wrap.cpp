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

#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "wrap.h"
#include "pmem.h"
#include "memtool.h"
#include "debug.h"
#include "tracer.h"
#include "WrapImpl.h"
#include "WrapImplSoftware.h"

WrapImplType getDefaultWrapImpl()
{
	WrapImplType wtype = MemCheck;
	char *s = getenv("WRAP");
	if (s != NULL)
	{
		errno = 0;
		int temp = strtol(s, NULL, 10);
		//fprintf(stderr, "WRAP=%s  temp=%d  errno=%d\n", s, temp, errno);
		if ((errno == 0) && (temp >= 0) && (temp <= 8))
			wtype = (WrapImplType)temp;
	}
	return wtype;
}

WrapImplType _wrapImplType = getDefaultWrapImpl();
int _usingSCM = 1;
WrapImpl *_wrapImplementation = NULL;
pthread_rwlock_t _lock = PTHREAD_RWLOCK_INITIALIZER;


void setWrapImpl(WrapImplType wrapimpltype)
{
	if (wrapimpltype == NoSCM)
		_usingSCM = 0;

	_wrapImplType = wrapimpltype;
}

WrapImpl *getWrapImpl()
{
	if (_wrapImplementation == NULL)
	{
		pthread_rwlock_wrlock(&_lock);
		//  Now we have the lock.
		//  Check to make sure the implementation is still null.
		if (_wrapImplementation == NULL)
			_wrapImplementation = WrapImpl::getWrapImpl(_wrapImplType);
		//  Release
		pthread_rwlock_unlock(&_lock);
	}
	return _wrapImplementation;
}

void wrapCleanup()
{
	if (_wrapImplementation != NULL)
	{
		delete _wrapImplementation;
		_wrapImplementation = NULL;
	}
}

void wrapFlush()
{
	getWrapImpl()->flush();
}
void setAliasTableWorkers(int num)
{
	WrapImplSoftware::setAliasTableWorkers(num);
}

void setWrapImplOptions(int options)
{
	getWrapImpl()->setOptions(options);
}

void printWrapImplOptions(FILE *stream)
{
	char c[1024];
	getWrapImpl()->getOptionString(c);
	fprintf(stream, "\t%s\n", c);
}

void printWrapImplTypes(FILE *stream)
{

	//MemCheck, NoSCM, NoGuarantee, NoAtomicity, UndoLog, RedoLog, Wrap_Software, Wrap_Hardware, Wrap_Memory
	fprintf(stream, "\tMemCheck=%d (default), NoSCM=%d, NoGuarantee=%d, NoAtomicity=%d, UndoLog=%d, RedoLog=%d,\n",
			MemCheck, NoSCM, NoGuarantee, NoAtomicity, UndoLog, RedoLog);
	fprintf(stream, "\tWrap_Software=%d, Wrap_Hardware=%d, Wrap_Memory=%d\n",
			Wrap_Software, Wrap_Hardware, Wrap_Memory);
}


void startPinStatistics()
{

}

void startStatistics()
{
	getWrapImpl()->startStatistics();
	startMemStats();
	startPinStatistics();
}

void getPinStatistics(char *c)
{
	sprintf(c, "Pin not running or overloading the getPinStatistics function.");
}
void getStatistics(char *c)
{
	char tc[1024];
	getPinStatistics(tc);
	sprintf(c, "ntstore= %d \tp_msync= %d \twritecomb= %d \t%s \t" \
			"totalWrapStreams= %d \ttotalBytesStreamed= %ld",
			getNumNtStores(), getNumPMSyncs(), getNumWriteComb(), tc,
			getWrapImpl()->totWrapStreams, getWrapImpl()->totBytesStreamed);
}

void getAllStatistics(char *c)
{
	char a[1024];
	getWrapImpl()->getStatistics(a);
	char b[1024];
	getStatistics(b);
	sprintf(c, "WRAPSTATS \t%s \t%s", a, b);
}

void printStatistics(FILE *stream)
{
	char c[2048];
	getAllStatistics(c);
	fprintf(stream, "\n%s\n", c);
}


WrapImplType getWrapImplType()
{
	return getWrapImpl()->getWrapImplType();
}

const char *getAliasDetails()
{
	return getWrapImpl()->getAliasDetails();
}


WRAP_TLS int nWrapsOpen = 0;
WRAP_TLS int isWrapStreamOpen = 0;
WRAP_TLS WRAPTOKEN topToken = 0;
WRAPTOKEN wrapOpen()
{
	nWrapsOpen++;
	//  Pin should inject an instrument before return value.
	getWrapImpl()->wrapStatOpen();
	if (isWrapStreamOpen)
	{
		assert(nWrapsOpen > 0);
		return 1;
	}
	topToken = getWrapImpl()->wrapImplOpen();
	return topToken;
}

int wrapClose(WRAPTOKEN w)
{
	nWrapsOpen--;
	//  Pin should inject an instrument before entry value.
	getWrapImpl()->wrapStatClose();
	if (isWrapStreamOpen)
	{
		assert(nWrapsOpen > 0);
		return 1;
	}
	return getWrapImpl()->wrapImplClose(w);
}


WRAPTOKEN wrapStreamOpen()
{
	if (nWrapsOpen == 0)
	{
		isWrapStreamOpen = 1;
		topToken = 1;
		getWrapImpl()->wrapStatStream();
	}
	nWrapsOpen++;
	getWrapImpl()->wrapStatOpen();
	return topToken;
}

int wrapStreamClose(WRAPTOKEN w)
{
	nWrapsOpen--;
	if (nWrapsOpen == 0)
		isWrapStreamOpen = 0;
	getWrapImpl()->wrapStatClose();
	return 1;
}


void wrapWrite(void *ptr, void *src, int size, WRAPTOKEN w)
{
	if (isWrapStreamOpen)
	{
		Debug("wrapWriteStream %p %p %d\n", ptr, src, size);
		ntstore(ptr, src, size);
		getWrapImpl()->wrapStatStreamWrite(size);
		return;
	}
	if (isDebug())
	{
		Debug("wrapWrite %p %p %d\n", ptr, src, size);
		//assert(isInPMem(ptr));
	}
	getWrapImpl()->wrapStatWrite(size);
	getWrapImpl()->wrapImplWrite(ptr, src, size, w);
}

size_t wrapRead(void *ptr, const void *src, size_t size, WRAPTOKEN w)
{
	Debug("wrapRead ptr=%p src=%p size=%d\n", ptr, src, size);
	getWrapImpl()->wrapStatRead(size);
	return getWrapImpl()->wrapImplRead(ptr, src, size, w);
}

uint64_t wrapLoad64(void *ptr, WRAPTOKEN w)
{
	Debug("wrapLoad64 ptr=%p\n", ptr);

	//getWrapImpl()->wrapStatLoad();
	getWrapImpl()->wrapStatRead(8);

	return getWrapImpl()->wrapImplLoad64(ptr, w);
}

uint32_t wrapLoad32(void *ptr, WRAPTOKEN w)
{
	Debug("wrapLoad32 ptr=%p\n", ptr);

	//getWrapImpl()->wrapStatLoad();
	getWrapImpl()->wrapStatRead(4);

	return getWrapImpl()->wrapImplLoad32(ptr, w);
}

uint16_t wrapLoad16(void *ptr, WRAPTOKEN w)
{
	Debug("wrapLoad16 ptr=%p\n", ptr);

	//getWrapImpl()->wrapStatLoad();
	getWrapImpl()->wrapStatRead(2);

	return getWrapImpl()->wrapImplLoad16(ptr, w);
}

uint8_t wrapLoadByte(void *ptr, WRAPTOKEN w)
{
	Debug("wrapLoadByte ptr=%p\n", ptr);

	//getWrapImpl()->wrapStatLoad();
	getWrapImpl()->wrapStatRead(1);

	return getWrapImpl()->wrapImplLoadByte(ptr, w);
}


void wrapStore64(void *ptr, uint64_t value, WRAPTOKEN w)
{
	if (isWrapStreamOpen)
	{
		ntstore(ptr, &value, sizeof(value));
		getWrapImpl()->wrapStatStreamWrite(sizeof(value));
		return;
	}
	//getWrapImpl()->wrapStatStore();
	getWrapImpl()->wrapStatWrite(8);

	getWrapImpl()->wrapImplStore64(ptr, value, w);
}

void wrapStore32(void *ptr, uint32_t value, WRAPTOKEN w)
{
	if (isWrapStreamOpen)
	{
		ntstore(ptr, &value, sizeof(value));
		getWrapImpl()->wrapStatStreamWrite(sizeof(value));
		return;
	}
	//getWrapImpl()->wrapStatStore();
	getWrapImpl()->wrapStatWrite(4);

	getWrapImpl()->wrapImplStore32(ptr, value, w);
}

void wrapStore16(void *ptr, uint16_t value, WRAPTOKEN w)
{
	if (isWrapStreamOpen)
	{
		ntstore(ptr, &value, sizeof(value));
		getWrapImpl()->wrapStatStreamWrite(sizeof(value));
		return;
	}
	//getWrapImpl()->wrapStatStore();
	getWrapImpl()->wrapStatWrite(2);

	getWrapImpl()->wrapImplStore16(ptr, value, w);
}

void wrapStoreByte(void *ptr, uint8_t value, WRAPTOKEN w)
{
	if (isWrapStreamOpen)
	{
		ntstore(ptr, &value, sizeof(value));
		getWrapImpl()->wrapStatStreamWrite(sizeof(value));
		return;
	}
	//getWrapImpl()->wrapStatStore();
	getWrapImpl()->wrapStatWrite(1);

	getWrapImpl()->wrapImplStoreByte(ptr, value, w);
}

void persistentNotifyPin(void *v, size_t size, int isLogArea)
{
	//  Pin should overload this routine if running.

}

void persistentSHMCreated(void *v, size_t size, bool isLogArea)
{
	if (_usingSCM)
		persistentNotifyPin(v, size, isLogArea);
}


void pfreeNotifyPin(void *p)
{
	//  Pin should overload this routine if running and remove the address from the pmem.cpp list.
}


//  TODO Strip out as a class
class PM
{
	bool _usemmap;
	int _fdout;
	void *_base;
	unsigned _last;
	unsigned _memchunk;
#define MemSize (1<<16)
public:
	PM()
{
		_usemmap = false;
		_base = NULL;
		_last = 0;
		char *s = getenv("SCM");
		if ((s != NULL) && (strlen(s) > 0))
		{
			_usemmap = true;
			/* open/create the output file */
			errno = 0;
			if ((_fdout = open (s, O_RDWR | O_CREAT)) < 0)
			{
				perror("Error creating file.\n");
				fprintf(stderr, "can't create %s for writing", s);
				abort();
			}
			lseek(_fdout, MemSize - 1, SEEK_SET);
			write (_fdout, "", 1);
			/* mmap the file. */
			errno = 0;
			if ((_base = mmap (0, MemSize, PROT_READ | PROT_WRITE, MAP_SHARED, _fdout, (off_t)0)) == (void *) -1)
			{
				perror("Error in creating mmap.\n");
				abort();
			}
		}
		if (_usemmap==0)
		{
			_memchunk = 1<<27;
			_last = _memchunk+1;
			_base = NULL;
		}
}
	~PM()
	{
		if (_usemmap)
		{
			msync(_base, MemSize, MS_SYNC);
			munmap(_base, MemSize);
			close(_fdout);
		}
	}
	void *PMalloc(size_t size, bool isLogArea)
	{
		if (_memchunk>0)
		{
			void *v = (void *)((uint64_t)_base + _last);
			_last += size;
			if (_last > _memchunk)
			{
				_base = malloc(_memchunk);
				v = _base;
				size = 0;
				allocatedPMem(_base, _memchunk);
				_last += size;
			}
			Debug("PMalloc(size=%d) _last=%d _memchunk=%d\n", size, _last, _memchunk);
			return v;
		}
		if (_usemmap)
		{
			void *v = (void *)((uint64_t)_base + _last);
			_last += size;
			Debug("PMalloc(size=%d) _last=%d MemSize=%d\n", size, _last, MemSize);
			assert(_last < MemSize);
			return v;
		}
		//	Debug("%d ", (int)size);
		//	Debug("%d\n", (int)size);
		//void *v = malloc(size);
		void *v = mallocAligned(size);
		if (_usingSCM)
		{
			if (isDebug())
				allocatedPMem(v, size);	
			persistentNotifyPin(v, size, isLogArea);
			persistentNotifyTracer(v, size);
		}
		Debug("pmalloc(%d) = %p to %p\n", (int)size, v, (void *)(size + (unsigned long)v));
		return v;
	}
	void PFree(void *ptr)
	{
		if (_usemmap || (_memchunk>0))
			return;
		//  Call into pin and mark as free.
		if (_usingSCM)
		{
			if (isDebug())
				freedPMem(ptr);
			pfreeNotifyPin(ptr);
			//  If Physical Tracer then free and don't return.
			if (pfreeNotifyTracer(ptr))
				return;
		}
		free(ptr);
	}
	void *PCalloc(size_t nmemb, size_t size)
	{
		size_t tsize = (nmemb * size);
		void *p = PMalloc(tsize, 0);
		int *pi = (int*)p;
		for (unsigned i = 0; i < (tsize / 4); i++)
			__builtin_ia32_movnti(pi++, 0);

		//  Get the last bytes that are unaligned if any.
		char *b = (char *)p;
		for (unsigned i = 0; i < (tsize&3); i++)
		{
			b[size-i-1] = 0;
			clflush(b);
		}
		return p;
	}
	void *PRealloc(void *ptr, size_t size)
	{
		if (_usemmap || (_memchunk>0))
		{
			void *p = PMalloc(size, 0);
			bcopy(p, ptr, size);
			PFree(ptr);
			return p;
		}
		void *p = realloc(ptr, size);
		if (isDebug() && _usingSCM)
			reallocedPMem(p, size, ptr);
		return p;
	}
};

PM _pm;

void *pmalloc(size_t size)
{
	return _pm.PMalloc(size, false);
}

void *pmallocLog(size_t size)
{
	return _pm.PMalloc(size, true);
}


void pfree(void *ptr)
{
	return _pm.PFree(ptr);
}

void *pcalloc(size_t nmemb, size_t size)
{
	return _pm.PCalloc(nmemb, size);
}
void *prealloc(void *ptr, size_t size)
{
	return _pm.PRealloc(ptr, size);
}

/*
int m_debugLevel = 0;

void Debug(int level, char *format, ...)
{
	if (level <= m_debugLevel)
	{
		va_list argptr;
		va_start(argptr, format);
		va_end(argptr);
		vprintf(format, argptr);
	}

}

void setDebugLevel(int level)
{
	m_debugLevel = level;
}
int getDebugLevel()
{
	return m_debugLevel;
}
 */


class cleanup
{
public: inline ~cleanup()  { wrapCleanup(); }
};

cleanup c;
/*
int isSCM()
{
	return (WrapImplementation != NoSCM);
}
 */
/*
void setAbortOnUnWrapped(int b)
{
	//  Pin should overload this routine if running.
}
 */
