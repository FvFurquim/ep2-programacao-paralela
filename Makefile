OUTPUT=mandelbrot

IMAGE=.ppm

CC=gcc
CC_OPT=-std=c11
CC_MPI=mpicc

CC_OMP=-fopenmp
CC_PTH=-pthread

.PHONY: all
all: $(OUTPUT)_omp $(OUTPUT)_pth $(OUTPUT)_seq $(OUTPUT)_omp_ompi $(OUTPUT)_pth_ompi $(OUTPUT)_seq_ompi

$(OUTPUT)_omp: $(OUTPUT)_omp.c
	$(CC) -o $(OUTPUT)_omp $(CC_OPT) $(CC_OMP) $(OUTPUT)_omp.c

$(OUTPUT)_pth: $(OUTPUT)_pth.c
	$(CC) -o $(OUTPUT)_pth $(CC_OPT) $(CC_PTH) $(OUTPUT)_pth.c

$(OUTPUT)_seq: $(OUTPUT)_seq.c
	$(CC) -o $(OUTPUT)_seq $(CC_OPT) $(OUTPUT)_seq.c

$(OUTPUT)_omp_ompi: $(OUTPUT)_omp_ompi.c
	$(CC_MPI) -o $(OUTPUT)_omp_ompi $(CC_OPT) $(OUTPUT)_omp_ompi.c $(CC_OMP) -lm

$(OUTPUT)_pth_ompi: $(OUTPUT)_pth_ompi.c
	$(CC_MPI) -o $(OUTPUT)_pth_ompi $(CC_OPT) $(OUTPUT)_pth_ompi.c $(CC_PTH) -lm

$(OUTPUT)_seq_ompi: $(OUTPUT)_seq_ompi.c
	$(CC_MPI) -o $(OUTPUT)_seq_ompi $(CC_OPT) $(OUTPUT)_seq_ompi.c -lm

.PHONY: clean
clean:
	rm $(OUTPUT)_omp $(OUTPUT)_pth $(OUTPUT)_seq $(OUTPUT)_omp_ompi $(OUTPUT)_pth_ompi $(OUTPUT)_seq_ompi *$(IMAGE)
