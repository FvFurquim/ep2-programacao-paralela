# ep2-programacao-paralela

Como compilar:
make all

Como rodar:
mpirun -np 4 ./mandelbrot_omp_ompi -2.5 1.5 -2.0 2.0 8000

mpirun -np 4 ./mandelbrot_pth_ompi -2.5 1.5 -2.0 2.0 8000 4

mpirun -np 4 ./mandelbrot_seq_ompi -2.5 1.5 -2.0 2.0 8000
