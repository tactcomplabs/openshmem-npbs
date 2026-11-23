# Fortran SHMEM shim for the OpenSHMEM NAS Parallel Benchmarks. Needs OSSS's
# oshcc on PATH. Link libfshmem.a into Fortran programs and add include/ to
# their include path (it provides mpp/shmem.fh and a stand-in mpif.h).

OSHCC  ?= oshcc
CFLAGS ?= -O2 -g -Wall

libfshmem.a: fshmem.o
	$(AR) rcs $@ $^

fshmem.o: fshmem.c
	$(OSHCC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f fshmem.o libfshmem.a

.PHONY: clean
