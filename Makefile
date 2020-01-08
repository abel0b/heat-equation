CC      = gcc
CFLAGS += -Wall -Wextra -g -O4 -fopenmp
LDLIBS += -lm -lrt

all: stencil-baseline stencil-openmp

stencil-%: stencil-%.c
	$(CC) $(CFLAGS) $(LDLIBS) $< -o $@

clean:
	rm -f stencil-baseline stencil-openmp

