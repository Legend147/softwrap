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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "WrapImpl.h"
#include "memtool.h"
#include "debug.h"
#include "WrapImplSoftware.h"
#include "AliasTable.h"
#include "AT2.h"
#include "AT3.h"
#include "ATL.h"
#include "LAT.h"

//  TLS
int WRAP_TLS WrapImplSoftware::_wrapToken = 0;
int WRAP_TLS WrapImplSoftware::_numOpen = 0;
typedef WrapLogger * wlp;
wlp WRAP_TLS WrapImplSoftware::_log = NULL;
volatile int WO = 1;


const char *WrapImplSoftware::getAliasDetails()
{
	return _table->getDetails();
}

int WrapImplSoftware::wrapImplOpen()
{
	if (_numOpen == 0)
	{
		_wrapToken = __sync_add_and_fetch(&WO,1); //random() + 1;  //  TODO Atomic increment
		_log = _logManager->getWrapLog(_wrapToken);
		_log->wrapLogOpen(_wrapToken);
		_table->onWrapOpen(_wrapToken);
	}
	_numOpen++;
	return _wrapToken;
}

int WrapImplSoftware::wrapImplClose(int wrapToken)
{
	if ((_wrapToken == 0) || (_numOpen == 0))
	{
		//  We shouldn't be closing if none are open and the token should never be zero.
		assert(0);
	}
	_numOpen--;
	if (_numOpen == 0)
	{
		_log->wrapLogClose();
		//  Add the log to the log manager class.
		_logManager->addLog(_log);
		_table->onWrapClose(_wrapToken, _log);
		_wrapToken = 0;

		//  Cleanup based on options.
		if (m_options == 0)
		{
			//  No cleanup.
		}
		else if (m_options == 1)
		{
			//  Synchronous Log Cleanup
			int n = _logManager->restoreAllLogs();
			//_table->resetTable();
			Debug("Synchronous log cleanup of %d entries.\n", n);
		}
		else if (m_options == 2)
		{
			//  Copy from alias table.
			//int n = _table->processTable();
			//Debug("Alias log cleanup of %d entries.\n", n);
			//  @TODO remove log

			_table->restoreAllElements();
			_logManager->removeAllLogs();
		}
		else if (m_options == 99)
		{
			//_table->resetTable();
		}
		else
		{
			assert(0);
		}
	}

	return 1;
}

void WrapImplSoftware::wrapImplWrite(void *ptr, void *src, int size, WRAPTOKEN w)
{
	if (ptr != src)
	{
		//  Stream to the log.
		_log->addWrapLogEntry(ptr, src, size, w);

		//  Add to the alias table.
		_table->write(ptr, src, size);
	}
}

size_t WrapImplSoftware::wrapImplRead(void *ptr, const void *src, size_t size, WRAPTOKEN w)
{
	if (useReadInto)
		return _table->readInto(ptr, src, size);

	void *v = _table->read((void*)src, size);

	if (v == NULL)
		memcpy(ptr, src, size);
	else
		memcpy(ptr, v, size);
	return size;
}

void WrapImplSoftware::wrapImplStore64(void *ptr, uint64_t value, WRAPTOKEN w)
{
	//  Stream to the log.
	_log->addWrapLogEntry(ptr, &value, 8, w);

	//  Add to the alias table.
	_table->store(ptr, value, 8);
}

void WrapImplSoftware::wrapImplStore32(void *ptr, uint32_t value, WRAPTOKEN w)
{
	//  Stream to the log.
	_log->addWrapLogEntry(ptr, &value, 4, w);

	//  Add to the alias table.
	_table->store(ptr, value, 4);
}

void WrapImplSoftware::wrapImplStore16(void *ptr, uint16_t value, WRAPTOKEN w)
{
	//  Stream to the log.
	_log->addWrapLogEntry(ptr, &value, 2, w);

	//  Add to the alias table.
	_table->store(ptr, value, 2);
}
void WrapImplSoftware::wrapImplStoreByte(void *ptr, uint8_t value, WRAPTOKEN w)
{
	//  Stream to the log.
	_log->addWrapLogEntry(ptr, &value, 1, w);

	//  Add to the alias table.
	_table->store(ptr, value, 1);
}

uint64_t WrapImplSoftware::wrapImplLoad64(void *ptr, WRAPTOKEN w)
{
	if (useReadInto)
	{
		uint64_t data;
		 _table->readInto(&data, ptr, sizeof(data));
		 return data;
	}
	uint64_t *t = (uint64_t *)_table->load(ptr);
	return *t;
}
uint32_t WrapImplSoftware::wrapImplLoad32(void *ptr, WRAPTOKEN w)
{
	if (useReadInto)
	{
		uint32_t data;
		 _table->readInto(&data, ptr, sizeof(data));
		 return data;
	}

	uint64_t *t = (uint64_t *)_table->load(ptr);
	return (uint32_t)*t;
}
uint16_t WrapImplSoftware::wrapImplLoad16(void *ptr, WRAPTOKEN w)
{
	if (useReadInto)
	{
		uint16_t data;
		 _table->readInto(&data, ptr, sizeof(data));
		 return data;
	}
	uint64_t *t = (uint64_t *)_table->load(ptr);
	return (uint16_t)*t;
}
uint8_t WrapImplSoftware::wrapImplLoadByte(void *ptr, WRAPTOKEN w)
{
	if (useReadInto)
	{
		uint8_t data;
		 _table->readInto(&data, ptr, sizeof(data));
		 return data;
	}
	uint64_t *t = (uint64_t *)_table->load(ptr);
	return (uint8_t)*t;
}

WrapImplSoftware::WrapImplSoftware()
{
	useReadInto = 0;
	char *at = getenv("AliasTable");
	if ((at != NULL) && ((strcmp(at, "AT2") == 0) || (strcmp(at, "AT3") == 0) ||
			(strcmp(at, "ATL") == 0) || (strcmp(at, "LAT") == 0)))
	{
		int ATSize = 1<<13;
		char *atopt = getenv("AliasTableSize");
		if ((atopt != NULL) && (atoi(atopt) > 5))
		{
			ATSize = 1<< atoi(atopt);
		}
		int ATThresh = 8;
		atopt = getenv("AliasTableThresh");
		if ((atopt != NULL) && (atoi(atopt) > 1))
		{
			ATThresh = atoi(atopt);
		}

		if (strcmp(at, "AT2") == 0)
			_table = new AT2(_logManager, ATSize, ATThresh);
		else if (strcmp(at, "AT3") == 0)
			_table = new AT3(_logManager, ATSize, ATThresh);
		else if (strcmp(at, "ATL") == 0)
			_table = new ATL(_logManager, ATSize, ATThresh);
		else
		{
			useReadInto = 1;
			_table = new LAT(_logManager, ATSize);
		}
	}
	else
		_table = new AliasTable(_logManager);
}


void WrapImplSoftware::setAliasTableWorkers(int num)
{
	AT2::setNumWorkerThreads(num);
	AT3::setNumWorkerThreads(num);
}

WrapImplSoftware::~WrapImplSoftware()
{
	printf("LazyWrapTime= \t%ld\n", elapsedTime());
	printf("cleaning up wrapimplsoftware\n");
	delete _table;
	Debug("done deleting table.\n");
}

void WrapImplSoftware::flush()
{
	printf("flush\n");
	_table->restoreAllElements();
}

void WrapImplSoftware::getOptionString(char *c)
{
	sprintf(c, "0 (default) - Lazy (no) Log Cleanup\n1 - Synchronous Log Cleanup\n2 - Alias Table Based Cleanup");
}
