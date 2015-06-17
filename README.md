#Setup of SoftWrAP, a Multi-Platform Framework for Write Aside Persistence for Storage Class Memories

The SoftWrAP Framework provides a method for making atomic groups of writes to storage class memory.  It can be compiled and built on Unix based platforms including Mac OSX Darwin.  This document has a high-level description of the overall architecture, download, setup, build, demo and API of the SoftWrAP framework.

##Downloading

Clone from github.com

##Building

Unzip the file if downloaded above.  Change to the softwrap directory.

To build the WrAP software library and all of the tests, simply use make:

Prompt> cd softwrap/src
Prompt> make
g++ -g -Wall -Werror -msse2 -o wrap.o -fPIC -c wrap.cpp
g++ -g -Wall -Werror -msse2 -o memtool.o -fPIC -c memtool.c
g++    -c -o tracer.o tracer.cpp
g++ -g -Wall -Werror -msse2 -o WrapImpl.o -fPIC -c WrapImpl.cpp
g++ -g -Wall -Werror -msse2 -o WrapImplUndoLog.o -fPIC -c WrapImplUndoLog.cpp
g++ -g -Wall -Werror -msse2 -o WrapImplSoftware.o -fPIC -c WrapImplSoftware.cpp
g++ -g -Wall -Werror -msse2 -o WrapLogger.o -fPIC -c WrapLogger.cpp
g++ -g -Wall -Werror -msse2 -o WrapLogManager.o -fPIC -c WrapLogManager.cpp
g++ -g -Wall -Werror -msse2 -o AliasTable.o -fPIC -O -c AliasTable.cpp
g++ -g -Wall -Werror -msse2 -o pmem.o -c pmem.cpp
g++ -o libwrap.dylib -g -shared -fPIC wrap.o memtool.o tracer.o WrapImpl.o WrapImplUndoLog.o WrapImplSoftware.o WrapLogger.o WrapLogManager.o AliasTable.o pmem.o -lpthread 
ar -rv libwrap.a wrap.o memtool.o tracer.o WrapImpl.o WrapImplUndoLog.o WrapImplSoftware.o WrapLogger.o WrapLogManager.o AliasTable.o pmem.o
ar: creating archive libwrap.a
a - wrap.o
a - memtool.o
a - tracer.o
a - WrapImpl.o
a - WrapImplUndoLog.o
a - WrapImplSoftware.o
a - WrapLogger.o
a - WrapLogManager.o
a - AliasTable.o
a - pmem.o
g++ -g -Wall -Werror -msse2 -o testsimple testsimple.cpp -L. -lwrap -lpthread
g++ -g -Wall -Werror -msse2 -o testarray testarray.cpp -L. -lwrap
g++ -g -Wall -Werror -msse2 -o testmap testmap.cpp -L. -lwrap
g++ -g -Wall -Werror -msse2 -o testwrapbtree testwrapbtree.cpp -L. -lwrap 
g++ -g -Wall -Werror -msse2 -o bencharray bencharray.cpp -L. -lwrap
g++ -g -Wall -Werror -msse2 -o benchmap benchmap.cpp -L. -lwrap
g++ -g -Wall -Werror -msse2 -o arraytrace arraytrace.cpp -L. -lwrap
g++ -g -Wall -Werror -msse2 -o arrayobject arrayobject.cpp -L. -lwrap
g++ -g -Wall -Werror -msse2 -o arrayvalidate arrayvalidate.cpp -L. -lwrap
g++ -g -Wall -Werror -msse2 -o mtarray mtarray.cpp -L. -lwrap -lpthread
g++ -g -Wall -Werror -msse2 -o simpletest simpletest.cpp -L. -lwrap
g++ -g -Wall -Werror -msse2 -o testwrapvar testwrapvar.cpp -L. -lwrap
g++ -g -Wall -Werror -msse2 -o workerthreads workerthreads.cpp -L. -lwrap -lpthread
 

##Executing The Demo

The array trace program creates an array in memory and then performs a user specified number of updates to the array each in a configurable number of transactions.

Usage: ./arraytrace option wrapSize numWraps [wrapType [options]]

The number of transactions, or groups of elements to be updated, is specified as the number of WrAPs or numWraps, and the number of elements in each group is specified as the wrapSize.  The selection of the element to be updated is specified as the option as one of:
•	0 - cacheline 
•	1 - sequential
•	2 - random
•	3 - block sequential
•	4 - block copy-on-write

In this demo, the transactional method of persistence will be specified in an environment variable, WRAP.  Also, the write time of the SCM will be specified in the SCM_TWR in nanoseconds.  First, we will time the SoftWrAP framework with a SCM write time of 50 nanoseconds.

Alternatively, simply source the file bin/setup.tcsh if using tcsh.
	
