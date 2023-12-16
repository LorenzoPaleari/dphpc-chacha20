# ChaCha20 Implementation

## Info
Number of threads modifiable inside [here](./num_thread_definer.sh)

## Current valid commands

#### Single run with specific file:

```
make s_run LEN=YOUR_DATA_SIZE VER=OPTIMIZATION_VERSION (default to 0)
make s_run LEN=4069 //For example
```

#### Multiple run (starting from 1024 going up by multiplicative factor of 4):
```
make m_run LEN=MAX_DATA_SIZE
make m_run LEN=67108864
//For example this last command will test 1024 4096 16384 ... 67108864
```

#### Valgrind
It will run s_run with valgrind
```
make valgrind LEN=YOUR_DATA_SIZE VER=OPTIMIZATION_VERSION (default to 0)
make valgrind LEN=4096 VER=1
```

#### Callgrind (Valgrind tool)
It will run s_run with callgrind
```
make callgrind LEN=YOUR_DATA_SIZE VER=OPTIMIZATION_VERSION (default to 0)
make callgrind LEN=4096 VER=1
```

#### Check -03 optimizations missed
It will run s_run with callgrind
```
make check-no-inline
make check-no-vectorizazion
make check-no-loop-unrolling
```

#### Clean
```
make clean
make clean-all //Delete also input files
```

#### Others
This commands check if input data is present and if the number of threads has been setted correctly. (This commands are runned by s_run, m_run, callgrind and valgrind).
```
make check-env
make check-files
```