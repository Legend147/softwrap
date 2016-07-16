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

#ifndef __AliasTableHash_h
#define __AliasTableHash_h

#include <stdint.h>
#include "WrapLogManager.h"
#include "AliasTableBase.h"
#include "LFHT.h"

using namespace std;

class AliasTableHash: public AliasTableBase, public LFHT
{
public:
	AliasTableHash(WrapLogManager *manager, int size, int mt=1);
	~AliasTableHash();
	void onWrapOpen(int wrapToken);
	void onWrapClose(int wrapToken, WrapLogger *log);

	void *read(void *ptr, int size);
	void write(void *ptr, void *src, int size);

	void *load(void *ptr);
	void store(void *ptr, uint64_t value, int size);

	const char *getDetails();

	//  Restore all elements.
	void restoreAllElements();

	double getAvgRestoreTime();

	void clearHash();

	long m_totalRestores;

private:
	WrapLogManager *m_logManager;
	long m_restoreTime, m_logTime;
	int m_maxWrapToken;
};

#endif
