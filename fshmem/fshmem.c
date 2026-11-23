/*
 * Fortran SHMEM bindings for the OpenSHMEM NAS Parallel Benchmarks, on top
 * of OSSS OpenSHMEM, which ships C/C++ bindings only.
 *
 * Covers exactly the routines the Fortran benchmarks call (EP, MG, BT, SP),
 * with gfortran's calling convention: lowercase names with a trailing
 * underscore, every argument by reference, default INTEGER is a C int.
 *
 * The caller's pSync arrays are ignored. Each collective instead gets one from
 * a rotating pool of C 'long' arrays on the symmetric heap. This works around
 * pSync bugs in the benchmarks (arrays sized for a broadcast but passed to a
 * reduction, the same array reused by back-to-back collectives with no
 * barrier in between) and the 4-byte INTEGER vs 8-byte long mismatch. It
 * relies on every collective here running over the same active set on every
 * PE, in the same order, which holds for these benchmarks.
 *
 * The C start_pes() is overridden too: OSSS's calls only its internal
 * shmemc_init(), skipping what shmem_init() sets up on top (the collectives
 * table, so the first barrier calls a NULL pointer; progress threads; the
 * atexit finalize). IS calls it, and linking this library ahead of libshmem
 * makes the definition below win.
 *
 * MPI_Wtime is also provided: the benchmarks only use it as a timer, and
 * mpif.h in this directory gives them the few MPI constants they reference,
 * so they don't need an MPI library at all.
 */

#include <shmem.h>
#include <stdlib.h>
#include <time.h>

/*
 * A PE can run ahead of a slower one by up to the number of collectives that
 * don't synchronise all PEs (a broadcast root doesn't wait). The benchmarks
 * issue at most three such calls in a row, so this is generous.
 */
#define FSHMEM_PSYNC_POOL 16

static long *bcast_psync;  /* FSHMEM_PSYNC_POOL x SHMEM_BCAST_SYNC_SIZE */
static long *reduce_psync; /* FSHMEM_PSYNC_POOL x SHMEM_REDUCE_SYNC_SIZE */
static unsigned bcast_next;
static unsigned reduce_next;

static long *fshmem_psync_alloc(size_t count) {
  long *psync = shmem_malloc(FSHMEM_PSYNC_POOL * count * sizeof(*psync));
  size_t i;

  if (psync == NULL) {
    abort();
  }

  for (i = 0; i < FSHMEM_PSYNC_POOL * count; ++i) {
    psync[i] = SHMEM_SYNC_VALUE;
  }
  return psync;
}

static long *fshmem_bcast_psync(void) {
  return bcast_psync +
         (bcast_next++ % FSHMEM_PSYNC_POOL) * SHMEM_BCAST_SYNC_SIZE;
}

static long *fshmem_reduce_psync(void) {
  return reduce_psync +
         (reduce_next++ % FSHMEM_PSYNC_POOL) * SHMEM_REDUCE_SYNC_SIZE;
}

/* Setup */

/* Start PEs (C) */
void start_pes(int npes) {
  (void)npes; /* ignored, as by OSSS's own start_pes() */
  shmem_init();
}

/* Start PEs (FORTRAN )*/
void start_pes_(const int *npes) {
  (void)npes; /* ignored, as by the C start_pes() */
  shmem_init();

  bcast_psync = fshmem_psync_alloc(SHMEM_BCAST_SYNC_SIZE);
  reduce_psync = fshmem_psync_alloc(SHMEM_REDUCE_SYNC_SIZE);
  /* Every PE's pool is initialised before anyone uses it */
  shmem_barrier_all();
}

int my_pe_(void) { return shmem_my_pe(); }

int num_pes_(void) { return shmem_n_pes(); }

/* Synchronisation */

void shmem_barrier_all_(void) { shmem_barrier_all(); }

/* RMA; len counts elements */

void shmem_double_put_(double *target, const double *source, const int *len,
                       const int *pe) {
  shmem_double_put(target, source, (size_t)*len, *pe);
}

void shmem_double_get_(double *target, const double *source, const int *len,
                       const int *pe) {
  shmem_double_get(target, source, (size_t)*len, *pe);
}

/* Broadcasts; nelems counts 4- or 8-byte elements */

#define FSHMEM_BROADCAST(_fname, _cname)                                       \
  void _fname(void *target, const void *source, const int *nelems,             \
              const int *pe_root, const int *pe_start,                         \
              const int *log_pe_stride, const int *pe_size, int *psync) {      \
    (void)psync;                                                               \
    _cname(target, source, (size_t)*nelems, *pe_root, *pe_start,               \
           *log_pe_stride, *pe_size, fshmem_bcast_psync());                    \
  }

FSHMEM_BROADCAST(shmem_broadcast4_, shmem_broadcast32)
FSHMEM_BROADCAST(shmem_broadcast8_, shmem_broadcast64)

/* Reductions */

#define FSHMEM_REDUCE(_fname, _cname, _type)                                   \
  void _fname(_type *target, const _type *source, const int *nreduce,          \
              const int *pe_start, const int *log_pe_stride,                   \
              const int *pe_size, _type *pwrk, int *psync) {                   \
    (void)psync;                                                               \
    _cname(target, source, *nreduce, *pe_start, *log_pe_stride, *pe_size,      \
           pwrk, fshmem_reduce_psync());                                       \
  }

FSHMEM_REDUCE(shmem_real8_sum_to_all_, shmem_double_sum_to_all, double)
FSHMEM_REDUCE(shmem_real8_max_to_all_, shmem_double_max_to_all, double)
FSHMEM_REDUCE(shmem_real8_min_to_all_, shmem_double_min_to_all, double)
FSHMEM_REDUCE(shmem_int4_max_to_all_, shmem_int_max_to_all, int)

/* MPI_Wtime, for the benchmarks' timers */

double mpi_wtime_(void) {
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}
