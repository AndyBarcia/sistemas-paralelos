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

// Calcular el número de escalones necesarios
// para repartir n procesos, opcionalmente
// calculando también cuántos quedan fuera.
int ilog2(int n, int *sobran) {
    int n_original = n;

    int j;
    for (j = 0; n > 0; ++j, n>>=1) {}
    if (sobran != NULL)
        *sobran = n_original - (1 << (j-1));
    return j;
}

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
	double total_local = 0.0;

	t_start = MPI_Wtime();

	for (long i = 0; i < iterations; ++i) {
		// Generar un número aleatorio [0,RAND_MAX]
		int r = rand();
		// Convertirlo al rango [0,RAND_MAX), para
		// evitar una división 1/0.
		if (r == RAND_MAX) { r = RAND_MAX-1; }

		// Evaluar 1/sqrt(1-x^2)
		double x = (double) r / RAND_MAX;
		total_local += 1.0 / sqrt(1 - x*x);
	}

	// Tomar la medida del tiempo que le tomó al proceso hacer su
	// aproximación de PI con su número de iteraciones.
	double t_end_local = MPI_Wtime();

	// Envío de datos en escalera. Cada proceso (excepto el ROOT) 
	// recibe datos de dos procesos del escalón superior, y cada 
	// proceso (excepto ROOT) manda datos a su correspondiente
	// del escalón inferior.
    //
	//                          8 9
	//                4 5 6 7 |--------- sobrantes
	//          2 3 |---------| nivel 3
	//      1 |-----| nivel 2
	//  0 |---| nivel 1
	// ---| nivel 0
	//

	// Calcular cuantos escalones hacen falta para todos los procesos,
	// y cuántos procesos quedan sueltos (no llenan un escalón completo).
    int sobran;
    int niveles = ilog2(npes, &sobran);
	// Calcular el escalón de nuestro proceso, y qué posición tiene.
	int local_i;
	int nivel = ilog2(node, &local_i);

	// Calcular el número de mensajes que tiene que recibir cada proceso
	// en base a su escalón y su posición en el escalón.
	int receives;         
	if (nivel < (niveles-1)) {
		// Si no estamos en el último nivel, hacemos siempre
		// 2 receives, excepto el ROOT, que hace solo 1 receive.
		receives = (node == ROOT_TASK) ? 1 : 2;
	} else if (nivel == (niveles-1)) {
		// En el último nivel, el número de receives depende de
		// cuantos procesos queden sueltos. Y habrá procesos
		// que a lo mejor no reciben nada.
		receives = sobran - (local_i*2);
		receives = receives > 0 ? receives : 0;
	} else {
		// Los procesos sueltos no tienen que recibir nada.
		receives = 0;
	}
	// Recibir tantos datos como se ha calculado.
	double total_global = total_local;
	for (int i = 0; i < receives; ++i) {
		double aux;
		printf("Node %d is awaiting for result %d\n", node, i);
		MPI_Status status;
		MPI_Recv(&aux, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("Node %d received data from %d\n", node, status.MPI_SOURCE);
		total_global += aux;
	}

	// Si no es el primer escalón (ROOT), enviar los datos
	// al proceso correspondiente del escalón inferior.
	if (node != ROOT_TASK) {
		int send_to = node >> 1;
		printf("Node %d sending results to %d\n", node, send_to);
		MPI_Send(&total_global, 1, MPI_DOUBLE, send_to, 0, MPI_COMM_WORLD);
	}

	// Reportar la aproximación de PI de este proceso.
	// Recordar que integamos sólo en el rango [0,1). Como la función
	// es simétrica, duplicar para obtener la integral de [-1,1].
	total_local = (total_local * 2.0) / (double) iterations;
	double t_elapsed_local = (t_end_local - t_start) - measure_overhead;
	double error_local = fabs(total_local - REAL_PI);
	double quality_local = 1.0 / (t_elapsed_local * error_local);

	printf("Local %d approx of PI with %ld iterations:\n\
\t-is %f\n\
\t-has an error %e\n\
\t-took %e seconds\n\
\t-quality %e\n", node, iterations, total_local, error_local, t_elapsed_local, quality_local);

	// Reportar la aproximación de PI de todos los procesos,
	// sumando el total de cada uno de los procesos.
	if (node == ROOT_TASK) {
		// Reportar la aproximación de PI de todo el conjunto de procesos.
		total_global = (total_global * 2.0) / (double) iterations_total;
		double t_end = MPI_Wtime();
		double t_elapsed = (t_end - t_start) - measure_overhead;
		double error = fabs(total_global - REAL_PI);
		double quality = 1.0 / (t_elapsed * error);

		printf("Global approx of PI with %ld iterations:\n\
\t-is %f\n\
\t-has an error %e\n\
\t-took %e seconds\n\
\t-quality %e\n", iterations_total, total_global, error, t_elapsed, quality);
	}

	printf("Node %d finished\n", node);

	MPI_Finalize();
}
