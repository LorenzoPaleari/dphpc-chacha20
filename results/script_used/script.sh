#!/bin/bash -l
#SBATCH --job-name="chacha_complete"
#SBATCH --account="g34"
#SBATCH --mail-type=FAIL
#SBATCH --mail-user=lpaleari@ethz.ch
#SBATCH --time=04:00:00
#SBATCH --nodes=1
#SBATCH --nodelist="ault19"
#SBATCH --mem=248GB
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=64
#SBATCH --hint=nomultithread

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export LD_LIBRARY_PATH=$HOME/libsodium-dev/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/users/fviscont/gcc/lib64:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/users/fviscont/mpfr/lib:$LD_LIBRARY_PATH

srun ./stream-script.sh
