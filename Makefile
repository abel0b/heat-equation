CC=gcc
MPICC=mpicc
CFLAGS += -Wall -Wextra -O4
ifeq ($(debug),true)
	CFLAGS += -DDEBUG
endif
LDFLAGS += -lm -lrt
MPI_INCLUDE_DIRS = -I /usr/include/openmpi-x86_64
BINARY_DIR = build

.PHONY: all
all: $(BINARY_DIR)/stencil-baseline $(BINARY_DIR)/stencil-openmp $(BINARY_DIR)/stencil-mpi $(BINARY_DIR)/stencil-mpi-openmp $(BINARY_DIR)/stencil-compare

$(BINARY_DIR)/stencil-baseline: src/stencil-baseline.c
	mkdir -p $(BINARY_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(BINARY_DIR)/stencil-openmp: src/stencil-openmp.c
	mkdir -p $(BINARY_DIR)
	$(CC) $(CFLAGS) -fopenmp $(LDFLAGS) -o $@ $<
	
$(BINARY_DIR)/stencil-mpi: src/stencil-mpi.c
	mkdir -p $(BINARY_DIR)
	$(MPICC) $(CFLAGS) $(LDFLAGS) $(MPI_INCLUDE_DIRS) -o $@ $<

$(BINARY_DIR)/stencil-mpi-openmp: src/stencil-mpi.c
	mkdir -p $(BINARY_DIR)
	$(MPICC) $(CFLAGS) -fopenmp -DOPENMP_ENABLED $(LDFLAGS) $(MPI_INCLUDE_DIRS) -o $@ $<

$(BINARY_DIR)/stencil-compare: test/stencil-compare.c
	mkdir -p $(BINARY_DIR)
	$(CC) $(CFLAGS) -o $@ $<	

.PHONY: test
test: all
	./test/compare_outputs.sh

.PHONY: bench
bench: all
	./bench/speedup.sh

.PHONY: clean
clean:
	rm -rf $(BINARY_DIR)/*
