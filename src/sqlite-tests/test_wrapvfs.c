/*
 ** 2010 April 7
 **
 ** The author disclaims copyright to this source code.  In place of
 ** a legal notice, here is a blessing:
 **
 **    May you do good and not evil.
 **    May you find forgiveness for yourself and forgive others.
 **    May you share freely, never taking more than you give.
 **
 *************************************************************************
 **
 ** This file implements an example of a simple VFS implementation that
 ** omits complex features often not required or not possible on embedded
 ** platforms.  Code is included to buffer writes to the journal file,
 ** which can be a significant performance improvement on some embedded
 ** platforms.
 **
 ** OVERVIEW
 **
 **   The code in this file implements a minimal SQLite VFS that can be
 **   used on Linux and other posix-like operating systems. The following
 **   system calls are used:
 **
 **    File-system: access(), unlink(), getcwd()
 **    File IO:     open(), read(), write(), fsync(), close(), fstat()
 **    Other:       sleep(), usleep(), time()
 **
 **   The following VFS features are omitted:
 **
 **     1. File locking. The user must ensure that there is at most one
 **        connection to each database when using this VFS. Multiple
 **        connections to a single shared-cache count as a single connection
 **        for the purposes of the previous statement.
 **
 **     2. The loading of dynamic extensions (shared libraries).
 **
 **     3. Temporary files. The user must configure SQLite to use in-memory
 **        temp files when using this VFS. The easiest way to do this is to
 **        compile with:
 **
 **          -DSQLITE_TEMP_STORE=3
 **
 **     4. File truncation. As of version 3.6.24, SQLite may run without
 **        a working xTruncate() call, providing the user does not configure
 **        SQLite to use "journal_mode=truncate", or use both
 **        "journal_mode=persist" and ATTACHed databases.
 **
 **   It is assumed that the system uses UNIX-like path-names. Specifically,
 **   that '/' characters are used to separate path components and that
 **   a path-name is a relative path unless it begins with a '/'. And that
 **   no UTF-8 encoded paths are greater than 512 bytes in length.
 **
 ** JOURNAL WRITE-BUFFERING
 **
 **   To commit a transaction to the database, SQLite first writes rollback
 **   information into the journal file. This usually consists of 4 steps:
 **
 **     1. The rollback information is sequentially written into the journal
 **        file, starting at the start of the file.
 **     2. The journal file is synced to disk.
 **     3. A modification is made to the first few bytes of the journal file.
 **     4. The journal file is synced to disk again.
 **
 **   Most of the data is written in step 1 using a series of calls to the
 **   VFS xWrite() method. The buffers passed to the xWrite() calls are of
 **   various sizes. For example, as of version 3.6.24, when committing a
 **   transaction that modifies 3 pages of a database file that uses 4096
 **   byte pages residing on a media with 512 byte sectors, SQLite makes
 **   eleven calls to the xWrite() method to create the rollback journal,
 **   as follows:
 **
 **             Write offset | Bytes written
 **             ----------------------------
 **                        0            512
 **                      512              4
 **                      516           4096
 **                     4612              4
 **                     4616              4
 **                     4620           4096
 **                     8716              4
 **                     8720              4
 **                     8724           4096
 **                    12820              4
 **             ++++++++++++SYNC+++++++++++
 **                        0             12
 **             ++++++++++++SYNC+++++++++++
 **
 **   On many operating systems, this is an efficient way to write to a file.
 **   However, on some embedded systems that do not cache writes in OS
 **   buffers it is much more efficient to write data in blocks that are
 **   an integer multiple of the sector-size in size and aligned at the
 **   start of a sector.
 **
 **   To work around this, the code in this file allocates a fixed size
 **   buffer of SQLITE_DEMOVFS_BUFFERSZ using sqlite3_malloc() whenever a
 **   journal file is opened. It uses the buffer to coalesce sequential
 **   writes into aligned SQLITE_DEMOVFS_BUFFERSZ blocks. When SQLite
 **   invokes the xSync() method to sync the contents of the file to disk,
 **   all accumulated data is written out, even if it does not constitute
 **   a complete block. This means the actual IO to create the rollback
 **   journal for the example transaction above is this:
 **
 **             Write offset | Bytes written
 **             ----------------------------
 **                        0           8192
 **                     8192           4632
 **             ++++++++++++SYNC+++++++++++
 **                        0             12
 **             ++++++++++++SYNC+++++++++++
 **
 **   Much more efficient if the underlying OS is not caching write
 **   operations.
 */

/**
 * Modified for SoftWrAP.
 */

#include <sqlite3.h>

