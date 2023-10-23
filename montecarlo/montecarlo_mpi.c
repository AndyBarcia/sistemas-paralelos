#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// ssh curso166@ft3.cesga.es
// Compilar y ejecutar:
// module load gcc openmpi/4.1.1_ft3
// mpicc -o montecarlo_mpi -lm montecarlo_mpi.c
// sbatch montecarlo_mpi.sh montecarlo_mpi
// squeue
// cat myjob.*.out

#define ITERATIONS_TOTAL 10000
#define ROOT_TASK 0

#define REAL_PI 3.1415926535897932384626433832795028841971693993751058209749446

int main(int argc, char **argv) {
	// Inicializar MPI.
	MPI_Init(&argc, &argv);

	int node=0, npes;
	MPI_Comm_size(MPI_COMM_WORLD, &npes);
	MPI_Comm_rank(MPI_COMM_WORLD, &node);

	printf("Node %d (of %d) starting.\n", node, npes);

	// Si el número de argumentos es incorrecto, abortar (y
	// reportar el error sólo en uno de los procesos).
	if (argc != 2) {
		if (node == ROOT_TASK)
			printf("Expecting exactly one argument.\n");
		MPI_Finalize();
		return 0;
	}

	// Medir resolución del reloj y el overhead de medir el tiempo tomado.
	double t_resolution = MPI_Wtick();
	double t_start = MPI_Wtime();
	double t_end = MPI_Wtime();
	double measure_overhead = t_end - t_start;

	printf("Node %d (of %d) has time resolution of %e seconds and a measure overhead of %e seconds.\n", node, npes,t_resolution, measure_overhead);

	// Calcular el número total de iteraciones y el
	// número de iteraciones por proceso.
	long iterations_total = strtol(argv[1], NULL, 10);
	long iterations = iterations_total / npes;

	// Que cada proceso inicialice de forma aleatoria
	// la semilla con su tiempo (para que cada evaluación
	// esa distinta) y su número de proceso (esto es 
	// importante, porque con la misma semilla todos 
	// los procesos evaluarían los mismos puntos).
	srand(time(NULL) + node);
	double total = 0.0;

	t_start = MPI_Wtime();

	for (long i = 0; i < iterations; ++i) {
		// Generar un número aleatorio [0,RAND_MAX]
		int r = rand();
		// Convertirlo al rango [0,RAND_MAX), para
		// evitar una división 1/0.
		if (r == RAND_MAX) { r = RAND_MAX-1; }

		// Evaluar 1/sqrt(1-x^2)
		double x = (double) r / RAND_MAX;
		total += 1.0 / sqrt(1 - x*x);
	}

	// Tomar la medida del tiempo que le tomó al proceso hacer su
	// aproximación de PI con su número de iteraciones.
	double t_end_local = MPI_Wtime();

	// Si no es el nodo ROOT, mandar su resultado a ROOT.
	if (node != ROOT_TASK) {
		MPI_Send(&total, 1, MPI_DOUBLE, ROOT_TASK, 0, MPI_COMM_WORLD);
	}

	// Reportar la aproximación de PI de este proceso.
	// Recordar que integamos sólo en el rango [0,1). Como la función
	// es simétrica, duplicar para obtener la integral de [-1,1].
	double local_total = (total * 2.0) / (double) iterations;
	double t_elapsed_local = (t_end_local - t_start) - measure_overhead;
	double error_local = abs(local_total - REAL_PI);
	double quality_local = 1.0 / (t_elapsed_local * error_local);

	printf("Local %d approx of PI with %ld iterations:\n\
\t-is %f\n\
\t-has an error %e\n\
\t-took %e seconds\n\
\t-quality %e\n", node, iterations, local_total, error_local, t_elapsed_local, quality_local);

	// Reportar la aproximación de PI de todos los procesos,
	// sumando el total de cada uno de los procesos.
	if (node == ROOT_TASK) {
		for (int i = 1; i < npes ; i++){
			double aux;
			MPI_Recv(&aux, 1, MPI_DOUBLE, i, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			total += aux;
		}

		// Reportar la aproximación de PI de todo el conjunto de procesos.
		double collective_total = (total * 2.0) / (double) iterations_total;
		double t_end = MPI_Wtime();
		double t_elapsed = (t_end - t_start) - measure_overhead;
		double error = abs(collective_total - REAL_PI);
		double quality = 1.0 / (t_elapsed * error);

		printf("Global approx of PI with %ld iterations:\n\
\t-is %f\n\
\t-has an error %e\n\
\t-took %e seconds\n\
\t-quality %e\n", iterations_total, collective_total, error, t_elapsed, quality);
	}

	MPI_Finalize();
}
