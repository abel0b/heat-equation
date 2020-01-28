CC=gcc
MPICC=mpicc
CFLAGS+=-Wall -Wextra
ifeq ($(debug),true)
	CFLAGS += -DDEBUG
endif
LDFLAGS+=-lm -lrt
MPI_INCLUDE_DIRS=-I/usr/include/openmpi-x86_64
BINARY_DIR=build

.PHONY: all
all: $(BINARY_DIR) $(BINARY_DIR)/stencil-baseline $(BINARY_DIR)/stencil-baseline-unoptimized $(BINARY_DIR)/stencil-openmp $(BINARY_DIR)/stencil-mpi $(BINARY_DIR)/stencil-mpi-openmp $(BINARY_DIR)/stencil-compare

$(BINARY_DIR):
	mkdir -p $(BINARY_DIR)

$(BINARY_DIR)/stencil-baseline: src/stencil-baseline.c
	$(CC) $(CFLAGS) -O3 -mavx $(LDFLAGS) -o $@ $<

$(BINARY_DIR)/stencil-baseline-unoptimized: src/stencil-baseline.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(BINARY_DIR)/stencil-openmp: src/stencil-openmp.c
	$(CC) $(CFLAGS) -O3 -mavx -fopenmp $(LDFLAGS) -o $@ $<
	
$(BINARY_DIR)/stencil-mpi: src/stencil-mpi.c
	$(MPICC) $(CFLAGS) -O3 -mavx $(LDFLAGS) $(MPI_INCLUDE_DIRS) -o $@ $<

$(BINARY_DIR)/stencil-mpi-openmp: src/stencil-mpi.c
	$(MPICC) $(CFLAGS) -O3 -mavx -fopenmp -DOPENMP_ENABLED $(LDFLAGS) $(MPI_INCLUDE_DIRS) -o $@ $<

$(BINARY_DIR)/stencil-compare: test/stencil-compare.c
	$(CC) $(CFLAGS) -O3 -mavx -o $@ $<	

.PHONY: test
test: all
	./test/compare_outputs.sh

.PHONY: bench
bench: all
	./bench/speedup.sh

.PHONY: bench-weak-scalability
bench-weak-scalability:
	salloc -p mistral -N 4 --exclusive bash -c 'source env.sh && ./bench/weak_scalability.sh'

.PHONY: bench-strong-scalability
bench-strong-scalability:
	salloc -p mistral -N 4 --exclusive bash -c 'source env.sh && ./bench/strong_scalability.sh'


.PHONY: clean
clean:
	rm -rf $(BINARY_DIR)/* core*
