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

#ifndef _WRAP_H
#define _WRAP_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define WRAP	1
#ifndef WRAPNDEBUG
#include <assert.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//  Various wrap implementation types.
typedef enum {MemCheck, NoSCM, NoGuarantee, NoAtomicity, UndoLog, RedoLog, Wrap_Software, Wrap_Hardware, Wrap_Memory } WrapImplType;
void printWrapImplTypes(FILE *stream);

//  Set the wrap's implementation scheme.
void setWrapImpl(WrapImplType wrapimpltype);
//  Get the wrap implementation type.
WrapImplType getWrapImplType();
//  Set and print the implementation options.
void setWrapImplOptions(int options);
void printWrapImplOptions(FILE *stream);

const char *getAliasDetails();

//  Persistent malloc.
void *pmalloc(size_t size);
void *pmallocLog(size_t size);
//  Persistent free.
//void pfree(void *p);
void pfree(void *ptr);

void *pcalloc(size_t nmemb, size_t size);
void *prealloc(void *ptr, size_t size);

//  Mark a created shm segment as persistent.
void persistentSHMCreated(void *v, size_t size);

//  Statistics functions

void startStatistics();
void printStatistics(FILE *stream);
void getAllStatistics(char *c);

/**
 * Persistent Allocate.
 */
//void *pmallocTrace(size_t size, int token);
#define pmallocTrace(size, token) pmalloc(size)
//  Type specific persistent memory allocators.
#define pmallocChar(token)	pmallocTrace(sizeof(char), token)
#define pmallocShort(token)	pmallocTrace(sizeof(short), token)
#define pmallocInt(token)	(int *)pmallocTrace(sizeof(int), token)
#define pmallocLong(token)	pmallocTrace(sizeof(long), token)
#define pmallocLongLong(token)	pmallocTrace(sizeof(long long), token)
#define pmallocFloat(token)	pmallocTrace(sizeof(float), token)
#define pmallocDouble(token)	pmallocTrace(sizeof(double), token)
#define pmallocLongDouble(token) 	pmallocTrace(sizeof(long double), token)


/**
 * Wrap structure.
 */
/*
typedef struct
{
	int state;
	char *token;
	int number;
	int type;
} WRAP;

typedef WRAP* WRAPTOKEN

//  An explicit wrap means that all persistent variables must be read/written with a wrap read or write call.
#define WRAP_EXPLICIT	1
//  An automatic wrap means that any persistent variables read or written will be wrapped by the most current open wrap owned by the thread.
#define WRAP_AUTOMATIC	2
*/

typedef int WRAPTOKEN;


/**
 * Wrap open and close routines.
 */
WRAPTOKEN wrapOpen();
//WRAPTOKEN wrapOpen(const char *token, int type);
int wrapClose(WRAPTOKEN w);

/**
 * New WrAP Streams
 */
WRAPTOKEN wrapStreamOpen();
int wrapStreamClose(WRAPTOKEN w);

void wrapWrite(void *ptr, void *src, int size, WRAPTOKEN w);
//void wrapWriteInt(int *ptr, int i, WRAPTOKEN w);
//#define wrapWriteInt(ptr, val, w)	wrapWrite(ptr, &val, sizeof(int), w)
//void *wrapRead(void *ptr, int size, WRAPTOKEN w);
size_t wrapRead(void *ptr, const void *src, size_t size, WRAPTOKEN w);
//int wrapReadInt(int *ptr, WRAPTOKEN w);
//#define wrapReadInt(ptr, w)	*(int *)wrapRead(ptr, sizeof(int), w)

uint64_t wrapLoad64(void *ptr, WRAPTOKEN w);
uint32_t wrapLoad32(void *ptr, WRAPTOKEN w);
uint16_t wrapLoad16(void *ptr, WRAPTOKEN w);
uint8_t wrapLoadByte(void *ptr, WRAPTOKEN w);
void wrapStore64(void *ptr, uint64_t value, WRAPTOKEN w);
void wrapStore32(void *ptr, uint32_t value, WRAPTOKEN w);
void wrapStore16(void *ptr, uint16_t value, WRAPTOKEN w);
void wrapStoreByte(void *ptr, uint8_t value, WRAPTOKEN w);

#define wrapReadInt(ptr, w)	(int)wrapLoad32(ptr, w)
#define wrapWriteInt(ptr, val, w)	wrapStore32(ptr, (uint32_t)val, w)

