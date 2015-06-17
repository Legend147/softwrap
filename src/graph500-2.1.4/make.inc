# -*- Makefile -*-
# Copyright 2010, Georgia Institute of Technology, USA.
# See COPYING for license.
CPPFLAGS = -g  -std=c99 -I.. -I../..
#CFLAGS = -g -std=c99 -O3 -march=native -fgcse-sm -fgcse-las -fgcse-after-reload -floop-strip-mine -ftree-loop-im -fivopts -funswitch-loops
LDLIBS = -lm -lrt -L.. -lwrap
#CPPFLAGS = -DUSE_MMAP_LARGE -DUSE_MMAP_LARGE_EXT

BUILD_OPENMP = No
BUILD_MPI = No
