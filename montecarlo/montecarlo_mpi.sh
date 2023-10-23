#!/bin/bash
#----------------------------------------------------
# Generic SLURM script -- MPI Hello World
#----------------------------------------------------
#SBATCH -J myjob             # Job name
#SBATCH -o myjob.%j.out      # stdout; %j expands to jobid
#SBATCH -e myjob.%j.err      # stderr; skip to combine stdout and stderr
#SBATCH -N 4                 # Number of nodes, not cores
#SBATCH -n 32                # Total number of MPI tasks
#SBATCH -t 00:10:00          # max time
#SBATCH --mem=4G             # memory per core demanded

#module load cesga/2020 intel impi
module load cesga/2020 gcc openmpi/4.1.1_ft3

for N in {1..4}; do
    for n in {1..32}; do
        # Sólo ejecutar si el número de procesadores es mayor o igual al
        # número de nodos (no tiene sentido tener menos procesadores que
        # nodos, pues necesariamente habría nodos no utilizados).
        if [ "$n" -ge "$N" ]; then
            echo "Ejecutando con $N nodos y $n procesadores"
            # Ejecuta el comando srun con el valor actual de i
            srun -N$N -n$n montecarlo_mpi 1000000000
        fi
    done
done

echo "Terminado todas las ejecuciones"
