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
 * memtrace.h
 * Author: erg
 */

#ifndef __MEMTRACE_H
#define __MEMTRACE_H

#include <unistd.h>
#include "memtool.h"
#include "stdio.h"

#ifdef MEMTRACE_TOKEN
#define TRACE_OUT(c,t,x,z)		printf("%c \t%d \t%lx \t%lx \t%ld\n", c, t, (unsigned long)x>>PAGEMASKBITS, ((unsigned long)(x))&(PAGESIZE-1), (long)z)
//  Create some space in the page so vars aren't on the same cache line.
#define TRACE_CL_SPACE	{void *v; allocCacheLine(&v);}
//  Initialize a traced variable with token, virtual pointer, and size and flush to memory.
#define TRACE_INIT(t,x,z)	{TRACE_OUT('I',t,x,z); clflush(x);}
//  Place tokens at the start of the program.
#define TRACE_PROGRAM_START  {void *p; allocPage(&p); int i; for (i=0; i<(PAGEMASKBITS-CACHELINEBITS); i++) { (*(long long*)p)=i; TRACE_INIT(0,p,(long)CACHELINESIZE); p+=CACHELINESIZE;}}
#define TRACE_PROGRAM_END	TRACE_PROGRAM_START

#endif

#ifdef MEMTRACE_MAP
// TODO a virtual to physical printing.  define trace_out
//  We don't need cache line space in a mapped system.
#define TRACE_CL_SPACE
#define TRACE_INIT(t,x,z) TRACE_OUT('I',t,x,z)
#define TRACE_PROGRAM_START //  TODO create, find, and print tokens
#define TRACE_PROGRAM_END	//  TODO creat, find and print tokens
#endif

#if defined(MEMTRACE_TOKEN) || defined(MEMTRACE_MAP)

//  The TRACE_WAIT operation will wait for 5 seconds if WAIT_SLEEP is defined otherwise prompt for input on a trace.
#ifdef WAIT_SLEEP
#define TRACE_WAIT	fprintf(stderr, "Sleeping for 5 seconds.....\n"); sleep(5);
#else
#define TRACE_WAIT	fprintf(stderr, "Press [Enter] to start the test....\n"); getchar();
#endif

#ifdef DOTRACE
//  Trace a variable with token and virtual pointer.
#define TRACE(t,x,z)	TRACE_OUT('T',t,x,(long)z)
#else
#define TRACE(t,x,z)
#endif

#ifdef DONOISE
#define TRACE_NOISE(x)	TRACE_OUT('N',-1,x,(long)0)
#else
#define TRACE_NOISE(x)
#endif

#else
#define TRACE_INIT(t,x,z)
#define TRACE_CL_SPACE
#define TRACE_PROGRAM_START
#define TRACE_PROGRAM_END
#define TRACE_WAIT
#define TRACE(t,x,z)
#define TRACE_NOISE(x)

#endif

#endif /* __MEMTRACE_H */
