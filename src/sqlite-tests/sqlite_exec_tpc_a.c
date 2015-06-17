/**
 * Found on http://dislab.hufs.ac.kr/lab/TPC-A_Benchmark/sqlite_exec.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "sqlite3.h"
 
/* Global declarations */
/* tpc bm a scaling rules */
long tps = 1; /* the tps scaling factor: here it is 1 */
long nbranches = 1; /* number of branches in 1 tps db */
long ntellers = 10; /* number of tellers in 1 tps db */
long naccounts = 100000; /* number of accounts in 1 tps db */
long nhistory = 2592000; /* number of history recs in 1 tps db */
 
/* working storage */
long i, j, sqlcode, Bid, Tid, Aid, delta, Abalance;
 
/* functions by shkim */
void ErrorChk(int, char *);
void SetFKConstraint(sqlite3*); /* for sqlite, we should set the constraint of foreign key enforcely as a trigger. */
//void exit_with_error(int, sqlite3*); /* obsolute */
 
/* functions with the demand of TPC-A SPEC */
void CreateDatabase(); /* create and Initializes a scaled database */ 
void DoOne();
 
/* obsolute - 실패 ㅡㅡ;; */
//void exit_with_error(int func, sqlite3* db) {
//		if (func != SQLITE_OK) 
//		{ 
//			fprintf(stderr, "line %d: %s \n: %s\n", __LINE__, func, sqlite3_errmsg(db)); 
//			exit(sqlite3_errcode(db)); 
//		} 
//}
 
 
/*
 * CreateDatabase - Creates and Initializes a scaled database.
 */
