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
#include "AliasTableMap.h"
#include "memtool.h"

AliasTableMap::AliasTableMap(int size)
{
	_data = (char *)malloc(size);
	_dataSize = size;
	_dataOffset = 0;
};


AliasTableMap::~AliasTableMap()
{
	free(_data);
};

void AliasTableMap::resetTable()
{
	_dataOffset = 0;
	_map.clear();
};

char *AliasTableMap::getAlias(void *ptr)
{
	AliasMap::iterator it = _map.find(ptr);
	if (it == _map.end())
		return NULL;
	return (char*)(it->second);
};

void AliasTableMap::onWrite(void *ptr, void *src, int size, WRAPTOKEN w)
{
	char *alias = (char *)getAlias(ptr);
	if (alias == NULL)
	{
		alias = _data + _dataOffset;
		_dataOffset += size + sizeof(int);
		assert(_dataOffset < _dataSize);
		_map[ptr] = alias;
	}
	memcpy(alias, &size, sizeof(int));
	memcpy(alias+sizeof(int), src, size);
};

void *AliasTableMap::onRead(void *ptr, int size, WRAPTOKEN w)
{
	char *alias = getAlias(ptr);
	if (alias == NULL)
		return ptr;
	return alias+sizeof(int);
};

int AliasTableMap::processTable()
{
	int num = 0;
	int size;
	for (AliasMap::iterator it=_map.begin(); it!=_map.end(); ++it)
	{
		//std::cout << it->first << " => " << it->second << '\n';
		size = *(int*)(it->second);
		ntstore(it->first, ((char*)(it->second))+sizeof(int), size);
		num++;
	}
	p_msync();
	resetTable();
	return num;
};
