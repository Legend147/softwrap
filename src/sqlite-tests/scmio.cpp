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

#include "scmio.h"
#include "debug.h"

#ifndef SCM
#include <unistd.h>
#include <fcntl.h>

int scmopen(const char *path, int oflag, int mode)
{
	Debug("scmopen(path=%s, oflag=%d, mode=%d)\n", path, oflag, mode);
	int ret = open(path, oflag, mode);
	Debug("\treturning=%d\n", ret);
	return ret;
}
int scmclose(int fildes)
{
	Debug("scmclose(fildes=%d)\n", fildes);
	int ret = close(fildes);
	Debug("\treturning=%d\n", ret);
	return ret;
}

ssize_t scmread(int fildes, void *buf, size_t nbyte)
{
	Debug("scmread(fildes=%d, buf=%p, nbyte=%d)\n", fildes, buf, nbyte);
	int ret = scmread(fildes, buf, nbyte);
	Debug("\treturning=%d\n", ret);
	return ret;
}

ssize_t scmwrite(int fildes, const void *buf, size_t nbyte)
{
	Debug("scmwrite(fildes=%d, buf=%p, nbyte=%d)\n", fildes, buf, nbyte);
	int ret = scmwrite(fildes, buf, nbyte);
	Debug("\treturning=%d\n", ret);
	return ret;
}

#endif