void CreateDatabase() {
 
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *tail;
	int rc;
	char *sql, *sql_insert, *sql_update, *sql_delete;
	char *zErr;
 
	int branch_id_ntellers;
	int branch_id_accounts;
 
	rc = sqlite3_open("test.db", &db);
 
	sql = "begin;";
	rc = sqlite3_exec(db, sql, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sql = "CREATE TABLE branches(\
		   Bid NUMERIC(9) PRIMARY KEY,\
		   Bbalance NUMERIC(10),\
		   filler CHAR(88) )";
 
	rc = sqlite3_exec(db, sql, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sql = "CREATE TABLE tellers(\
		   Tid NUMERIC(9) PRIMARY KEY, \
		   Bid NUMERIC(9) CONSTRAINT fk_branches_Bid REFERENCES a(Bid) ON DELETE CASCADE, \
		   Tbalance NUMERIC(10), \
		   filler CHAR(84) )";
 
	rc = sqlite3_exec(db, sql, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sql = "CREATE TABLE accounts(\
		   Aid NUMERIC(9) PRIMARY KEY, \
		   Bid NUMERIC(9) CONSTRAINT fk_branches_Bid REFERENCES a(Bid) ON DELETE CASCADE,\
		   Abalance NUMERIC(10),\
		   filler CHAR(84) )";
 
	rc = sqlite3_exec(db, sql, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sql = "CREATE TABLE history(\
		   Tid NUMERIC(9) CONSTRAINT fk_tellers_Tid  REFERENCES a(Tid) ON DELETE CASCADE,\
		   Bid NUMERIC(9) CONSTRAINT fk_branches_Bid REFERENCES a(Bid) ON DELETE CASCADE,\
		   Aid NUMERIC(9) CONSTRAINT fk_accounts_Aid REFERENCES a(Aid) ON DELETE CASCADE,\
		   delta NUMERIC(10),\
		   time NOT NULL DEFAULT CURRENT_TIMESTAMP,\
		   filler CHAR(22) )";
 
	rc = sqlite3_exec(db, sql, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
//	SetFKConstraint(db); //문제 발생...Foreign Key 지정 실패.. 나중에 수정해야 함.
 
	/* prime database using TPC BM A scaling rules.
	 * Note that for each branch and teller:
	 * branch_id = teller_id / ntellers
	 * branch_id = account_id / naccounts
	 */
 
	sql = "INSERT INTO branches (Bid, Bbalance) VALUES (?, 0)";
	for (i=0; i < nbranches*tps; i++) {
		if(sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
			printf(sqlite3_errmsg(db));
 
		/* '?' 위치에 i 값을 바인딩한다 */
		if(sqlite3_bind_int(stmt, 1, i) != SQLITE_OK)
			fprintf(stderr, "%d, first error of branches\n", i);
 
		/*sql 쿼리 실행 */
		if(sqlite3_step(stmt) != SQLITE_DONE) 	
			printf("%d \n", sqlite3_column_int(stmt, 1)); /* 실행된 결과값을 받아와 출력 */
	}
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
 
	sql = "INSERT INTO tellers (Tid, Bid, Tbalance) VALUES (?, ?, 0)";
	for (i=0; i < (ntellers*tps); i++) {
 
		branch_id_ntellers = i / ntellers;
 
		if(sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
			printf(sqlite3_errmsg(db));
 
		/* 첫번째  '?' 위치에 i 값을 바인딩한다 */
		if(sqlite3_bind_int(stmt, 1, i) != SQLITE_OK)
			fprintf(stderr, "%d, first  error of tellers \n", i);
 
		/* 두번째 '?' 위치에 branch_id_ntellers 값을 바인딩한다 */
		if(sqlite3_bind_int(stmt, 2, branch_id_ntellers) != SQLITE_OK)
			fprintf(stderr, "%d, second error of tellers \n", branch_id_ntellers);
 
		/*sql 쿼리 실행 */
		if(sqlite3_step(stmt) != SQLITE_DONE) /* 실행된 결과값을 받아와 출력 */
			printf("%d : %d : %d\n", sqlite3_column_int(stmt,1), sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 3));
	}
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
 
	sql = "INSERT INTO accounts (Aid, Bid, Abalance) VALUES (?, ?, 0)";
	for (i=0; i < (naccounts*tps); i++) {
 
		branch_id_accounts = i / naccounts;
 
		if(sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
			printf(sqlite3_errmsg(db));
 
		/* '?' 위치에 i 값을 바인딩한다 */
		if(sqlite3_bind_int(stmt, 1, i) != SQLITE_OK)
			fprintf(stderr, "%d, first error of accounts \n", i);
		/* 두번째 '?' 위치에 branch_id_accounts 값을 바인딩한다 */
		if(sqlite3_bind_int(stmt, 2, branch_id_accounts) != SQLITE_OK)
			fprintf(stderr, "%d, second error of accounts \n", branch_id_accounts);
 
		/* sql 쿼리 실행 */
		if(sqlite3_step(stmt) != SQLITE_DONE)/* 실행된 결과값을 받아와 출력 */
			printf("%d : %d : %d\n", sqlite3_column_int(stmt,1), sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 3));
	}
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
 
	sql = "commit;";
	rc = sqlite3_exec(db, sql, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	/* - obsolute
//	for (i=1; i < (nbranches*tps)+1; i++) {
//		sql = "INSERT INTO branches (Bid, Bbalance) VALUES (i, 0);";
//		rc = sqlite3_exec(db, sql, NULL, NULL, &zErr);
//		ErrorChk(rc, zErr);
//	}
//	for (i=0; i < ntellers*tps; i++) {
//		branch_id_ntellers = i / ntellers;
//		sql = "INSERT INTO tellers (Tid, Bid, Tbalance) VALUES (:i, :branch_id_ntellers, 0);";
//		rc = sqlite3_exec(db, sql, NULL, NULL, &zErr);
//		ErrorChk(rc, zErr);
//	}
//	for (i=0; i < naccounts*tps; i++) {
//		branch_id_accounts = i / naccounts;
//		sql = "INSERT INTO accounts (Aid, Bid, Abalance) VALUES (:i, :branch_id_accounts, 0);";
//		rc = sqlite3_exec(db, sql, NULL, NULL, &zErr);
//		ErrorChk(rc, zErr);
!//	}
//	*/
	sqlite3_close(db);
}
 
 
/*
 * DoOne - Executes a single TPC BM A transaction.
 */
void DoOne(int a, int b, int c, int d) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *tail;
	int rc;
	char *sql, *sql_insert, *sql_update, *sql_delete;
	char *zErr;
 
 
	int branch_id_ntellers;
	int branch_id_accounts;
 
	Bid = a;
	Tid = b;
	Aid =c;
	delta = d;
 
	//scanf("%ld %ld %ld %ld", &Bid, &Tid, &Aid, &delta); /* note: must pad to 100 bytes */
 
	rc = sqlite3_open("test.db", &db);
 
	sql ="begin;";
	rc = sqlite3_exec(db, sql, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
 
	fprintf(stderr,  "entering accounts updateing\n");
	sql = "UPDATE accounts SET Abalance = Abalance + ? WHERE Aid = ?";
 
	if(sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
		printf(sqlite3_errmsg(db));
	if(sqlite3_bind_int(stmt, 1, delta) != SQLITE_OK)
		fprintf(stderr, "%d, delta value error\n", delta);
	if(sqlite3_bind_int(stmt, 2, Aid) != SQLITE_OK)
		fprintf(stderr, "%d, Aid value error\n", Aid);
	if(sqlite3_step(stmt) != SQLITE_DONE)
		printf("1st step error! \n");
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
 
	sql = "SELECT Abalance FROM accounts WHERE Aid = ?";
 
	if(sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
		printf(sqlite3_errmsg(db));
	if(sqlite3_bind_int(stmt, 1, Aid) != SQLITE_OK)
		fprintf(stderr, "%d, Aid value error\n", Aid);
	if(sqlite3_step(stmt) != SQLITE_DONE)
		;
		//printf("2nd step error! \n");
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
 
 
	sql = "UPDATE tellers SET Tbalance = Tbalance + ? WHERE Tid = ?";
	if(sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
		printf(sqlite3_errmsg(db));
	if(sqlite3_bind_int(stmt, 1, delta) != SQLITE_OK)
		fprintf(stderr, "%d, delta value error\n", delta);
	if(sqlite3_bind_int(stmt, 2, Tid) != SQLITE_OK)
		fprintf(stderr, "%d, Tid value error\n", Tid);
	if(sqlite3_step(stmt) != SQLITE_DONE)
		printf("3rd step error! \n");
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
 
 
	sql = "UPDATE branches SET Bbalance = Bbalance + ? WHERE Bid = ?";
 
	if(sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
		printf(sqlite3_errmsg(db));
	if(sqlite3_bind_int(stmt, 1, delta) != SQLITE_OK)
		fprintf(stderr, "%d, delta value error\n", delta);
	if(sqlite3_bind_int(stmt, 2, Bid) != SQLITE_OK)
		fprintf(stderr, "%d, Bid value error\n", Bid);
	if(sqlite3_step(stmt) != SQLITE_DONE)
		printf("4rd step error! \n");
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
 
	sql = "INSERT INTO history(Bid, Tid, Aid, delta) VALUES( :1, :2, :3, :4)";
 
//	printf("%ld, %ld, %ld, %ld\n", Bid, Tid, Aid, delta);
 
	if(sqlite3_prepare(db, sql, strlen(sql), &stmt, &tail) != SQLITE_OK) {
		fprintf(stderr, "sqlite3_prepare() : Error: %s\n", tail);
		printf(sqlite3_errmsg(db));
	}
	if(sqlite3_bind_int(stmt, sqlite3_bind_parameter_index(stmt, ":1"), Tid) != SQLITE_OK)
		fprintf(stderr, "%d, Bid value error\n", Bid);
	if(sqlite3_bind_int(stmt, sqlite3_bind_parameter_index(stmt, ":2"), Bid) != SQLITE_OK)
		fprintf(stderr, "%d, Tid value error\n", Tid);
	if(sqlite3_bind_int(stmt, sqlite3_bind_parameter_index(stmt, ":3"), Aid) != SQLITE_OK)
		fprintf(stderr, "%d, Aid value error\n", Aid);
	if(sqlite3_bind_int(stmt, sqlite3_bind_parameter_index(stmt, ":4"), delta) != SQLITE_OK)
		fprintf(stderr, "%d, delta value error\n", delta);
	if(sqlite3_step(stmt) != SQLITE_DONE)
		printf("5th step error! \n");
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
 
	sql = "commit;";
	rc = sqlite3_exec(db, sql, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sqlite3_close(db);
 
	printf("%ld, %ld, %ld, %ld\n", Bid, Tid, Aid, delta);
	/* note: must pad to 200 bytes */
} /* end of DoOne */
 
/* main program,
 *
 * Create a 1-tps database, ie 1 branch, 10 tellers, ...
 * runs one TPC BM A transaction
 */
int main(int argc, char **argv)
{
	int i=0;
	int b=0;
	int t=10;
	CreateDatabase();
 
	printf("input :");
	for(i=1;i <11; i++)
		DoOne(b%i, (t%i), (i-1), 5000);
 
 
	printf("it's done! \n");
	return 0;
}
 
void ErrorChk(int rc, char *zErr) 
{
	if(rc != SQLITE_OK) {
		if(zErr != NULL) {
			fprintf(stderr, "SQL error: %s\n", zErr);
			sqlite3_free(zErr);
		}
	}
}
 
//////////////////////////////////////////////////////
/* FOREIGN KEY 트리거 설정 - Insert, Update, Delete */
//////////////////////////////////////////////////////
void SetFKConstraint(sqlite3* db) {
	char *zErr;
	char *sql_insert, *sql_update, *sql_delete;
	int rc;
 
	rc = sqlite3_open("test.db", &db);
 
	/* -- branches and tellers -- */
	sql_insert = "CREATE TRIGGER fki_tellers_branches_Bid \
		   BEFORE INSERT ON tellers \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Bid FROM branches WHERE Bid = NEW.Bid) IS NULL) \
				THEN RAISE(ABORT, ' insert on table \"tellers\" violates foreign key || constraint \"fk_branches_Bid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_insert, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sql_update = "CREATE TRIGGER fku_tellers_branches_Bid \
		   BEFORE UPDATE ON tellers \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Bid FROM branches WHERE Bid = NEW.Bid) IS NULL) \
				THEN RAISE(ABORT, ' update on table \"tellers\" violates foreign key || constraint \"fk_branches_Bid\"' ) \
			END; \
		   END;";
 
 
	rc = sqlite3_exec(db, sql_update, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	/* delete 는 primary key에 대해서만 적용.. cascade delete를 막아놨음... */
	sql_delete = "CREATE TRIGGER fkd_tellers_branches_Bid \
		   BEFORE DELETE ON branches \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Bid FROM tellers WHERE Bid = OLD.Bid) IS NULL) \
				THEN RAISE(ABORT, ' delete on table \"tellers\" violates foreign key || constraint \"fk_branches_Bid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_delete, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
/* -- branches and accounts -- */
	sql_insert = "CREATE TRIGGER fki_accounts_branches_Bid \
		   BEFORE INSERT ON accounts \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Bid FROM branches WHERE Bid = NEW.Bid) IS NULL) \
				THEN RAISE(ABORT, ' insert on table \"accounts\" violates foreign key || constraint \"fk_branches_Bid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_insert, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sql_update = "CREATE TRIGGER fku_accounts_branches_Bid \
		   BEFORE UPDATE ON accounts \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Bid FROM branches WHERE Bid = NEW.Bid) IS NULL) \
				THEN RAISE(ABORT, ' update on table \"accounts\" violates foreign key || constraint \"fk_branches_Bid\"' ) \
			END; \
		   END;";
 
 
	rc = sqlite3_exec(db, sql_update, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	/* delete 는 primary key에 대해서만 적용.. cascade delete를 막아놨음... */
	sql_delete = "CREATE TRIGGER fkd_accounts_branches_Bid \
		   BEFORE DELETE ON branches \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Bid FROM accounts WHERE Bid = OLD.Bid) IS NULL) \
				THEN RAISE(ABORT, ' delete on table \"accounts\" violates foreign key || constraint \"fk_branches_Bid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_delete, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
/* -- branches , accounts, tellers and history -- */
 
	/* for insert */
	sql_insert = "CREATE TRIGGER fki_history_branches_Bid \
		   BEFORE INSERT ON history \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Bid FROM branches WHERE Bid = NEW.Bid) IS NULL) \
				THEN RAISE(ABORT, ' insert on table \"accounts\" violates foreign key || constraint \"fk_branches_Bid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_insert, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sql_insert = "CREATE TRIGGER fki_history_tellers_Tid \
		   BEFORE INSERT ON history \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Tid FROM tellers WHERE Tid = NEW.Tid) IS NULL) \
				THEN RAISE(ABORT, ' insert on table \"tellers\" violates foreign key || constraint \"fk_tellers_Tid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_insert, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sql_insert = "CREATE TRIGGER fki_history_accounts_Aid \
		   BEFORE INSERT ON accounts \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Aid FROM accounts WHERE Aid = NEW.Aid) IS NULL) \
				THEN RAISE(ABORT, ' insert on table \"accounts\" violates foreign key || constraint \"fk_accounts_Aid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_insert, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);	
 
	/* for update */
	sql_insert = "CREATE TRIGGER fku_history_branches_Bid \
		   BEFORE UPDATE ON history \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Bid FROM branches WHERE Bid = NEW.Bid) IS NULL) \
				THEN RAISE(ABORT, ' update on table \"accounts\" violates foreign key || constraint \"fk_branches_Bid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_insert, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sql_insert = "CREATE TRIGGER fku_history_tellers_Tid \
		   BEFORE UPDATE ON history \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Tid FROM tellers WHERE Tid = NEW.Tid) IS NULL) \
				THEN RAISE(ABORT, ' update on table \"accounts\" violates foreign key || constraint \"fk_tellers_Tid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_insert, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sql_insert = "CREATE TRIGGER fku_history_accounts_Aid \
		   BEFORE UPDATE ON accounts \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Aid FROM accounts WHERE Aid = NEW.Aid) IS NULL) \
				THEN RAISE(ABORT, ' update on table \"accounts\" violates foreign key || constraint \"fk_accounts_Aid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_insert, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);	
 
	/* for delete */
	/* delete 는 primary key에 대해서만 적용.. cascade delete를 막아놨음... */
	sql_delete = "CREATE TRIGGER fkd_history_branches_Bid \
		   BEFORE DELETE ON branches \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Bid FROM branches WHERE Bid = OLD.Bid) IS NULL) \
				THEN RAISE(ABORT, ' delete on table \"branches\" violates foreign key || constraint \"fk_branches_Bid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_delete, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sql_delete = "CREATE TRIGGER fkd_history_tellers_Tid \
		   BEFORE DELETE ON tellers \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Tid FROM tellers WHERE Tid = OLD.Tid) IS NULL) \
				THEN RAISE(ABORT, ' delete on table \"tellers\" violates foreign key || constraint \"fk_tellers_Tid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_delete, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sql_delete = "CREATE TRIGGER fkd_history_accounts_Aid \
		   BEFORE DELETE ON accounts \
		   FOR EACH ROW BEGIN \
		   	SELECT CASE \
				WHEN ((SELECT Aid FROM accounts WHERE Aid = OLD.Aid) IS NULL) \
				THEN RAISE(ABORT, ' delete on table \"accounts\" violates foreign key || constraint \"fk_accounts_Aid\"' ) \
			END; \
		   END;";
 
	rc = sqlite3_exec(db, sql_delete, NULL, NULL, &zErr);
	ErrorChk(rc, zErr);
 
	sqlite3_close(db);
}
