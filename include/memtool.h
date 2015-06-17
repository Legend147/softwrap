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

/*
 * memtool.h
 *
 *  Author: ergiles
 */

#ifndef __MEMTOOL_H
#define __MEMTOOL_H

#include <stdlib.h>
#include <stdint.h>

//#define WRAP_TLS
#define WRAP_TLS __thread

#define dmax(x, y)	((x>y)?x:y)

//  Pin a thread to a core.
int attachThreadToCore(int core_id);
#define AttachPrimaryCore attachThreadToCore(9)
#define AttachRetireCore  attachThreadToCore(11)

void doMicroSecDelay(int delay);
void setSCMtwrFactor(int factor);

void clflush(volatile void *p);

uint64_t rdtsc();

//void wtstore(void *dest, void *src, int size);

void ntstore(void *dest, void *src, int size);

int getNumNtStores();
int getNumPMSyncs();
int getNumWriteComb();
long getNsTime();

//void ntObjectStore(void *dest, void *src, int size);

void cacheflush();

void p_msync();

void msync();

void *mallocAligned(size_t size);

void printMemoryAttributes(void);

int getCacheLineSize();


//  Page size is a system page size and may not be the same as the underlying hardware page size.
//  The unix command pagesize will return the size of a page in bytes.
//  The getpagesize(void) function will return the number of bytes in a page.
#ifndef PAGESIZE
#define PAGESIZE	4096
#endif
#ifndef PAGEMASKBITS
#define PAGEMASKBITS	12
#endif

#ifndef NTRACES
#define NTRACES		16
#endif

#ifndef CACHELINESIZE
#define CACHELINESIZE 64
#endif
#ifndef CACHELINEBITS
#define CACHELINEBITS	6
#endif

#define allocPage(p) 	posix_memalign(p, PAGESIZE, PAGESIZE)
#define allocCacheLine(line)	posix_memalign(line, CACHELINESIZE, CACHELINESIZE)




#endif /* __MEMTOOL_H */
