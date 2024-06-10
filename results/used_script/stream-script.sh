#!/bin/bash
#ad STREAM build with AOCC into environment
# NOTE: if you have compiled multiple versions you may need to be more specific
# Spack will complain if your request is ambiguous and could refer to multiple
# packages. (https://spack.readthedocs.io/en/latest/basic_usage.html#ambiguous-specs)

spack load stream %gcc

# Optimize OpenMP performance behavious
export OMP_SCHEDULE=static  # Disable dynamic loop scheduling
export OMP_PROC_BIND=TRUE   # Bind threads to specific resources
export OMP_DYNAMIC=false    # Disable dynamic thread pool sizing

# OMP_PLACES is used for binding OpenMP threads to cores
# See: https://www.openmp.org/spec-html/5.0/openmpse53.html
# For example, a dual socket AMD 4th Gen EPYC™ Processor with 192 (96x2) cores,
# with 4 threads per L3 cache: 96 total places, stride by 2 cores:
export OMP_PLACES={0:64:1}
export OMP_NUM_THREADS=64

# Running stream
stream_c.exe