//  Cleanup all wraps before exit.
void wrapCleanup();
//  Flush all tables if using aliasing.
void wrapFlush();
//  Set alias table processing threads for WrAP Software.
void setAliasTableWorkers(int num);

/*
#define wrapLoad64(ptr, w)	*(uint64_t *)wrapRead(ptr, 8), w)
#define wrapStore64(ptr, val, w)	wrapWrite(ptr, &val, 8, w)
#define wrapLoad32(ptr, w)	*(uint32_t *)wrapRead(ptr, 4), w)
#define wrapStore32(ptr, val, w)	wrapWrite(ptr, &val, 4, w)
*/

//  Easy defines for quick wrapping
#define WRAPOPEN	WRAPTOKEN tempToken = wrapOpen()
#define WRAPCLOSE	wrapClose(tempToken)
#if 1
#define WRAPLOAD64(x)	(__typeof__ (x))wrapLoad64((void*)(&x), tempToken)
#define WRAPLOAD32(x)	(__typeof__ (x))wrapLoad32((void*)(&x), tempToken)
#define WRAPSTORE64(x, y)	wrapStore64((void*)(&x), y, tempToken)
#define WRAPSTORE32(x, y)	wrapStore32((void*)(&x), y, tempToken)
#elif 0
#define WRAPLOAD64(x)	*(__typeof__ (x) *)wrapRead((void*)(&(x)), 8, tempToken)
#define WRAPLOAD32(x)	*(__typeof__ (x) *)wrapRead((void*)(&(x)), 4, tempToken)
#define WRAPSTORE64(x, y)	wrapWrite((void*)(&(x)), &(y), 8, tempToken)
#define WRAPSTORE32(x, y)	wrapWrite((void*)(&(x)), &(y), 4, tempToken)
#define WRAP_PP64(x)	{__typeof__ (x) tempVar = WRAPLOAD64(x) + 1; WRAPSTORE64(x, tempVar);}
#else
#define WRAPLOAD64(x)	x
#define WRAPLOAD32(x)	x
#define WRAPSTORE64(x, y) x=y
#define WRAPSTORE32(x, y) x=y
#endif

//#define WRAP_PP(t, x)	{t tempVar = WRAPREAD(t, x) + 1; WRAPWRITE(x, tempVar);}
//#define WRAP_MM(t, x)	{t tempVar = WRAPREAD(t, x) - 1; WRAPWRITE(x, tempVar);}
#define WRAP_PP64(x)	WRAPSTORE64(x, WRAPLOAD64(x) + 1)
//#define WRAP_MM64(x)	WRAPSTORE64(x, WRAPLOAD64(x) - 1)

//#define WRAPASSIGN(x, y) 	{ __typeof__ (x) t = (y); WRAPWRITE(x, t); }
//#define WRAPREADWRITE(x, y) { __typeof__ (x) t = (WRAPREAD( __typeof__ (x), y));  WRAPWRITE(x, t); }
/*
#ifdef WRAPNDEBUG
#define WRAPWRITE(x, y)		wrapWrite(&x, &y, sizeof(x), tempToken)
#else
#define WRAPWRITE(x, y)		{assert(sizeof(x) == sizeof(y)); wrapWrite(&x, &y, sizeof(x), tempToken); }
#endif
 */

//  Routines to support basic C types.
//  char, short, int, long, long long, float, double, long double
/*
 * Wrap write routines.
 */
/*
size_t wrapWrite(void *ptr, size_t size, size_t nitems, WRAPTOKEN w);
//  Type specific writers.
#define wrapWriteType(ptr, val, w, type) *((type *)ptr) = val
#define wrapWriteChar(ptr, val, w) wrapWriteType(ptr, val, w, char)
#define wrapWriteShort(ptr, val, w) wrapWriteType(ptr, val, w, short)
#define wrapWriteInt(ptr, val, w) wrapWriteType(ptr, val, w, int)
#define wrapWriteLong(ptr, val, w) wrapWriteType(ptr, val, w, long)
#define wrapWriteLongLong(ptr, val, w) wrapWriteType(ptr, val, w, long long)
#define wrapWriteFloat(ptr, val, w) wrapWriteType(ptr, val, w, float)
#define wrapWriteDouble(ptr, val, w) wrapWriteType(ptr, val, w, double)
#define wrapWriteLongDouble(ptr, val, w) wrapWriteType(ptr, val, w, long double)
*/