#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#include "wrap.h"
#include "memtool.h"
#include "debug.h"

/*
** The maximum pathname length supported by this VFS.
*/
#define MAXPATHNAME 512

//  10 MB
#define WRAP_SIZE (10<<20)

#define vfsDebug	if (p->token)	Debug

/*
 ** When using this VFS, the sqlite3_file* handles that SQLite uses are
 ** actually pointers to instances of type WRAPFile.
 */
typedef struct WRAPFile WRAPFile;
struct WRAPFile
{
	sqlite3_file base;              /* Base class. Must be first. */

	char *buffer;                 /* Pointer to malloc'd buffer */
	int bufferSize;                    /* Valid bytes of data in zBuffer */
	int bufferOffset;      /* Offset in file of zBuffer[0] */
	int currentSize;
	WRAPTOKEN token;
};

void size(WRAPFile *p, int sz)
{
	if (p->currentSize < sz)
		p->currentSize = sz;
}

/*
 ** Write directly to the file passed as the first argument. Even if the
 ** file has a write-buffer (WRAPFile.aBuffer), ignore it.
 */
static int wrap_DirectWrite(
		WRAPFile *p,                    /* File handle */
		const void *zBuf,               /* Buffer containing data to write */
		int iAmt,                       /* Size of data to write in bytes */
		sqlite_int64 iOfst              /* File offset to write to */
)
{
	vfsDebug("wrap_DirectWrite %d %d\n", iAmt, iOfst);

	if (p->token)
	{
		wrapWrite(p->buffer + iOfst, (void*)zBuf, iAmt, p->token);
	}
	else
	{
		return SQLITE_OK;
		memcpy(p->buffer + iOfst, (void*)zBuf, iAmt);
	}
	size(p, iAmt + iOfst);

	return SQLITE_OK;
}


/*
 ** Close a file.
 */
static int wrap_Close(sqlite3_file *pFile)
{
	WRAPFile *p = (WRAPFile*)pFile;
	vfsDebug("wrap_Close\n");
	if (p->token)
		wrapClose(p->token);

	//  TODO for now don't free as we want it to persist.
	//pfree(p->buffer);
	return SQLITE_OK;
}

/*
 ** Read data from a file.
 */
static int wrap_Read(
		sqlite3_file *pFile,
		void *zBuf,
		int iAmt,
		sqlite_int64 iOfst
)
{
	WRAPFile *p = (WRAPFile*)pFile;
	if (!p->token)
		return SQLITE_OK;
	vfsDebug("wrap_Read %d %d\n", iAmt, iOfst);

	int sz = iOfst + iAmt;
	int amt = iAmt;
	if (p->currentSize < sz)
	{
		amt = p->currentSize - iOfst;
		if (amt < 0)
		{
			  vfsDebug("\tioerr_Read\n");
			return SQLITE_IOERR_SHORT_READ;
		}

		memcpy(zBuf, (p->token)?wrapRead(p->buffer + iOfst, amt, p->token):(p->buffer + iOfst), amt);
		  vfsDebug("\tioerr_short_Read %d\n", amt);

		return SQLITE_IOERR_SHORT_READ;
	}

	void *src = p->buffer + iOfst;
	vfsDebug("src=%p\n", src);
	src = (p->token)?wrapRead(src, amt, p->token):src;
	vfsDebug("src=%p\n", src);
	memcpy(zBuf, src, iAmt);
	size(p, iAmt + iOfst);
	return SQLITE_OK;
}

/*
 ** Write data to a crash-file.
 */
static int wrap_Write(
		sqlite3_file *pFile,
		const void *zBuf,
		int iAmt,
		sqlite_int64 iOfst
)
{
	WRAPFile *p = (WRAPFile*)pFile;
	return wrap_DirectWrite(p, zBuf, iAmt, iOfst);
}

/*
 ** Truncate a file. This is a no-op for this VFS (see header comments at
 ** the top of the file).
 */
static int wrap_Truncate(sqlite3_file *pFile, sqlite_int64 size)
{
	Debug("wrap_Truncate\n");
	return SQLITE_OK;
}

/*
 ** Sync the contents of the file to the persistent media.
 */
static int wrap_Sync(sqlite3_file *pFile, int flags)
{
	WRAPFile *p = (WRAPFile*)pFile;
	vfsDebug("wrap_Sync\n");
	if (p->token)
	{
		wrapClose(p->token);
		p->token = wrapOpen();
	}
	return SQLITE_OK;
}

/*
 ** Write the size of the file in bytes to *pSize.
 */
