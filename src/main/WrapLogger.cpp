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
#include "WrapLogger.h"
#include "memtool.h"
#include "debug.h"

WrapLogger::~WrapLogger()
{
	if (!m_dofree)
		return;

	//printf("F");
	Debug("Freeing log!!!\n");
	pfree(_log);
};

//  TODO make it work for undo log restore on failure too.
//  Note, a log can grow beyond this size with a pointer to a continuation log, but this is TODO.
//  Initialize a log with an existing memory location.  Note this location is not freed on exit.
//  If ptr is null, then the log is created and freed on exit.
WrapLogger::WrapLogger(int size, char *ptr) : _log(ptr)
{
	m_dofree = false;
	if (_log == NULL)
	{
		_log = (char *)pmallocLog(size);
		m_dofree = true;
	}
	assert(_log != NULL);
	//printf("malloc log %p of size %d.\n", _log, size);
	_currentLocation = _log;

	//  Initialize the log in SCM.
	//  Size.
	_sizeOfLog = size;
	ntstore(_currentLocation, &_sizeOfLog, 4);
	_currentLocation += 4;

	//  Continuation pointer.
	_continuationLog = NULL;
	ntstore(_currentLocation, &_continuationLog, 8);
	_currentLocation += 8;

	_wrapId = 0;
	ntstore(_currentLocation, &_wrapId, 4);
	_currentLocation += 4;

	_flagEntry = 0;
	ntstore(_currentLocation, &_flagEntry, 4);
	_currentLocation += 4;

	//  TODO make sure not necessary!!!
	//p_msync();
}

void WrapLogger::wrapLogOpen(WRAPTOKEN w)
{
	//  Mark as invalid, bit 1, and set bit 0 to inverse of last log.
	if (_flagEntry & 1)
		_flagEntry = 0;
	else
		_flagEntry = 1;
	ntstore(_log + 16, &_flagEntry, 4);

	//  Save the wrap id.
	_wrapId = w;
	ntstore(_log + 12, &_wrapId, 4);

	//  Go to the first log location entry.
	_currentLocation = _log + 20;

	//p_msync();
}

void WrapLogger::wrapLogClose()
{
	unsigned long l = 0;
	ntstore(_currentLocation, &l, 8);
	ntstore(_currentLocation+8, &l, 8);

	//  Make sure other entries clear.
	//p_msync();

	//  Set bit 1 to mark log as valid and wrap complete.
	_flagEntry |= 2;
	ntstore(_log + 16, &_flagEntry, 4);

	p_msync();
}

/*
uint64_t htonll(uint64_t value)
{
	int num = 42;
	if (*(char *)&num == 42)
	{
		uint32_t high_part = htonl((uint32_t)(value >> 32));
		uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));
		return (((uint64_t)low_part) << 32) | high_part;
	} else {
		return value;
	}
}
*/

//  Stream an entry to the log from src that belongs in dest of size specified.
//  We'll just double check the wrap token is equal.
void WrapLogger::addWrapLogEntry(void *ptr, void *src, int size, WRAPTOKEN w)
{
	if (w != _wrapId)
		printf("w=%d _wrapId=%d\n", w, _wrapId);
	assert(w == _wrapId);
	if (_flagEntry >= 2)
		printf("w=%d flag=%d\n", w, _flagEntry);
	assert(_flagEntry < 2);  // flag should be 0 or 1 since it's open.
	if (!(_currentLocation + 12 + size < _log + _sizeOfLog))
	{
		printf("currentLocation=%p + 12 + size=%d < _log=%p + _sizeOfLog=%d\n",
				_currentLocation, size, _log, _sizeOfLog);
		assert(0);
	}

	//  TODO VERIFY!!
	if (size == 4)
	{
		uint64_t u = (uint64_t)ptr;
		//  Make sure the last bits of the pointer are zero, eg it's 4 byte aligned.
		//assert((u & 3) == 0);
		u += (_flagEntry & 1);
		ntstore(_currentLocation, (void *)u, 8);
		_currentLocation += 8;
	}
	else
	{
		int mask = size << 2;
		mask += 2;
		mask += (_flagEntry & 1);
		ntstore(_currentLocation, &mask, 4);
		_currentLocation += 4;

		Debug("size=%d dest=%p data=%p\n", size, ptr, src);
		ntstore(_currentLocation, &ptr, 8);
		_currentLocation += 8;
	}
	ntstore(_currentLocation, src, size);
	_currentLocation += size;
}

//  Restores all entries in the log.  This asserts that the wrap log is complete and returns the number of
//  entries in the log that were written to main memory.
int WrapLogger::restoreWrapLogEntries()
{
	//Debug("restoreWrapLogEntries %p\n", _log);
	return restoreWrapLogEntries(_log);
}

//  Restores all entries in the log.  This asserts that the wrap log is complete and returns the number of
//  entries in the log that were written to main memory.
int WrapLogger::restoreWrapLogEntries(char *log)
{
	//  Make sure the wrap log is complete.
	assert(isWrapLogComplete(log));
	//  TODO check the continuation pointer and restore it as well, treat it as another WrapLogger maybe.

	//  Get the current valid flag.
	int flag = *((int *)(log + 16));
	flag &= 1;

	//  Start the log offset.
	char *current = log + 20;
	int num = 0;
	while (1)
	{
		int i = *((int *)current);
		if ((i & 1) == flag)
		{
			//  Valid entry.
			void *dest;
			void *data;
			int size;
			/*
			//  ==1?????
			if ((i & 2) == 0)
			{
				// Size is 4.
				uint64_t u = *(uint64_t *)current;
				if (u == 0)
					break;
				//  Mask the pointer since we don't need the lower two bits.
				uint64_t pintmask = ~((uint64_t)3);
				u &= pintmask;
				//  Destination, size, and data.
				dest = (void *)u;
				size = 4;
				data = current + 8;
				current += 8 + size;
			}
			else
			*/
			{
				size = i >> 2;
				current += 4;
				dest = *(void**)current;
				if (dest == 0)
					break;
				data = current + 8;
				current += 8 + size;
				Debug("size=%d dest=%p data=%p\n", size, dest, data);
			}
			ntstore(dest, data, size);

			num++;
		}
		else
		{
			break;
		}
	}
	//  Synchronize on all the data copies.
	p_msync();

	//  Invalidate the log.
	//  Mark as invalid but keep the bit as last valid flag since it's inverted on wrapOpen.
	flag &= 1;
	ntstore(log + 16, &flag, 4);

	//  Save the wrap id.
	int wrapid = 0;
	ntstore(log + 12, &wrapid, 4);

	//  Synchronize on the log status.
	p_msync();
	return num;
}


//  Is log complete, eg, has wrapClose been called on this log.
bool WrapLogger::isWrapLogComplete(char *log)
{
	//  Check the log pointer.
	int i = *((int *)(log + 16));
	if ((i & 2) == 2)
	{
		return true;
	}
	return false;
}
