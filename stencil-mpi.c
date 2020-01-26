#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <mpi.h>

int STENCIL_SIZE_X = 64;
int STENCIL_SIZE_Y = 64;

/** number of buffers for N-buffering; should be at least 2 */
int STENCIL_NBUFFERS = 2;

/** conduction coeff used in computation */
static const double alpha = 0.02;

/** threshold for convergence */
static const double epsilon = 0.0001;

/** max number of steps */
static const int stencil_max_steps = 10000;

static double *** values = NULL;

/** latest computed buffer */
static int current_buffer = 0;

int dimensions[2];

MPI_Comm grid;

int coords[2];
    
int grid_rank;

int block_width, block_height;

/** init stencil values to 0, borders to non-zero */
static void stencil_init() {
    int b, x, y;
    
    printf("blockwidth %d %d\n\n", block_width, block_height);

    values = malloc(STENCIL_NBUFFERS * sizeof(double **));
    for(b = 0; b<STENCIL_NBUFFERS; b++) {
        values[b] = malloc(sizeof(double *) * (block_width+1));
        for (x = 0; x < block_width+1; x++) {
            values[b][x] = malloc(sizeof(double) * (block_height+1));
        }
    }

    for(b = 0; b < STENCIL_NBUFFERS; b++) {
        for(x = coords[0]*block_width; x < (coords[0]+1)*block_width; x++) {
	        for(y = coords[1]*block_height; y < (coords[1]+1)*block_height; y++) {
	            values[b][x%block_width][y%block_height] = 0.0;
	        }
    	}
        
        for(x = coords[0]*block_width; x < (coords[0]+1)*block_width; x++) {
            values[b][x%block_width][0] = x;
	        values[b][x%block_width][(STENCIL_SIZE_Y - 1)%block_height] = STENCIL_SIZE_X - x;
	    }
        
        for(y = coords[1]*block_height; y < (coords[1]+1)*block_height; y++) {
	        values[b][0][y%block_height] = y;
	        values[b][(STENCIL_SIZE_X - 1)%block_width][y%block_height] = STENCIL_SIZE_Y - y;
	    }
    }
}

/** display a (part of) the stencil values */
static void stencil_display(int b, int x0, int x1, int y0, int y1) {
    int x, y;
    for(y = y0; y <= y1; y++) {
      for(x = x0; x <= x1; x++) {
	    printf("%8.5g ", values[b][x][y]);
	  }
      printf("\n");
    }
}

/** compute the next stencil step */
static void stencil_step(void) {
    int prev_buffer = current_buffer;
    int next_buffer = (current_buffer + 1) % STENCIL_NBUFFERS;
    int x, y;
    for(x = 1; x < block_width; x++) {
        for(y = 1; y < block_height; y++) {
	        values[next_buffer][x][y] =
	            alpha * values[prev_buffer][x - 1][y] +
	            alpha * values[prev_buffer][x + 1][y] +
	            alpha * values[prev_buffer][x][y - 1] +
	            alpha * values[prev_buffer][x][y + 1] +
	            (1.0 - 4.0 * alpha) * values[prev_buffer][x][y];
	    }
    }
    current_buffer = next_buffer;
}

/** return 1 if computation has converged */
static int stencil_test_convergence(void) {
    int prev_buffer = (current_buffer - 1 + STENCIL_NBUFFERS) % STENCIL_NBUFFERS;
    int x, y;
    for(x = 1; x < STENCIL_SIZE_X - 1; x++) {
        for(y = 1; y < STENCIL_SIZE_Y - 1; y++) {
	        if(fabs(values[prev_buffer][x][y] - values[current_buffer][x][y]) > epsilon)
	            return 0;
	    }
    }
    return 1;
}

inline int max(int a, int b) {
    return (a>=b)? a : b;
}

int main(int argc, char**argv) {
    MPI_Init(&argc, &argv);
    int pid;
    int np;
    int master = 0;
    bool display_enabled = false;
    struct timespec t1, t2;

    MPI_Comm_rank(MPI_COMM_WORLD, &pid); 
    MPI_Comm_size(MPI_COMM_WORLD, &np); 

    if (pid == master) {
        if (argc > 2) {
            STENCIL_SIZE_X = atoi(argv[1]);
            STENCIL_SIZE_Y = atoi(argv[2]);
        }

        display_enabled = max(STENCIL_SIZE_X, STENCIL_SIZE_Y) <= 10;

        if(display_enabled) {
            stencil_display(current_buffer, 0, STENCIL_SIZE_X - 1, 0, STENCIL_SIZE_Y - 1);
        }
    }        
    
    dimensions[0] = sqrt(np);
    dimensions[1] = 0;
    MPI_Dims_create(np, 2, dimensions);
    
    int periods[2] = {0, 0};
    MPI_Cart_create(MPI_COMM_WORLD, 2, dimensions, periods, false, &grid);
    
    MPI_Comm_rank(grid, &grid_rank);

    MPI_Cart_coords(grid, grid_rank, 2, coords);
    
    block_width = STENCIL_SIZE_X / dimensions[0];
    block_height = STENCIL_SIZE_Y / dimensions[1];


    stencil_init();
    
    printf("P%d(%d,%d)\n", grid_rank, coords[0], coords[1]);
    //stencil_display(current_buffer, 0, STENCIL_SIZE_X/dims[0]-1, 0, STENCIL_SIZE_Y/dims[1]-1);

    if (pid == master) {
        clock_gettime(CLOCK_MONOTONIC, &t1);
    }

    int s;
    for(s = 0; s < stencil_max_steps; s++) {
        stencil_step();
        if(stencil_test_convergence()) {
	        printf("# steps = %d\n", s);
	        break;
	    }
    }
    
    if (pid == master) {
        clock_gettime(CLOCK_MONOTONIC, &t2);
        const double t_usec = (t2.tv_sec - t1.tv_sec) * 1000000.0 + (t2.tv_nsec - t1.tv_nsec) / 1000.0;
        printf("# time = %g usecs.\n", t_usec);

        double GFLOPs = 5.0 * stencil_max_steps * STENCIL_SIZE_X * STENCIL_SIZE_Y / 1.0e9 / t_usec / 1.0e-6;

        printf("GFLOPs=%f\n", GFLOPs);

        if (display_enabled) {
            stencil_display(current_buffer, 0, STENCIL_SIZE_X - 1, 0, STENCIL_SIZE_Y - 1);
        }
    }
    MPI_Finalize();
    return 0;
}

