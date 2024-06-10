# High-Performance and Parallel ChaCha20
This project showcases a highly efficient and parallel C++ implementation of ChaCha20, a symmetric key stream cipher recognized as the only alternative to AES in TLS v1.3 (Transport Layer Security).

### Authors
 - [Alessandro Giaconia](https://github.com/alecsino)
 - [Lorenzo Paleari](https://github.com/LorenzoPaleari)

## Project Overview
 Authenticated Encryption project, developed for the course Design of Parallel and High-Performance Computing at ETH Zürich during the 2023/2024 academic year.

This implementation is part of the [`Authenticated Encryption`](https://github.com/filippovisconti/dphpc-project)  project, developed for the course `Design of Parallel and High-Performance Computing at ETH Zürich during the 2023/2024 academic year`.

For detailed results and analyses, please refer to our our [`report`](./results/report.pdf).

### Anonymous Reviews
- The report is very good – it is well written overall, with very good structure and a good discussion of the results. However, I believe a more thorough analysis of for example the discontinuity in Figure 4 would have been warranted. In general, more variation in the experimental design would have also been welcome. Going for a paper from here seems possible.

- Very good report, easy to follow, and extensive evaluation under different scenarios. The single-thread scalability results are impressive.

- Very good report with results that beat the state-of-the-art for their corresponding algorithms. Easy to follow with extensive evaluation. Some small editorial changes would be welcome. The results are explained in many places. However, there is no comparison between other algorithms like AES so it's hard to position the results. There is also no evaluation of combining the encryption and authentication parts.

## Reproduction Steps
To execute the project correctly, ensure you have a working installation of g++ 13 and the OpenMP library.

**Note for macOS users:** The default C/C++ compiler, clang, does not support some of the flags used in this project. It is recommended to install a full GNU compiler.

Depending on your operating system, the OpenMP library might already be installed, or you may need to install it separately.

You may need to adjust paths in the [`Makefile`](./Makefile)

### Setting number of threads
To modify the number of threads, adjust the settings in [`num_thread_definer.sh`](./num_thread_definer.sh), then execute one of the following commands:
```bash
source num_thread_definer.sh

# Alternative
. ./num_thread_definer.sh
```

### Code execution

The standard code execution will test various numbers of threads, starting from 1 and doubling each time until the specified maximum number of threads is reached. For each thread count, the data size will be tested starting from 1MB, increasing in size witha  factor of 2 until the maximum lenght is reached.

To compile and run the code, use the following commands:

```bash
make compile-all

# Choose one between standard and vectorized, the latter is faster.

# Standard Execution
make m_run LEN=<MAX_DATA_SIZE>
# Vectorized Execution
make m_run-vect LEN=<MAX_DATA_SIZE>

# Example execution with a data size greater than 128MB
make m_run LEN=200000000 # > 128MB
# This command will test 1MB 2MB 4MB 8MB 16MB 32MB 64MB 128MB
```



## Complete Commands List

#### Compilation

```bash
make compile-base
make compile-vect

# Will compile all the above
make compile-all
```

#### Single run with specific file:
Default version is 0.
```bash
# Standard
make s_run LEN=<YOUR_DATA_SIZE> VER=<OPTIMIZATION_VERSION>
# Vectorized
make s_run-vect LEN=<YOUR_DATA_SIZE> VER=<OPTIMIZATION_VERSION>

# Example
make s_run LEN=4069
```

#### Multiple run
Start from 1MB increasing dimension with a 2x factor until reaching the specified number.
```bash
# Standard
make m_run LEN=<MAX_DATA_SIZE>
# Vectorized
make m_run-vect LEN=<MAX_DATA_SIZE>

# Example
make m_run LEN=200000000 # > 128MB
# This command will test 1MB 2MB 4MB 8MB 16MB 32MB 64MB 128MB
```

#### Plotting Results
It is reccommended to execute after a Multiple Run.
```bash
make plot
```

#### Valgrind
It will execute s_run with valgrind
```bash
make valgrind LEN=<YOUR_DATA_SIZE> VER=<OPTIMIZATION_VERSION>

# Example
make valgrind LEN=4096 VER=1
```

#### Callgrind (Valgrind tool)
It will execute s_run with callgrind
```bash
make callgrind LEN=<YOUR_DATA_SIZE> VER=<OPTIMIZATION_VERSION>

# Example
make callgrind LEN=4096 VER=1
```

#### Check -03 optimizations missed
It will compile the code with special flags to detect which optimizations the compiler is unable to achieve.
```bash
make check-no-inline
make check-no-vectorizazion
make check-no-loop-unrolling
```

#### Clean
```bash
make clean
make clean-all # Delete also input files
```

#### Others
This commands check if input data is present and if the number of threads has been setted correctly.

Note: This commands are executed by s_run, m_run, callgrind and valgrind.
```bash
make check-env
make check-files
```