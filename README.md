Compilar y ejecutar

```
module load gcc openmpi/4.1.1_ft3
mpicc -o montecarlo_mpi -lm montecarlo_mpi.c
sbatch montecarlo_mpi.sh montecarlo_mpi
```

Comprobar la cola y el resultado de ejecución

```
squeue
cat myjob.*.out
```