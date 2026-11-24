c---------------------------------------------------------------------
c  Receive buffer for CG's pairwise SHMEM exchanges (see exch_put in
c  cg.f). COMMON, so it is symmetric. Two slots, each large enough
c  for a PE's whole row block (at most na/num_proc_rows+1 elements),
c  which also covers the 2-element butterfly sums. Include after
c  npbparams.h.
c---------------------------------------------------------------------

      integer            exch_buf_len
      parameter        ( exch_buf_len = na/num_proc_rows+2 )

      double precision   exch_buf(exch_buf_len, 0:1)
      integer            exch_slot
      common /exchbuf/   exch_buf, exch_slot
