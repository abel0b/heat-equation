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

#define VALUES(b,x,y) values[(b)*(block_width+2)*(block_height+2)+(x)*(block_width+2)+(y)]

/** conduction coeff used in computation */
const double alpha = 0.02;

/** threshold for convergence */
const double epsilon = 0.0001;

/** max number of steps */
int stencil_max_steps = 10000;

double * values = NULL;

/** latest computed buffer */
static int current_buffer = 0;

int dimensions[2];

MPI_Comm grid;

int coordinates[2];
    
int grid_rank;

int block_width, block_height;

MPI_Datatype row, column;

int neighbors[4];

enum Neighbor {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

/** init stencil values to 0, borders to non-zero */
static void stencil_init() {
    int b, x, y;
    
    values = malloc((block_width+2) * (block_height+2) * STENCIL_NBUFFERS * sizeof(double));

    for(b = 0; b < STENCIL_NBUFFERS; b++) {
        for(x=0; x<block_width+2; x++) {
            for(y=0;y<block_height+2;y++) {
                VALUES(b,x,y) = 0.0;
            }
        }

        if (coordinates[1] == 0) {
            for(x = 1; x < block_width+1; x++) {
                VALUES(b,x,1) = block_height*coordinates[0] + x - 1;
	        }
        }
	    else if(coordinates[1] == dimensions[1]-1) {
            for(x = 1; x < block_width+1; x++) {
                VALUES(b,x,block_height) = STENCIL_SIZE_X - (block_height*coordinates[0]+x-1) ;
            }
        }

        if (coordinates[0] == 0) {
            for(y = 1; y < block_height+1; y++) {
	            VALUES(b,1,y) = block_width*coordinates[1]+y-1;
            }
        }
        else if (coordinates[0] == dimensions[0]-1) {
            for(y = 1; y < block_height+1; y++) {
	            VALUES(b,block_width,y) = STENCIL_SIZE_Y - (block_height*coordinates[1]+y-1);
            }
        }
    }
}

/** display a (part of) the stencil values */
static void stencil_display(int b, int x0, int x1, int y0, int y1) {
    int x, y;
    for(y = y0; y <= y1; y++) {
      for(x = x0; x <= x1; x++) {
	    printf("%8.5g ", VALUES(b,x,y));
	  }
      printf("\n");
    }
}

/** compute the next stencil step */
static void stencil_step(void) {
    int prev_buffer = current_buffer;
    int next_buffer = (current_buffer + 1) % STENCIL_NBUFFERS;
    int x, y;

    for(x = 0; x < block_width; x++) {
        if ((coordinates[0] == 0 && x == 0) || (coordinates[0] == dimensions[0]-1 && x == block_width-1)) {
            continue;
        }
        for(y = 0; y < block_height; y++) {
	        if ((coordinates[1] == 0 && y == 0) || (coordinates[1] == dimensions[1]-1 && x == block_height-1)) {
                continue;
            }
            VALUES(next_buffer,x+1,y+1) =
	            alpha * VALUES(prev_buffer,x ,y+1) +
	            alpha * VALUES(prev_buffer,x + 2,y+1) +
	            alpha * VALUES(prev_buffer,x+1,y) +
	            alpha * VALUES(prev_buffer,x+1,y + 2) +
	            (1.0 - 4.0 * alpha) * VALUES(prev_buffer,x+1,y+1);
	    }
    }

    int nb_requests = 0;
    MPI_Request requests[8];

    // Send border data to neighbors    
    MPI_Isend(&VALUES(current_buffer, 1, 1), 1, column, neighbors[LEFT], 0, grid, &requests[nb_requests++]);
    MPI_Isend(&VALUES(current_buffer, 1, block_width), 1, column, neighbors[RIGHT], 0, grid, &requests[nb_requests++]);
    MPI_Isend(&VALUES(current_buffer, 1, 1), 1, row, neighbors[UP], 0, grid, &requests[nb_requests++]);
    MPI_Isend(&VALUES(current_buffer, block_height, 1), 1, row, neighbors[DOWN], 0, grid, &requests[nb_requests++]);

    // Receive border data from neighbors
    MPI_Irecv(&VALUES(current_buffer, 1, 0), 1, column, neighbors[LEFT], 0, grid, &requests[nb_requests++]);
    MPI_Irecv(&VALUES(current_buffer, 1, block_width+1), 1, column, neighbors[RIGHT], 0, grid, &requests[nb_requests++]);
    MPI_Irecv(&VALUES(current_buffer, 0, 1), 1, row, neighbors[UP], 0, grid, &requests[nb_requests++]);
    MPI_Irecv(&VALUES(current_buffer, block_height+1, 1), 1, row, neighbors[DOWN], 0, grid, &requests[nb_requests++]);

    MPI_Status statuses[8];
    MPI_Waitall(nb_requests, requests, statuses);

    current_buffer = next_buffer;
}

/** return 1 if computation has converged */
static int stencil_test_convergence(void) {
    int prev_buffer = (current_buffer - 1 + STENCIL_NBUFFERS) % STENCIL_NBUFFERS;
    int x, y;
    int converged = 1;

    for(x = 0; x < block_width; x++) {
        for(y = 0; y < block_height; y++) {
	        if(fabs(VALUES(prev_buffer,x+1,y+1) - VALUES(current_buffer,x+1,y+1)) > epsilon) {
	            converged = 0;
            }
	    }
    }
    
    int all_converged = 0;

    MPI_Allreduce(&converged, &all_converged, 1, MPI_INT, MPI_LAND, grid);

    return all_converged;
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

    if (argc > 2) {
        STENCIL_SIZE_X = atoi(argv[1]);
        STENCIL_SIZE_Y = atoi(argv[2]);
    }

    if (argc > 3) {
        stencil_max_steps = atoi(argv[3]);
    }

    if (pid == master) {
        display_enabled = max(STENCIL_SIZE_X, STENCIL_SIZE_Y) <= 10;
    } 

    // Create cartesian topology
    // Each processor is responsible for a block alsmost square
    dimensions[0] = sqrt(np);
    dimensions[1] = 0;
    MPI_Dims_create(np, 2, dimensions);
    
    int periods[2] = {0, 0};
    MPI_Cart_create(MPI_COMM_WORLD, 2, dimensions, periods, false, &grid);
    
    if (MPI_Comm_rank(grid, &grid_rank) != MPI_SUCCESS) {
        exit(1);
    }

    MPI_Cart_coords(grid, grid_rank, 2, coordinates);
    
    // Retrieve neighbors ranks
    int source_rank;
    MPI_Cart_shift(grid, 0, 1, &source_rank, &neighbors[RIGHT]);
    MPI_Cart_shift(grid, 0, -1, &source_rank, &neighbors[LEFT]);
    MPI_Cart_shift(grid, 1, 1, &source_rank, &neighbors[UP]);
    MPI_Cart_shift(grid, 1, -1, &source_rank, &neighbors[DOWN]);
    
    block_width = STENCIL_SIZE_X / dimensions[0];
    block_height = STENCIL_SIZE_Y / dimensions[1];

    // Create column and row datatypes
    MPI_Type_contiguous(block_width, MPI_DOUBLE, &row);
    MPI_Type_vector(block_height, 1, block_width, MPI_DOUBLE, &column);
    MPI_Type_commit(&row);
    MPI_Type_commit(&column);

    // Initialize stencil values
    stencil_init();

    if(display_enabled && pid == master) {
        stencil_display(current_buffer, 0, block_width+1, 0, block_height+1);
    }  

    // Print debugging information
    #ifdef DEBUG
    if (pid == master) {
        printf("grid_dimensions=%d %d\n", dimensions[0], dimensions[1]);
        printf("block_width=%d\n", block_width);
        printf("block_height=%d\n", block_height);
    }
    MPI_Barrier(grid);
    printf("P%d(%d,%d)\n", grid_rank, coordinates[0], coordinates[1]);
    MPI_Barrier(grid);
    #endif
    
    if (pid == master) {
        clock_gettime(CLOCK_MONOTONIC, &t1);
    }

    int s;
    for(s = 0; s < stencil_max_steps; s++) {
        // Update stencil values
        stencil_step();
        
        if(stencil_test_convergence()) {
	        printf("# steps = %d\n", s);
	        break;
	    }
    }
    
    // Output results
    if (pid == master) {
        clock_gettime(CLOCK_MONOTONIC, &t2);
        const double t_usec = (t2.tv_sec - t1.tv_sec) * 1000000.0 + (t2.tv_nsec - t1.tv_nsec) / 1000.0;
        printf("# time = %g usecs.\n", t_usec);

        double GFLOPs = 5.0 * stencil_max_steps * STENCIL_SIZE_X * STENCIL_SIZE_Y / 1.0e9 / t_usec / 1.0e-6;

        printf("GFLOPs=%f\n", GFLOPs);

        if (display_enabled) {
            stencil_display(current_buffer, 0, block_width+1, 0, block_height+2);
        }
    }
    MPI_Finalize();
    return 0;
}

