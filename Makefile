UNAME := $(shell uname)
INCLUDES := -I/usr/local/include   # Position of the sodium.h header file
LDFLAGS := -L/usr/local/lib -lsodium   # Position of the libsodium library
TARGET := bin
TARGET_VECT := bin_vect
F_POS := input_data/
FILES := ./src/chacha.cpp main.cpp ./src/test_client.cpp
FILE_CHACHA := ./src/chacha.cpp
FILE_CHACHA_O := ./src/chacha.o 

# Specific configuration for AMD EPYC 7742 (Rome)
#CXXFLAGS_VECT := -D $(UNAME) -Wall -Wextra -pedantic -std=c++11 -fprefetch-loop-arrays --param prefetch-latency=300 -O2 -ffast-math -mfma -mavx2 -march=znver2 -mtune=znver2 -fopenmp
#CXXFLAGS := -D $(UNAME) -Wall -Wextra -pedantic -std=c++11 -O3 -ffast-math -mfma -mavx2 -mavx -march=znver1 -mtune=znver1 -fopenmp

# To modify the target architecture, change the value of the -march and -mtune flags
CXXFLAGS_VECT := -D $(UNAME) -Wall -Wextra -pedantic -std=c++11 -O3 -fprefetch-loop-arrays --param prefetch-latency=300 -ffast-math -mfma -mavx2 -march=skylake -mtune=skylake -fopenmp
CXXFLAGS := -D $(UNAME) -Wall -Wextra -pedantic -std=c++11 -O3 -ffast-math -mfma -mavx2 -march=skylake -mtune=skylake -fopenmp
OBJS := $(FILES:.cpp=.o)
OBJS_VECT := $(FILES:.cpp=_vect.o)
VER ?= 0

ifeq ($(UNAME), Darwin)
	CXX := g++-13
else
# Used on cluster - needed updated version of gcc
# CXX_VECT := /users/fviscont/gcc/bin/g++
	CXX := g++
endif

all: $(TARGET) $(TARGET_VECT)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(CXXFLAGS) $(LDFLAGS) $(INCLUDES)

$(TARGET_VECT): $(OBJS_VECT)
	$(CXX) $(OBJS_VECT) -o $(TARGET_VECT) $(CXXFLAGS_VECT) $(LDFLAGS) $(INCLUDES)
# Used on cluster - needed updated version of gcc
#$(CXX_VECT) $(OBJS_VECT) -o $(TARGET_VECT) $(CXXFLAGS_VECT) $(LDFLAGS) $(INCLUDES)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDES)

%_vect.o: %.cpp
	$(CXX) $(CXXFLAGS_VECT) -c $< -o $@ $(INCLUDES)
# Used on cluster - needed updated version of gcc
#$(CXX_VECT) $(CXXFLAGS_VECT) -c $< -o $@ $(INCLUDES)

s_run: compile-base check-env check-files
	./$(TARGET) $(LEN) $(F_POS)input_$(LEN).txt $(VER)

s_run-vect: compile-vect check-env check-files
	./$(TARGET_VECT) $(LEN) $(F_POS)input_$(LEN).txt $(VER)

m_run: compile-base check-env
	./$(TARGET) $(LEN)
	@if [ $$? -eq 0 ]; then \
	    printf "\n\033[1;32mRun ended without errors.\033[0m\033[1m\n\nTo plot results:\n\n      make plot\n\n\033[0m"; \
	else \
	    printf "\033[1;31mError: The run failed.\033[0m\n"; exit 1; \
	fi

m_run-vect: compile-vect check-env
	./$(TARGET_VECT) $(LEN)
	@if [ $$? -eq 0 ]; then \
	    printf "\n\033[1;32mRun ended without errors.\033[0m\033[1m\n\nTo plot results:\n\n      make plot\n\n\033[0m"; \
	else \
	    printf "\033[1;31mError: The run failed.\033[0m\n"; exit 1; \
	fi

compile-all: $(TARGET) $(TARGET_VECT)
	@if [ $$? -eq 0 ]; then \
	    printf "\n\033[1;32mCompilation ended without errors.\033[0m\n\n"; \
	else \
	    printf "\033[1;31mError: The compilation failed.\033[0m\n"; exit 1; \
	fi

compile-vect: $(TARGET_VECT)
	@if [ $$? -eq 0 ]; then \
        	printf "\n\033[1;32mCompilation ended without errors.\033[0m\n\n"; \
	else \
		printf "\033[1;31mError: The compilation failed.\033[0m\n"; exit 1; \
	fi

compile-base: $(TARGET)
	@if [ $$? -eq 0 ]; then \
		printf "\n\033[1;32mCompilation ended without errors.\033[0m\n\n"; \
	else \
		printf "\033[1;31mError: The compilation failed.\033[0m\n"; exit 1; \
	fi

plot:
	@python3 -m venv venv; . ./venv/bin/activate; pip install --upgrade -q pip; pip install -q matplotlib; python3 ./output_data/graphs.py;
	@if [ $$? -eq 0 ]; then \
	    printf "\r\033[1;32mResults Plotted.    \033[0m\n\n"; \
	else \
	    printf "\033[1;31mError plotting results\033[0m\n"; exit 1; \
	fi

callgrind: compile check-env check-files
	mkdir -p callgrind
	valgrind --tool=callgrind --simulate-cache=yes --callgrind-out-file=callgrind/callgrind_$(LEN)_opt$(VER) ./$(TARGET) $(LEN) $(F_POS)input_$(LEN).txt $(VER)

valgrind: compile check-env check-files
	valgrind --leak-check=yes --track-origins=yes ./$(TARGET) $(LEN) $(F_POS)input_$(LEN).txt $(VER)

check-no-inline: clean
	$(CXX) $(CXXFLAGS) -fopt-info-inline-missed -c $(FILE_CHACHA) -o $(FILE_CHACHA_O)  $(INCLUDES)

check-no-vectorizazion: clean
	$(CXX) $(CXXFLAGS) -fopt-info-vec-missed -c $(FILE_CHACHA) -o $(FILE_CHACHA_O)  $(INCLUDES)

check-no-loop-unrolling: clean
	$(CXX) $(CXXFLAGS) -fopt-info-loop-missed -c $(FILE_CHACHA) -o $(FILE_CHACHA_O)  $(INCLUDES)

clean:
	rm -fR bin bin_vect $(OBJS) $(OBJS_VECT)

clean-all: clean
	rm -fR input_data bin.dSYM

check-env:
	@if [ -n "$(OMP_NUM_THREADS)" ]; then \
		printf "\033[1;32mThread number set up correctly.\033[0m\n\n"; \
	else \
		printf "\033[1;33mERROR\033[0m\n\033[33mThe number of threads is not set. Please do: \n\nsource num_thread_definer.sh\033[0m\n\n"; \
	fi

check-files:
	@MISSING_FILE=$(if $(wildcard $(F_POS)input_$(LEN).txt),,$(LEN)); \
	MISSING_COUNT=`echo "$$MISSING_FILE" | awk '{print NF}'`; \
	if [ $$MISSING_COUNT -gt 0 ]; then \
    	printf "\033[1;33mMissing file!\033[0m\n"; \
    	printf "Generating input data...\n"; \
    	mkdir -p $(F_POS); \
    	base64 -i /dev/urandom | head -c $(LEN) > $(F_POS)input_$(LEN).txt; \
		printf "\033[1;32mDone\033[0m\n\n"; \
	else \
    	printf "\033[1;32mInput file is present.\033[0m\n\n"; \
	fi