static int wrap_FileSize(sqlite3_file *pFile, sqlite_int64 *pSize){
	WRAPFile *p = (WRAPFile*)pFile;
	//int rc;                         /* Return code from fstat() call */
	//struct stat sStat;              /* Output of fstat() call */

	Debug("wrap_FileSize %d\n", p->currentSize);

	*pSize = p->currentSize;
	return SQLITE_OK;
}

/*
 ** Locking functions. The xLock() and xUnlock() methods are both no-ops.
 ** The xCheckReservedLock() always indicates that no other process holds
 ** a reserved lock on the database file. This ensures that if a hot-journal
 ** file is found in the file-system it is rolled back.
 */
static int wrap_Lock(sqlite3_file *pFile, int eLock){
	return SQLITE_OK;
}
static int wrap_Unlock(sqlite3_file *pFile, int eLock){
	return SQLITE_OK;
}
static int wrap_CheckReservedLock(sqlite3_file *pFile, int *pResOut){
	*pResOut = 0;
	return SQLITE_OK;
}

/*
 ** No xFileControl() verbs are implemented by this VFS.
 */
static int wrap_FileControl(sqlite3_file *pFile, int op, void *pArg){
	return SQLITE_OK;
}

/*
 ** The xSectorSize() and xDeviceCharacteristics() methods. These two
 ** may return special values allowing SQLite to optimize file-system
 ** access to some extent. But it is also safe to simply return 0.
 */
static int wrap_SectorSize(sqlite3_file *pFile){
	return 0;
}
static int wrap_DeviceCharacteristics(sqlite3_file *pFile){
	return 0;
}

/*
 ** Open a file handle.
 */
static int wrap_Open(
		sqlite3_vfs *pVfs,              /* VFS */
		const char *zName,              /* File to open, or 0 for a temp file */
		sqlite3_file *pFile,            /* Pointer to WRAPFile struct to populate */
		int flags,                      /* Input SQLITE_OPEN_XXX flags */
		int *pOutFlags                  /* Output SQLITE_OPEN_XXX flags (or NULL) */
){
	static const sqlite3_io_methods wrap_io = {
			1,                            /* iVersion */
			wrap_Close,                    /* xClose */
			wrap_Read,                     /* xRead */
			wrap_Write,                    /* xWrite */
			wrap_Truncate,                 /* xTruncate */
			wrap_Sync,                     /* xSync */
			wrap_FileSize,                 /* xFileSize */
			wrap_Lock,                     /* xLock */
			wrap_Unlock,                   /* xUnlock */
			wrap_CheckReservedLock,        /* xCheckReservedLock */
			wrap_FileControl,              /* xFileControl */
			wrap_SectorSize,               /* xSectorSize */
			wrap_DeviceCharacteristics     /* xDeviceCharacteristics */
	};
	//printf("type=%d\n", getWrapImplType());
	if (getWrapImplType() == 6)
	{
		//setWrapImplOptions(2);
	}

	WRAPFile *p = (WRAPFile*)pFile; /* Populate this structure */
	//printf("wrap_Open %s %d\n", zName, flags);

	if( zName==0 ){
		return SQLITE_IOERR;
	}

	memset(p, 0, sizeof(WRAPFile));

	int sz = WRAP_SIZE;
	if( flags&SQLITE_OPEN_MAIN_JOURNAL )
	{
		Debug("wrap_Open main journal\n");
		sz = WRAP_SIZE >> 2;
		//p->buffer = (char *)malloc(sz);
		p->token = 0;
	}
	else
	{
		//  TODO for now just pmalloc
		p->buffer = (char *)pmalloc(sz);
		p->token = wrapOpen();
	}
	//memset(p->buffer, 0, sz);
	p->bufferSize = sz;
	p->bufferOffset = 0;

	if( pOutFlags ){
		*pOutFlags = flags;
	}
	p->base.pMethods = &wrap_io;
	return SQLITE_OK;
}

/*
 ** Delete the file identified by argument zPath. If the dirSync parameter
 ** is non-zero, then ensure the file-system modification to delete the
 ** file has been synced to disk before returning.
 */
static int wrap_Delete(sqlite3_vfs *pVfs, const char *zPath, int dirSync){
	int rc = 0;                         /* Return code */

	//  TODO delete from pcm.
	Debug("wrap_Delete %s\n", zPath);
	return (rc==0 ? SQLITE_OK : SQLITE_IOERR_DELETE);
}

#ifndef F_OK
# define F_OK 0
#endif
#ifndef R_OK
# define R_OK 4
#endif
#ifndef W_OK
# define W_OK 2
#endif

