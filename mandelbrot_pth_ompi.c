#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <mpi.h>

#define MASTER 0

double c_x_min;
double c_x_max;
double c_y_min;
double c_y_max;

double pixel_width;
double pixel_height;

int iteration_max = 200;

int image_size;
unsigned char **image_buffer;

int i_x_max;
int i_y_max;
int image_buffer_size;

int nthreads = 1;
int current_iy;
pthread_mutex_t stacklock = PTHREAD_MUTEX_INITIALIZER;

int taskid, numtasks, provided;
MPI_Status status;
int *a_iteration;

int gradient_size = 16;
int colors[17][3] = {
                        {66, 30, 15},
                        {25, 7, 26},
                        {9, 1, 47},
                        {4, 4, 73},
                        {0, 7, 100},
                        {12, 44, 138},
                        {24, 82, 177},
                        {57, 125, 209},
                        {134, 181, 229},
                        {211, 236, 248},
                        {241, 233, 191},
                        {248, 201, 95},
                        {255, 170, 0},
                        {204, 128, 0},
                        {153, 87, 0},
                        {106, 52, 3},
                        {16, 16, 16},
                    };

void allocate_image_buffer(){
    int rgb_size = 3;
    image_buffer = (unsigned char **) malloc(sizeof(unsigned char *) * image_buffer_size);

    for(int i = 0; i < image_buffer_size; i++){
        image_buffer[i] = (unsigned char *) malloc(sizeof(unsigned char) * rgb_size);
    };
};

void init(int argc, char *argv[]){
    if(argc < 6){
        printf("usage: ./mandelbrot_pth c_x_min c_x_max c_y_min c_y_max image_size n_threads\n");
        printf("examples with image_size = 11500:\n");
        printf("    Full Picture:         ./mandelbrot_pth -2.5 1.5 -2.0 2.0 11500 2\n");
        printf("    Seahorse Valley:      ./mandelbrot_pth -0.8 -0.7 0.05 0.15 11500 2\n");
        printf("    Elephant Valley:      ./mandelbrot_pth 0.175 0.375 -0.1 0.1 11500 2\n");
        printf("    Triple Spiral Valley: ./mandelbrot_pth -0.188 -0.012 0.554 0.754 11500 2\n");
        exit(0);
    }
    else{
        sscanf(argv[1], "%lf", &c_x_min);
        sscanf(argv[2], "%lf", &c_x_max);
        sscanf(argv[3], "%lf", &c_y_min);
        sscanf(argv[4], "%lf", &c_y_max);
        sscanf(argv[5], "%d", &image_size);

        i_x_max           = image_size;
        i_y_max           = image_size;
        image_buffer_size = image_size * image_size;

        pixel_width       = (c_x_max - c_x_min) / i_x_max;
        pixel_height      = (c_y_max - c_y_min) / i_y_max;
    };
};