/**
 * Wrap read routines.
 */
/*
size_t wrapRead(void *ptr, size_t size, size_t nitems, WRAPTOKEN w);
//  Type specific readers.
#define wrapReadType(ptr, w, type) (type)(*ptr)
#define wrapReadChar(ptr, w) wrapReadType(ptr, w, char)
#define wrapReadShort(ptr, w) wrapReadType(ptr, w, short)
#define wrapReadInt(ptr, w) wrapReadType(ptr, w, int)
#define wrapReadLong(ptr, w) wrapReadType(ptr, w, long)
#define wrapReadLongLong(ptr, w) wrapReadType(ptr, w, long long)
#define wrapReadFloat(ptr, w) wrapReadType(ptr, w, float)
#define wrapReadDouble(ptr, w) wrapReadType(ptr, w, double)
#define wrapReadLongDouble(ptr, w) wrapReadType(ptr, w, long double)
*/

/*
//  Wrap Write Tracers
#define wrapWriteTrace(ptr, size, nitems, w, tok)	wrapWrite(ptr, size, nitems, w); TRACE(tok, ptr, (size*nitems))
#define wrapWriteCharTrace(ptr, val, w, tok) 	wrapWriteChar(ptr, val, w); TRACE(tok, ptr, sizeof(char))
#define wrapWriteShortTrace(ptr, val, w, tok) 	wrapWriteShort(ptr, val, w); TRACE(tok, ptr, sizeof(short))
#define wrapWriteIntTrace(ptr, val, w, tok) 	wrapWriteInt(ptr, val, w); TRACE(tok, ptr, sizeof(int))
#define wrapWriteLongTrace(ptr, val, w, tok) 	wrapWriteLong(ptr, val, w); TRACE(tok, ptr, sizeof(long))
#define wrapWriteLongLongTrace(ptr, val, w, tok) 	wrapWriteLongLong(ptr, val, w); TRACE(tok, ptr, sizeof(long long))
#define wrapWriteFloatTrace(ptr, val, w, tok) 	wrapWriteFloat(ptr, val, w); TRACE(tok, ptr, sizeof(float))
#define wrapWriteDoubleTrace(ptr, val, w, tok) 	wrapWriteDouble(ptr, val, w); TRACE(tok, ptr, sizeof(double))
#define wrapWriteLongDoubleTrace(ptr, val, w, tok) 	wrapWriteLongDouble(ptr, val, w); TRACE(tok, ptr, sizeof(long double))

//  Wrap Read Tracers
#define wrapReadTrace(ptr, size, nitems, w, tok) 	wrapRead(ptr, size, nitems, w); TRACE(tok, ptr, (size * nitems))
#define wrapReadCharTrace(ptr, w, tok) 	wrapReadChar(ptr, w);	TRACE(tok, ptr, sizeof(char))
#define wrapReadShortTrace(ptr, w, tok) 	wrapReadShort(ptr, w);	TRACE(tok, ptr, sizeof(short))
#define wrapReadIntTrace(ptr, w, tok) 	wrapReadInt(ptr, w);	TRACE(tok, ptr, sizeof(int))
#define wrapReadLongTrace(ptr, w, tok) 	wrapReadLong(ptr, w); TRACE(tok, ptr, sizeof(long))
#define wrapReadLongLongTrace(ptr, w, tok) 	wrapReadLongLong(ptr, w);	TRACE(tok, ptr, sizeof(long long))
#define wrapReadFloatTrace(ptr, w, tok) 	wrapReadFloat(ptr, w);	TRACE(tok, ptr, sizeof(float))
#define wrapReadDoubleTrace(ptr, w, tok) 	wrapReadDouble(ptr, w);	TRACE(tok, ptr, sizeof(double))
#define wrapReadLongDoubleTrace(ptr, w, tok) 	wrapReadLongDouble(ptr, w);	TRACE(tok, ptr, sizeof(long double))
*/


/**
 * isSCM returns true if the Wrap Implementation has been set and has been set to something other than NoSCM.
 * This is useful in wrapping legacy applications like a database that has other means of connecting to
 * it so that it can be run in default modes and not wrapped.
 */
//int isSCM();
//  Sets to abort on persistent access outside a wrap.  Used for boundary checking.
//void setAbortOnUnWrapped(int b);

#ifdef __cplusplus
}
#endif

#endif /* _WRAP_H */
