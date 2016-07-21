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

#ifndef _WrapImplSoftware_H
#define _WrapImplSoftware_H

#include "wrap.h"
#include "WrapImpl.h"
#include "AliasTableBase.h"

class WrapImplSoftware: public WrapImpl
{
public:
	WrapImplSoftware();
	~WrapImplSoftware();

	int wrapImplOpen();
	int wrapImplClose(int wrapToken);

	void wrapImplWrite(void *ptr, void *src, int size, WRAPTOKEN w);
	size_t wrapImplRead(void *ptr, const void *src, size_t size, WRAPTOKEN w);

	void wrapImplStore64(void *ptr, uint64_t value, WRAPTOKEN w);
	void wrapImplStore32(void *ptr, uint32_t value, WRAPTOKEN w);
	void wrapImplStore16(void *ptr, uint16_t value, WRAPTOKEN w);
	uint64_t wrapImplLoad64(void *ptr, WRAPTOKEN w);
	uint32_t wrapImplLoad32(void *ptr, WRAPTOKEN w);
	uint16_t wrapImplLoad16(void *ptr, WRAPTOKEN w);
	uint8_t wrapImplLoadByte(void *ptr, WRAPTOKEN w);

	void getOptionString(char *c);

	void flush();

	const char *getAliasDetails();

	static void setAliasTableWorkers(int num);

	//  TLS
	static WRAP_TLS int _wrapToken;
	static WRAP_TLS int _numOpen;
	static WRAP_TLS WrapLogger *_log;

private:
	AliasTableBase *_table;
};


#endif
