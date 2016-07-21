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

#ifndef _WRAPIMPL_H
#define _WRAPIMPL_H

#include <stdint.h>
#include "wrap.h"
#include "WrapLogger.h"
#include "WrapLogManager.h"

class WrapImpl
{
public:
	//MemCheck, NoSCM, NoGuarantee, NoAtomicity, UndoLog, RedoLog, Wrap_Software, Wrap_Hardware, Wrap_Memory
	virtual int wrapImplOpen();
	virtual int wrapImplClose(int wrapToken);

	virtual void wrapImplWrite(void *ptr, void *src, int size, WRAPTOKEN w);
	virtual size_t wrapImplRead(void *ptr, const void *src, size_t size, WRAPTOKEN w);
	virtual void wrapImplStore64(void *ptr, uint64_t value, WRAPTOKEN w);
	virtual void wrapImplStore32(void *ptr, uint32_t value, WRAPTOKEN w);
	virtual uint64_t wrapImplLoad64(void *ptr, WRAPTOKEN w);
	virtual uint32_t wrapImplLoad32(void *ptr, WRAPTOKEN w);

	//virtual void wrapStore(void *ptr, uint64_t value, int size, WRAPTOKEN w);
	//virtual uint64_t wrapLoad(void *ptr, WRAPTOKEN w);


	virtual void getOptionString(char *c);

	//  Statistics for now are non-virtual.
	void startStatistics();
	void getStatistics(char *c);

	static WrapImpl *getWrapImpl(WrapImplType t);
	static inline WrapImplType getWrapImplType() { return m_wrapImplType; }

	virtual const char *getAliasDetails();

	virtual void flush() {};

	//void restoreLogs();

	static inline void setOptions(int options)
	{
		m_options = options;
	}

	static inline int getOptions()
	{
		return m_options;
	}

	//  Stat routines.
	static inline void wrapStatOpen()
	{
		totWrapOpen++;
		if ((totWrapOpen - totWrapClose) > maxWrapDepth)
			maxWrapDepth = totWrapOpen - totWrapClose;
		if ((totWrapOpen - totWrapClose) == 1)
			totUniqueWraps++;
	}

	static inline void wrapStatClose()
	{
		totWrapClose++;
	}

	static inline void wrapStatRead(int nbytes)
	{
		totWrapRead++;
		totBytesRead+=nbytes;
		if (nbytes > maxBytesRead)
			maxBytesRead = nbytes;
	}

	static inline void wrapStatWrite(int nbytes)
	{
		totWrapWrite++;
		totBytesWrite+=nbytes;
		if (nbytes > maxBytesWrite)
			maxBytesWrite = nbytes;
	}

	/*
	static inline void wrapStatLoad()
	{
		totWrapLoad++;
	}

	static inline void wrapStatStore()
	{
		totWrapStore++;
	}
	*/

	/*
	static inline void setMaxLogSize(int bytes)
	{
		m_maxLogSize = bytes;
	}
	*/

	virtual ~WrapImpl();
	long elapsedTime();

protected:
	static WrapImplType m_wrapImplType;
	static int m_options;
	WrapImpl();

	//  TLS Stats object??  TODO
	//  Statistics
	static int totWrapOpen;
	static int totWrapClose;
	static int totWrapRead;
	static int totWrapWrite;
	static int totWrapLoad;
	static int totWrapStore;
	static int maxWrapDepth;
	static int totUniqueWraps;
	static long totBytesWrite;
	static long totBytesRead;
	static int maxBytesRead;
	static int maxBytesWrite;

	//  Log
	//static int m_maxLogSize;
	//WrapLogManager *m_logManager;
	//WrapLogger *getWrapLog(WRAPTOKEN w);
	WrapLogManager *_logManager;

	//  Time calcs.
	static long totalWrapTime;
};

#endif
