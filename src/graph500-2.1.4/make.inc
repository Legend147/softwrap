# -*- Makefile -*-
# Copyright 2010, Georgia Institute of Technology, USA.
# See COPYING for license.
CPPFLAGS = -g  -std=c99 -I.. -I../..  -I$(WRAP_INC_PATH)
#CFLAGS = -g -std=c99 -O3 -march=native -fgcse-sm -fgcse-las -fgcse-after-reload -floop-strip-mine -ftree-loop-im -fivopts -funswitch-loops
LDLIBS = $(WRAP_LIB) -lm -lrt -L..
#CPPFLAGS = -DUSE_MMAP_LARGE -DUSE_MMAP_LARGE_EXT

BUILD_OPENMP = No
BUILD_MPI = No