/*
 ** Query the file-system to see if the named file exists, is readable or
 ** is both readable and writable.
 */
static int wrap_Access(
		sqlite3_vfs *pVfs,
		const char *zPath,
		int flags,
		int *pResOut
)
{
	*pResOut = 0;
	return SQLITE_OK;
}

/*
 ** Argument zPath points to a nul-terminated string containing a file path.
 ** If zPath is an absolute path, then it is copied as is into the output
 ** buffer. Otherwise, if it is a relative path, then the equivalent full
 ** path is written to the output buffer.
 **
 ** This function assumes that paths are UNIX style. Specifically, that:
 **
 **   1. Path components are separated by a '/'. and
 **   2. Full paths begin with a '/' character.
 */
static int wrap_FullPathname(
		sqlite3_vfs *pVfs,              /* VFS */
		const char *zPath,              /* Input path (possibly a relative path) */
		int nPathOut,                   /* Size of output buffer in bytes */
		char *zPathOut                  /* Pointer to output buffer */
){
	strcpy(zPathOut, zPath);
	Debug("wrap_FullPathname %s\n", zPathOut);
	return SQLITE_OK;
}

/*
 ** The following four VFS methods:
 **
 **   xDlOpen
 **   xDlError
 **   xDlSym
 **   xDlClose
 **
 ** are supposed to implement the functionality needed by SQLite to load
 ** extensions compiled as shared objects. This simple VFS does not support
 ** this functionality, so the following functions are no-ops.
 */
static void *wrap_DlOpen(sqlite3_vfs *pVfs, const char *zPath){
	return 0;
}
static void wrap_DlError(sqlite3_vfs *pVfs, int nByte, char *zErrMsg){
	sqlite3_snprintf(nByte, zErrMsg, "Loadable extensions are not supported");
	zErrMsg[nByte-1] = '\0';
}
static void (*wrap_DlSym(sqlite3_vfs *pVfs, void *pH, const char *z))(void){
	return 0;
}
static void wrap_DlClose(sqlite3_vfs *pVfs, void *pHandle){
	return;
}

/*
 ** Parameter zByte points to a buffer nByte bytes in size. Populate this
 ** buffer with pseudo-random data.
 */
static int wrap_Randomness(sqlite3_vfs *pVfs, int nByte, char *zByte){
	return SQLITE_OK;
}

/*
 ** Sleep for at least nMicro microseconds. Return the (approximate) number
 ** of microseconds slept for.
 */
static int wrap_Sleep(sqlite3_vfs *pVfs, int nMicro){
	assert(0);
	sleep(nMicro / 1000000);
	usleep(nMicro % 1000000);
	return nMicro;
}

/*
 ** Set *pTime to the current UTC time expressed as a Julian day. Return
 ** SQLITE_OK if successful, or an error code otherwise.
 **
 **   http://en.wikipedia.org/wiki/Julian_day
 **
 ** This implementation is not very good. The current time is rounded to
 ** an integer number of seconds. Also, assuming time_t is a signed 32-bit
 ** value, it will stop working some time in the year 2038 AD (the so-called
 ** "year 2038" problem that afflicts systems that store time this way).
 */
static int wrap_CurrentTime(sqlite3_vfs *pVfs, double *pTime){
	time_t t = time(0);
	*pTime = t/86400.0 + 2440587.5;
	return SQLITE_OK;
}

/*
 ** This function returns a pointer to the VFS implemented in this file.
 ** To make the VFS available to SQLite:
 **
 **   sqlite3_vfs_register(sqlite3_wrap_vfs(), 0);
 */
sqlite3_vfs *sqlite3_wrap_vfs(void){
	static sqlite3_vfs wrap_vfs = {
			1,                            /* iVersion */
			sizeof(WRAPFile),             /* szOsFile */
			MAXPATHNAME,                  /* mxPathname */
			0,                            /* pNext */
			"wrap_vfs",                       /* zName */
			0,                            /* pAppData */
			wrap_Open,                     /* xOpen */
			wrap_Delete,                   /* xDelete */
			wrap_Access,                   /* xAccess */
			wrap_FullPathname,             /* xFullPathname */
			wrap_DlOpen,                   /* xDlOpen */
			wrap_DlError,                  /* xDlError */
			wrap_DlSym,                    /* xDlSym */
			wrap_DlClose,                  /* xDlClose */
			wrap_Randomness,               /* xRandomness */
			wrap_Sleep,                    /* xSleep */
			wrap_CurrentTime,              /* xCurrentTime */
	};
	return &wrap_vfs;
}
