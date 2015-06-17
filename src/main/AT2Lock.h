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

#ifndef __AT2_h
#define __AT2_h

#include <stdint.h>
#include "WrapLogManager.h"
#include "AliasTableBase.h"
#include "AliasTableHash.h"

using namespace std;

class AT2: public AliasTableBase
{
public:
	AT2(WrapLogManager *manager, int size, int thresh);
	~AT2();
	void onWrapOpen(int wrapToken);
	void onWrapClose(int wrapToken, WrapLogger *log);

	void *read(void *ptr, int size);
	void write(void *ptr, void *src, int size);

	uint64_t load(void *ptr);
	void store(void *ptr, uint64_t value, int size);

	const char *getDetails();

	//  Restore all elements.
	void restoreAllElements();

private:
	WrapLogManager *m_logManager;

	//  Two Alias Tables managed by this super class.
	AliasTableHash *m_atA;
	AliasTableHash *m_atB;
	//  Size threshold.
	int m_thresh;

	typedef enum {Active, Full, Retiring, Empty} TableState;
	TableState m_atAState;
	TableState m_atBState;

	//  Thread's copy of Alias Table pointer.
	typedef AliasTableHash * atp;
	static atp __thread h;
	AliasTableHash *m_aliasTableHash;

	static void *retireThreadFunc(void *t);
	void retireThread();
};

#endif
