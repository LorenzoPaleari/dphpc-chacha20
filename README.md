# ChaCha20 Implementation

## Info
Number of threads modifiable inside [here](./num_thread_definer.sh)

## Current valid commands

#### Single run with specific file:

```
make s_run LEN=YOUR_DATA_SIZE
make s_run LEN=4069 //For example
```

#### Multiple run (starting from 1024 going up by multiplicative factor of 4):
```
make m_run LEN=MAX_DATA_SIZE
make m_run LEN=67108864
//For example this last command will test 1024 4096 16384 ... 67108864
```

#### Clean
```
make clean
make clean-all //Delete also input files
```

#### Others
This commands check if input data is present and if the number of threads has been setted correctly. (This commands are runned by s_run and m_run).
```
make check-env
make check-files
```