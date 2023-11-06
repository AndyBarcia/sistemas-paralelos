#!/bin/bash
#----------------------------------------------------
# Generic SLURM script -- MPI Hello World
#----------------------------------------------------
#SBATCH -J myjob             # Job name
#SBATCH -o myjob.%j.out      # stdout; %j expands to jobid
#SBATCH -e myjob.%j.err      # stderr; skip to combine stdout and stderr
#SBATCH -N 8                 # Number of nodes, not cores
#SBATCH -n 128               # Total number of MPI tasks. At least 1 per node.
#SBATCH -t 01:30:00          # max time
#SBATCH --mem=4G             # memory per core demanded

#module load cesga/2020 intel impi
module load cesga/2020 gcc openmpi/4.1.1_ft3

if [ $# -ne 1 ]; then
  echo "Debes indicar qué archivo se va a ejecutar."
fi

for N in {1..8}; do
    for n in {1..32}; do
        # Sólo ejecutar si el número de procesadores es mayor o igual al
        # número de nodos (no tiene sentido tener menos procesadores que
        # nodos, pues necesariamente habría nodos no utilizados).
        if [ "$n" -ge "$N" ]; then
            # Sólo probar múltiples iteraciones si hay 16 iteraciones y 2 nodos.
            # En el resto, probar sólo con 1000000000 de iteraciones.
            if [ "$n" -eq 16 ] && [ "$N" -eq 4 ]; then
                for its in 1000 10000 100000 1000000 10000000 100000000 1000000000 10000000000 100000000000; do
                    echo "Ejecutando con $N nodos, $n procesadores y $its iteraciones"
                    # Ejecuta el comando srun con el valor actual de i
                    srun -N$N -n$n $1 $its
                done
            else
                echo "Ejecutando con $N nodos, $n procesadores y 10000000000 iteraciones"
                # Ejecuta el comando srun con el valor actual de i
                srun -N$N -n$n $1 10000000000
            fi
        fi
    done
done

echo "Terminado todas las ejecuciones"
