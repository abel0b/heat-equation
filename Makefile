CC      = gcc
CFLAGS += -Wall -Wextra -g -O4 -fopenmp
LDLIBS += -lm -lrt
INCLUDES = -I /usr/include/openmpi-x86_64

all: stencil-baseline stencil-openmp stencil-mpi

stencil-mpi: stencil-mpi.c
	mpicc $(CFLAGS) $(LDLIBS) $(INCLUDES) -o $@ $<

stencil-%: stencil-%.c
	$(CC) $(CFLAGS) $(LDLIBS) $(INCLUDES) -o $@ $<

clean:
	rm -f stencil-baseline stencil-openmp

