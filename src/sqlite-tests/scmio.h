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

	 fcntl
	int open(const char *path, int oflag, ...);
	unistd
	int close(int fildes);
	ssize_t pread(int d, void *buf, size_t nbyte, off_t offset);
	ssize_t read(int fildes, void *buf, size_t nbyte);
	ssize_t readv(int d, const struct iovec *iov, int iovcnt);
	ssize_t pwrite(int fildes, const void *buf, size_t nbyte, off_t offset);
	ssize_t write(int fildes, const void *buf, size_t nbyte);
	ssize_t writev(int fildes, const struct iovec *iov, int iovcnt);

	unistd
	off_t lseek(int fildes, off_t offset, int whence);
	DESCRIPTION
	The lseek() function repositions the offset of the file descriptor fildes to the
	argument offset, according to the directive whence.  The argument fildes must be an
	open file descriptor.  Lseek() repositions the file pointer fildes as follows:

		  If whence is SEEK_SET, the offset is set to offset bytes.

		  If whence is SEEK_CUR, the offset is set to its current location plus offset
		  bytes.

		  If whence is SEEK_END, the offset is set to the size of the file plus offset
		  bytes.

	*/
#include <sys/types.h>

int scmopen(const char *path, int oflag, int mode);
int scmclose(int fildes);
ssize_t scmread(int fildes, void *buf, size_t nbyte);
ssize_t scmwrite(int fildes, const void *buf, size_t nbyte);