Prompt> export WRAP=6 
Prompt> export SCM_TWR=1000
Prompt> ./arraytrace 2 100 100
Int_Size 	4
Page_Size 	4096
Software
Array size=10000 integers with start address: 0x100801000
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
WRAPSTATS 	Type= 	6 	Options= 	0 	totWrapOpen= 	100 	totWrapClose= 	100 	maxWrapDepth= 	1 	totUniqueWraps= 	100 	totWrapRead= 	0 	totWrapWrite= 	10000 	totBytesRead= 	0 	totBytesWrite= 	40000 	maxBytesRead= 	0 	maxBytesWrite= 	4 	ntstore= 41200 p_msync= 100 	writecomb= 0 	Pin not running or overloading the getPinStatistics function.
LazyWrapTime= 	154068
cleaning up wrapimplsoftware
restoring all elements!

totalRestored= 	6307
AliasTableSize= 	6307
totalWrapTime= 	515026


Next, change the transaction method to Copy-On-Write with setting WRAP to 4.  This changes the WrAP implementation of the API to use copy-on-write techniques instead of SoftWrAP.  Other methods and times can also be used in experiments.
	
Prompt> export WRAP=4 
Prompt> ./arraytrace 2 100 100
Int_Size 	4
Page_Size 	4096
UndoLog
…
totalWrapTime= 	1294164

Note, the actual times will differ based on the system and load.  For this computer and on this execution, we can see a speedup of 2.5.

#API

The C API for SoftWrAP is show below.  

SoftWrAP API	Description
void *pmalloc(size_t size);	Allocate persistent memory in bytes.
void pfree(void *ptr);	Free a pointer in persistent memory.
int wrapOpen();	Open a WrAP in the current thread.
int wrapClose();	Close last opened WrAP in current thread.
void wrapWrite(void *ptr, void *src, int size);	Write a pointer src of size bytes to ptr.
void *wrapRead(void *ptr, int size);	Read a value at ptr of size bytes.
uint64_t wrapLoad64(void *ptr);	Load a 64-bit value pointed to by ptr.
void wrapStore64(void *ptr, uint64_t	Save a 64-bit value to location pointed to by ptr.


##Architecture

The SoftWrAP Framework is implemented in C and C++ and builds on Unix based operating systems, including Mac OSX. It is comprised of a software library that can be statically or dynamically linked into software applications along with an Application Program Interface in C/C++, and a tool to automatically identify atomic regions. The figure below shows the overall architecture of the related software components of the SoftWrAP Framework.

 

The SoftWrAP API can model SoftWrAP, and may be configured with an environment variable to also model Copy-On-Write and Non-Transactional memory operations. Copy-On-Write using an undo log is a classic method of providing ACID guarantees in transaction management systems. In this approach, a mechanism is used to create a copy of an object into an undo log before any updates are applied to it. In case the transaction aborts or there is a system failure before all the updated values are committed to durable storage, then the system is rolled back to the instant before the start of the transaction using the original values in the undo log.
To protect against system failures in this approach, after copying a value, care must be taken to make sure that the value is written to the log before a new value is written.  A p_msync is used to make sure that the log value is persisted before the new value is written.

The Non-Transactional configuration is where a system streams values to durable storage, but does not perform any logging and is not failure resistant.  In this method, a system might fail halfway through a set of transactional writes and leave a data structure corrupt.

Some of the writes might have completed and be located on persistent storage, while others, though returned from the storage call, might be caught in a write buffer.  Both SoftWrAP and Copy-On-Write implementations utilize the same log manager and persistent memory management routines such as pmalloc and pfree. The log manager manages allocating space to new logs and saving pointers to logs of completed transactions in a persistent area.

The persistent memory management routines can be configured via environment variables on initialization to either write to DRAM or an SCM model that delays stores to (see Section 5) model SCM, or use memory-mapped files for persistent memory.  Modeling SCM uses DRAM for storage, however, streaming store and persistent memory flush operations are funneled through a store buffer.  The memory-map file is used for correctness checking, testing, and to persist applications between executions.

The SoftWrAP implementation utilizes a DRAM based Alias Table with fine-grained locking.  However, the SoftWrAP Framework allows for the Alias Table to be overridden for application-specific optimized versions.
The table manager is responsible for managing the Alias Table. When the table space reaches a certain configurable threshold, the table manager can flush the Alias
Table and remove the logs. This is accomplished by waiting until all current open WrAPs have completed while not allowing any new WrAPs to open. Once all WrAPs are closed, then the values in the Alias Table are consistent with the logs and can be copied directly to their home SCM location and the logs retired. On a transaction abort, the Alias Table might have inconsistent values with completed logs, as a variable in a completed log might also have been updated in the aborted WrAP.  
To protect against this case, the Alias Table is cleared and all logs processed by reading the log from SCM and copying the variable in the log to its home SCM location.

#Environment Variables

Environment Variable	Description
WRAP	The implementation of the WrAP API to use, see below.
SCM_TWR	The time in nanoseconds for writes to SCM.
SCM	The name of the file to save SCM writes (if not timing).
Additional framework configuration options:
WrapInitLogSize	The initial size of the log file.
Debug	0 or 1 to debug.

Where WRAP is one of:
•	0 – MemCheck (default)
•	1 – NoSCM
•	2 – NoGuarantee (writes are just to cache locations and not flushed)
•	3 – NoAtomicity (writes are flushed but not atomic)
•	4 – Copy-On-Write or UndoLog
•	5 – (unused)
•	6 – SoftWrAP implementation with DRAM Alias Table
•	7, 8 – (unused)

