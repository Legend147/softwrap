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

#include <assert.h>
#include <string.h>
#include "AliasTableSingle.h"
#include "memtool.h"
#include "debug.h"

AliasTableSingle::AliasTableSingle(int size)
{
	_data = (char *)calloc(size, 1);
	_hash = (TableEntry**)calloc(MapSize, sizeof(TableEntry*));
	_dataSize = size;
	resetTable();
};


AliasTableSingle::~AliasTableSingle()
{
	free(_data);
};

void AliasTableSingle::resetTable()
{
	_dataOffset = 0;
	//bzero(_data, _dataSize);
	size_t t = MapSize;
	t *= sizeof(TableEntry*);
	//printf("%lu hash=%p\n", t, _hash);
	bzero(_hash, t);
	//bzero(BloomField, BloomSize * sizeof(unsigned long));
};


int AliasTableSingle::processTable()
{
	int num = 0;
	int offset = 0;
	while (offset < _dataOffset)
	{
		Debug("Processing offset=%d\n", offset);
		TableEntry *te = (TableEntry *)(_data + offset);
		//assert(te != NULL);
		//assert(te->destinationPtr != NULL);
		//assert(te->size != 0);
		ntstore(te->destinationPtr, te + sizeof(TableEntry), te->size);
		_hash[getHash(te->destinationPtr)] = 0;
		offset += (sizeof(TableEntry) + te->size);
		num++;
	}
	p_msync();
	_dataOffset = 0;
	return num;
};
