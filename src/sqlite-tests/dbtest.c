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
#include <stdlib.h>
#include <sqlite3.h> 
#include <string.h>
#include <assert.h>
#include "wrap.h"
#include "debug.h"

extern sqlite3_vfs *sqlite3_demovfs();
extern sqlite3_vfs *sqlite3_scmvfs();
extern sqlite3_vfs *sqlite3_wrap_vfs();
extern void tic();
extern long toc();


static int callback(void *data, int argc, char **argv, char **azColName){
	int i;

	if (!isDebug())
		return 0;

	Debug("%s: \n", (const char*)data);
	for(i=0; i<argc; i++){
		Debug("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	Debug("\n");
	return 0;
}

void executeSqlStmt(sqlite3 *db, const char *stmt, int nstmts)
{
	char *zErrMsg = 0;
	int rc;
	const char* data = "Callback function called";

	rc = sqlite3_exec(db, stmt, callback, (void*)data, &zErrMsg);
	if( rc != SQLITE_OK ){
		fprintf(stderr, "Statement %d SQL error: %s\n", nstmts, zErrMsg);
		sqlite3_free(zErrMsg);
	}else{
		if (nstmts < 0)
			fprintf(stdout, "Operation done successfully\n");
	}
}

long executeSqlFile(sqlite3 *db, char *filename)
{
	char * stmt = NULL;
	size_t len = 0;
	ssize_t read;
	int ntrans = 0;
	int currstmts = 0;
	int maxstmts = 0;
	int nstmts = 0;

	FILE *fp = fopen(filename, "r");
	assert(fp != NULL);

	tic();
	while ((read = getline(&stmt, &len, fp)) != -1)
	{
		//printf("Retrieved line of length %zu :\n", read);
		//printf("%s", line);
		/* Execute SQL statement */
		if ((stmt == NULL) || (strlen(stmt) == 0))
			continue;
		if (strncmp(stmt, "BEGIN", 5) == 0)
		{
		}
		if (strncmp(stmt, "COMMIT", 5) == 0)
		{
			ntrans++;
			if (currstmts > maxstmts)
				maxstmts = currstmts;
			currstmts = 0;
		}
		nstmts++;
		currstmts++;
		executeSqlStmt(db, stmt, nstmts);
	}
	long t = toc();
	float f = ntrans * 1000000;
	f /= t;
	printf("%s %d statements, %d transactions, max size %d in %ld microseconds is %f trans/sec\n",
			filename, nstmts, ntrans, maxstmts, t, f );
	free(stmt);
	fclose(fp);
	return ntrans;
}


int main(int argc, char* argv[])
{
	sqlite3 *db;
	char vfs[256];

	if (strcmp(argv[1], "scm") == 0)
	{
		strcpy(vfs, "scm");
		sqlite3_vfs_register(sqlite3_scmvfs(), 1);
	}
	else if (strcmp(argv[1], "wrap") == 0)
	{
		strcpy(vfs, "wrap_vfs");
		sqlite3_vfs_register(sqlite3_wrap_vfs(), 1);
	}
	else
	{
		strcpy(vfs, "demo");
		sqlite3_vfs_register(sqlite3_demovfs(), 1);
	}

	startStatistics();

	/* Open database */
	int rc = sqlite3_open_v2(argv[1], &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, vfs);
	if( rc ){
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		exit(0);
	}else{
		fprintf(stdout, "Opened database successfully\n");
	}

	if (strcmp(argv[1], "wrap") == 0)
		executeSqlStmt(db, "PRAGMA journal_mode = OFF;\n", -1);

	executeSqlFile(db, argv[2]);

	executeSqlFile(db, argv[3]);

	executeSqlFile(db, argv[4]);
	printStatistics(stdout);

	sqlite3_close(db);
	return 0;
}
