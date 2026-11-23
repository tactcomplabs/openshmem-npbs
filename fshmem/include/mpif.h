!     Stand-in for MPI's mpif.h, for the OpenSHMEM NAS Parallel Benchmarks.
!     They never initialise MPI: they only time with MPI_Wtime (provided by
!     fshmem.c) and reference a few constants whose values they don't
!     depend on. Valid as both fixed- and free-form source.
      integer MPI_ADDRESS_KIND
      parameter (MPI_ADDRESS_KIND = 8)
      integer MPI_REAL, MPI_DOUBLE_PRECISION
      parameter (MPI_REAL = 13, MPI_DOUBLE_PRECISION = 17)
      double precision MPI_WTIME
      external MPI_WTIME