void update_rgb_buffer() {
    int iteration, permission;

    for (int i = 1; i < numtasks; i++) {
        a_iteration = (int *) malloc(i_y_max * ((i_x_max + numtasks - 1)/numtasks) * sizeof(int));
        MPI_Send(&permission, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
        MPI_Recv(a_iteration, i_y_max * ((i_y_max + numtasks - 1)/numtasks), MPI_INT, i, 1, MPI_COMM_WORLD, &status);

        for (int x = i ; x < i_x_max; x += numtasks) {
            for (int y = 0; y < i_y_max; y++) {
                iteration = a_iteration[(i_x_max *(x/numtasks)) + y];

                int color;
                if(iteration == iteration_max) {

                    image_buffer[(i_y_max * y) + x][0] = colors[gradient_size][0];
                    image_buffer[(i_y_max * y) + x][1] = colors[gradient_size][1];
                    image_buffer[(i_y_max * y) + x][2] = colors[gradient_size][2];
                } else {
                    color = iteration % gradient_size;

                    image_buffer[(i_y_max * y) + x][0] = colors[color][0];
                    image_buffer[(i_y_max * y) + x][1] = colors[color][1];
                    image_buffer[(i_y_max * y) + x][2] = colors[color][2];
                }
            }
        }
    }
};

void write_to_file(){
    FILE * file;
    char * filename               = "output.ppm";
    char * comment                = "# ";

    int max_color_component_value = 255;

    file = fopen(filename,"wb");

    fprintf(file, "P6\n %s\n %d\n %d\n %d\n", comment,
            i_x_max, i_y_max, max_color_component_value);

    for(int i = 0; i < image_buffer_size; i++){
        fwrite(image_buffer[i], 1 , 3, file);
    };

    fclose(file);
};

void *buffer_updater(void *args) {
    long tid = (long) args;
    double z_x;
    double z_y;
    double z_x_squared;
    double z_y_squared;
    double escape_radius_squared = 4;

    int iteration;
    int i_x;
    int i_y;

    double c_x;
    double c_y;

    for(i_x = taskid + (numtasks)*tid; i_x < i_x_max; i_x += numtasks*nthreads) {
        c_x = c_x_min + i_x * pixel_width;

        for(i_y = 0; i_y < i_y_max; i_y++) {
            c_y = c_y_min + i_y * pixel_height;

            if(fabs(c_y) < pixel_height / 2){
                c_y = 0.0;
            };

            z_x         = 0.0;
            z_y         = 0.0;

            z_x_squared = 0.0;
            z_y_squared = 0.0;

            for(iteration = 0;
                iteration < iteration_max && \
                ((z_x_squared + z_y_squared) < escape_radius_squared);
                iteration++){
                z_y         = 2 * z_x * z_y + c_y;
                z_x         = z_x_squared - z_y_squared + c_x;

                z_x_squared = z_x * z_x;
                z_y_squared = z_y * z_y;
            };

            if (taskid == MASTER) {
                int color;
                if(iteration == iteration_max){
                    image_buffer[(i_y_max * i_y) + i_x][0] = colors[gradient_size][0];
                    image_buffer[(i_y_max * i_y) + i_x][1] = colors[gradient_size][1];
                    image_buffer[(i_y_max * i_y) + i_x][2] = colors[gradient_size][2];
                }
                else{
                    color = iteration % gradient_size;

                    image_buffer[(i_y_max * i_y) + i_x][0] = colors[color][0];
                    image_buffer[(i_y_max * i_y) + i_x][1] = colors[color][1];
                    image_buffer[(i_y_max * i_y) + i_x][2] = colors[color][2];
                };
            } else {
                a_iteration[(i_x_max *(i_x/numtasks)) + i_y] = iteration;
            }
        }
    }

    pthread_exit(NULL);
}

void compute_mandelbrot() {
    if (taskid != MASTER) {
         a_iteration = (int *) malloc(i_y_max * ((i_y_max + numtasks - 1)/numtasks) * sizeof(int));
    }

    pthread_t thread[nthreads];
    for (long t = 0; t < nthreads; t++) {
        if (pthread_create(&thread[t], NULL, buffer_updater, (void*) t)) {
            printf("\n ERROR creating thread %ld\n", t);
            exit(1);
        }
    }

    for (int t = 0; t < nthreads; t++) {
        if (pthread_join(thread[t], NULL)) {
              printf("\n ERROR joining thread %d\n", t);
              exit(1);
        }
    }

    int permission;
    if (taskid != MASTER) {
        MPI_Recv(&permission, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD, &status);
        MPI_Send(a_iteration, i_y_max * ((i_y_max + numtasks - 1)/numtasks), MPI_INT, MASTER, 1, MPI_COMM_WORLD);
    }
};

int main(int argc, char *argv[]){
    struct timespec start_a, start_b, start_c, start_d, start_e, stop, start, end;
    double duration;

    MPI_Init_thread(0, 0, MPI_THREAD_MULTIPLE, &provided );
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);

    if(argc > 6) {
        nthreads = atoi(argv[6]);
    }

    clock_gettime(CLOCK_MONOTONIC, &start_a);
    init(argc, argv);

    if(taskid == MASTER) {
        clock_gettime(CLOCK_MONOTONIC, &start_b);
        allocate_image_buffer();
    }

    clock_gettime(CLOCK_MONOTONIC, &start_c);

    compute_mandelbrot();

    if(taskid != MASTER) {
        MPI_Barrier(MPI_COMM_WORLD);
    } else {
        update_rgb_buffer();
        MPI_Barrier(MPI_COMM_WORLD);

        clock_gettime(CLOCK_MONOTONIC, &start_d);
        write_to_file();
    }

    clock_gettime(CLOCK_MONOTONIC, &stop);


    for(int i = 0; i < argc; i++){
        printf("%s ", argv[i]);
    }

    printf(";");

    duration = ((double)start_b.tv_sec + 1.0e-9*start_b.tv_nsec) -
                ((double)start_a.tv_sec + 1.0e-9*start_a.tv_nsec); 
    printf( "%.5lf;", duration);

    duration = ((double)start_c.tv_sec + 1.0e-9*start_c.tv_nsec) - 
                ((double)start_b.tv_sec + 1.0e-9*start_b.tv_nsec); 
    printf( "%.5lf;", duration);

    duration = ((double)start_d.tv_sec + 1.0e-9*start_d.tv_nsec) - 
                ((double)start_c.tv_sec + 1.0e-9*start_c.tv_nsec); 
    printf( "%.5lf;", duration);

    duration = ((double)stop.tv_sec + 1.0e-9*stop.tv_nsec) - 
                ((double)start_d.tv_sec + 1.0e-9*start_d.tv_nsec); 
    printf( "%.5lf;", duration);

    duration = ((double)stop.tv_sec + 1.0e-9*stop.tv_nsec) - 
                ((double)start_a.tv_sec + 1.0e-9*start_a.tv_nsec); 
    printf( "%.5lf;\n", duration);

    MPI_Finalize();

    return 0;
};
