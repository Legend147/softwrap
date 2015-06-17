/* -*- mode: C; mode: folding; fill-column: 70; -*- */
/* Copyright 2010,  Georgia Institute of Technology, USA. */
/* See COPYING for license. */
#define _FILE_OFFSET_BITS 64
#define _THLOAD_SAFE
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <assert.h>

#include "../compat.h"
#include "wrap.h"
#include "../graph500.h"

static int64_t maxvtx, maxdeg, nIJ;
static const struct packed_edge * restrict IJ;
static int64_t * restrict head, * restrict deg, * restrict next;

int 
create_graph_from_edgelist (struct packed_edge *IJ_in, int64_t nedge)
{
  int err = 0;

  IJ = IJ_in;
  nIJ = nedge;
  maxvtx = -1;
  maxdeg = -1;

  WRAPOPEN;

  int64_t k;
  for (k = 0; k < nedge; ++k) {
    if (get_v0_from_edge(&IJ[k]) > maxvtx)
      maxvtx = get_v0_from_edge(&IJ[k]);
    if (get_v1_from_edge(&IJ[k]) > maxvtx)
      maxvtx = get_v1_from_edge(&IJ[k]);
  }

  int64_t negone = -1;
  int64_t zero = 0;

  head = pmalloc ((2*(maxvtx+1) + 2*nIJ) * sizeof (int64_t));
  if (!head){  WRAPCLOSE; return -1; }
  deg = &head[maxvtx+1];
  next = &deg[maxvtx+1];

  for (k = 0; k <= maxvtx; ++k) {
    WRAPSTORE64(head[k], negone);
    WRAPSTORE64(deg[k], zero);
  }

  for (k = 0; k < nedge; ++k) {
    const int64_t i = get_v0_from_edge(&IJ[k]);
    const int64_t j = get_v1_from_edge(&IJ[k]);
    int64_t t_head, t;

    if (i >= 0 && j >= 0 && i != j) {
      WRAPSTORE64(next[2*k], negone);
      WRAPSTORE64(next[1+2*k], negone);
      t = 2*k+1; /* Point at the *other* end. */
      t_head = WRAPLOAD64(head[i]);
      WRAPSTORE64(head[i], t);
      assert (t_head < 2*nIJ);
      WRAPSTORE64(next[t], t_head);
      //++deg[i];
      WRAP_PP64(deg[i]);

      --t;
      t_head = WRAPLOAD64(head[j]);
      WRAPSTORE64(head[j], t);
      assert (t_head < 2*nIJ);
      WRAPSTORE64(next[t], t_head);
      //++deg[j];
      WRAP_PP64(deg[j]);
    }
  }

  for (int64_t kg = 0; kg <= maxvtx; ++kg)
    if (WRAPLOAD64(deg[kg]) > maxdeg)
      maxdeg = WRAPLOAD64(deg[kg]);

  WRAPCLOSE;

  return err;
}

int
make_bfs_tree (int64_t *bfs_tree_out, int64_t *max_vtx_out,
	       int64_t srcvtx)
{
	WRAPOPEN;

  int64_t * restrict bfs_tree = bfs_tree_out;
  int err = 0;
  const int64_t nv = maxvtx+1;

  int64_t k, k1, k2, newk2;
  int64_t * restrict vlist;

  *max_vtx_out = maxvtx;

  //bfs_tree[srcvtx] = srcvtx;
  WRAPSTORE64(bfs_tree[srcvtx], srcvtx);
  newk2 = 1;

  vlist = malloc (nv * sizeof (*vlist));
  if (!vlist) return -1;
  //bfs_tree[srcvtx] = srcvtx;
  WRAPSTORE64(bfs_tree[srcvtx], srcvtx);
  k1 = 0; k2 = 1;
  vlist[0] = srcvtx;

  int64_t negone = -1;
  for (k = 0; k < srcvtx; ++k)
  {
    //bfs_tree[k] = -1;
	 WRAPSTORE64(bfs_tree[k], negone);
  }
  for (k = srcvtx+1; k < nv; ++k)
  {
    //bfs_tree[k] = -1;
	  WRAPSTORE64(bfs_tree[k], negone);
  }

  while (k1 != k2)
  {
	  int64_t k, newk2 = k2;
	  for (k = k1; k < k2; ++k)
	  {
		  //printf("k=%ld\n", k);
		  int64_t parent = vlist[k];
		  int64_t p = WRAPLOAD64(head[parent]);
		  while (p >= 0)
		  {
			  int64_t newv = ((p % 2) ? get_v1_from_edge(&IJ[p / 2]) : get_v0_from_edge(&IJ[p / 2]));
			  if (WRAPLOAD64(bfs_tree[newv]) < 0)
			  {
				  WRAPSTORE64(bfs_tree[newv], parent);
				  vlist[newk2++] = newv;
			  }
			  p = WRAPLOAD64(next[p]);
			  //printf("%ld\n", p);
		  }
		  k1 = k2;
		  k2 = newk2;
	  }
  }
  free (vlist);

  WRAPCLOSE;

  return err;
}

void
destroy_graph (void)
{
  pfree (head);
}
