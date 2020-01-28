#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#ifndef EPSILON
#define EPSILON 0.01
#endif

#define CHECK_READ(a,b) if ((int)(a)<(b)) {\
    fprintf(stderr, "Read operation failed\n");\
}

/*
 * Compare two stencil dumps
 */
int main(int argc, char * argv[]) {
    if(argc<3) {
        fprintf(stderr, "Error: missing arguments\n\nUsage: %s [file1] [file2]\n", argv[0]);
        exit(1);
    } 

    FILE * input1;
    FILE * input2;
    input1 = fopen(argv[1], "rb");
    input2 = fopen(argv[2], "rb");
    

    if (input1 == NULL || input2 == NULL) {
        perror("Error opening file");
        exit(1);
    }

    int size_x1, size_y1;
    int size_x2, size_y2; 
    
    CHECK_READ(fread(&size_x1, sizeof(int), 1, input1), 1);
    CHECK_READ(fread(&size_y1, sizeof(int), 1, input1), 1);
     
    CHECK_READ(fread(&size_x2, sizeof(int), 1, input2), 1);
    CHECK_READ(fread(&size_y2, sizeof(int), 1, input2), 1);
     
    if ((size_x1 != size_x2) || (size_y1 != size_y2)) {
        fprintf(stderr, "Headers are differents (%d,%d) != (%d,%d)\n", size_x1, size_y1, size_x2, size_y2);
        return EXIT_FAILURE;
    }
    
    double buffer1[size_x1*size_y1];
    double buffer2[size_x1*size_y2];

    CHECK_READ(fread(buffer1, sizeof(double), size_x1*size_y1, input1), size_x1*size_y1);
    CHECK_READ(fread(buffer2, sizeof(double), size_x1*size_y1, input2), size_x1*size_y1);
    
    int i;
    for(i=0; i<size_x1*size_y1; i++) {
        if((fabs(buffer1[i]-buffer2[i]) > EPSILON)) {
            #ifdef DEBUG
            fprintf(stderr, "%f\n", fabs(buffer1[i]-buffer2[i]));
            #endif
            return EXIT_FAILURE;
        }
    }
    fclose(input1);
    fclose(input2);
    return EXIT_SUCCESS;
}
