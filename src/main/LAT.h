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

#ifndef __LAT_h
#define __LAT_h

#include <stdint.h>
#include "WrapLogger.h"
#include "WrapLogManager.h"
#include "AliasTableBase.h"
#include "bloom.h"

using namespace std;

#define MAX_LAT_ENTRIES	65536
#define MAX_LAT_DATA	(MAX_LAT_ENTRIES * 8)

class LAT : public AliasTableBase
{
public:
		LAT(WrapLogManager *manager, int size);
		~LAT();
		void onWrapOpen(int wrapToken);
		void onWrapClose(int wrapToken, WrapLogger *log);
		void copyEntryToCache(int i);
		void writebackEntry(int i);
		void restoreAllElements();

		size_t readInto(void *ptr, const void *src, size_t size);
		void *read(void *ptr, int size) { assert(0); return NULL; }
		void write(void *ptr, void *src, int size);

		void *load(void *ptr);
		void store(void *ptr, uint64_t value, int size);

		const char *getDetails();

private:
		struct Entry
			{
			    void *key;
			    uint64_t value;
			    int size;
			};
		static __thread struct Entry m_entries[MAX_LAT_ENTRIES];

		static __thread int m_lastIndex;
		static __thread char m_data[MAX_LAT_DATA];
		static __thread int m_lastData;
		void restoreToCacheHierarchy();

		BloomFilter m_bloom;
		int m_delayWriteBack;
};

#endif
