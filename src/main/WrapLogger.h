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

#ifndef __WrapLogger_h
#define __WrapLogger_h

#include "wrap.h"

//  The wrap logger class is handled by the wrap log manager.

class WrapLogger
{
public:
	//  Note, a log can grow beyond this size with a pointer to a continuation log, but this is TODO.
	//  Initialize a log with an existing memory location.  Note this location is not freed on exit.
	//  If ptr is null, then the log is created and freed on exit.
	WrapLogger(int size, char *ptr = NULL);
	~WrapLogger();

	void wrapLogOpen(WRAPTOKEN w);
	void wrapLogClose();

	//  Stream an entry to the log from src that belongs in dest of size specified.
	//  We'll just double check the wrap token is equal.
	void addWrapLogEntry(void *ptr, void *src, int size, WRAPTOKEN w);

	int restoreWrapLogEntries();

private:
	//  Restores all entries in a log.  This asserts that the wrap log is complete and returns the number of
	//  entries in the log that were written to main memory.
	static int restoreWrapLogEntries(char *log);

	//  Is log complete, eg, has wrapClose been called on the passed log.
	static bool isWrapLogComplete(char *log);

	//  Pointer to the log data.  This will eventually be handled by a log manager class so it's persistent.
	char *_log;
	//  Flag to free the log on exit if it was created by this class.
	bool m_dofree;
	//  A private status pointer that need not be saved.
	char *_currentLocation;

	//  LOG STRUCTURE
	//  First bytes.
	//  First 4 bytes.
	//  This is in bytes and not the size of a wrap's log, but rather the space available with a 64 bit pointer to
	//  an extra space.  For our tests we'll make sure to allocate enough space in the primary log.
	int _sizeOfLog;
	//  Next 8 bytes, a pointer to the continuation log, NULL for now.
	char *_continuationLog;
	//  Next 4 bytes.
	//  The id of the wrap this log is servicing.  It is updated on wrapOpen.
	int _wrapId;
	//  Next byte.
	//  An entry is valid if the LSB bit, bit 0, of a log entry equals the LSB of this value.
	//  This way we don't have to clear the log but just alternate this bit on the creation of a new wrapOpen.
	//  Each valid entry in the log will have LSB equal to the LSB of this entry.
	//  Bit 1, the next bit, is set if the log is complete and the wrap has been closed on a wrapClose.
	//  Note this is an int to allow for extra flags and also non-temporal stores.
	int _flagEntry;

	//  Log entries
	/*
	 * Layout of the log.
	 * Read the first int in ntohl format.  If bit 0 is equal to bit 0 of _flagEntry then valid.
	 * If bit 1 is zero, then the size is four, so re-read first 8 bytes in ntohll format and it becomes 64 bit pointer
	 *   to the destination location in SCM with bits 0 and 1 set to 0 since they would be anyway if int is aligned,
	 *   which pmalloc aligns to 64 byte boundaries.
	 * If bit 1 is 1, then the int specifies the size after shift >> 2. and the next 8 bytes are the pointer
	 *   to the destination location in SCM.
	 * Next size bytes are the Data.
	 */
};

#endif
