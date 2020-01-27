CFLAGS += -Wall -Wextra -O4
ifdef debug
	CFLAGS += -DDEBUG
endif
LDLIBS += -lm -lrt
INCLUDES_MPI = -I /usr/include/openmpi-x86_64

all: stencil-baseline stencil-openmp stencil-mpi stencil-mpi-openmp

stencil-baseline: stencil-baseline.c
	$(CC) $(CFLAGS) $(LDLIBS) -o $@ $<

stencil-openmp: stencil-openmp.c
	$(CC) $(CFLAGS) $(LDLIBS) -o $@ $<
	
stencil-mpi: stencil-mpi.c
	mpicc $(CFLAGS) $(LDLIBS) $(INCLUDES_MPI) -o $@ $<

stencil-mpi-openmp: stencil-mpi.c
	mpicc $(CFLAGS) -fopenmp -DOPENMP_ENABLED $(LDLIBS) $(INCLUDES_MPI) -o $@ $<

clean:
	rm -f stencil-baseline stencil-openmp stencil-mpi stencil-mpi-openmp
