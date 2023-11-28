CXXFLAGS := -Wall -Wextra -Werror -pedantic -std=c++11 -O3 -march=native -fopenmp
INCLUDES := 
TARGET := bin
FILES := ./src/chacha.cpp main.cpp ./src/test_client.cpp
OBJS := $(FILES:.cpp=.o)
F_POS := input_data/

all: $(TARGET)

CXX := g++-13

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(INCLUDES)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDES)

s_run: $(TARGET) check-env
	./$(TARGET) $(LEN) $(F_POS)input_$(LEN).txt

m_run: $(TARGET) check-env check-files
	./$(TARGET) $(LEN)
	@if [ $$? -eq 0 ]; then \
	    printf "\n\033[1;32mRun ended without errors.\033[0m\n\nTo plot result please do:\n\033[1mpython3 ./output_data/plots.py\033[0m\n\n"; \
	else \
	    printf "\033[1;31mError: The run failed.\033[0m\n"; \
	fi
	
clean:
	rm -fR bin $(OBJS)

clean-all: clean
	rm -fR input_data

check-env:
	@[ -n "$(OMP_NUM_THREADS)" ] || (printf "\033[1;31mERROR\033[0m\n\033[31mThe number of threads is not set. Please do: \n\nsource num_thread_definer.sh\033[0m\n\n"; exit 1)
	@printf "\033[1;32mThread number set up correctly.\033[0m\n\n"

FILENAMES := 1024 4096 16384 65536 262144 1048576 4194304 16777216 67108864 268435456 1073741824 4294967296
MISSING_FILES := $(foreach file,$(FILENAMES),$(if $(wildcard $(F_POS)input_$(file).txt),,$(file)))

check-files:
	@MISSING_COUNT=`echo "$(MISSING_FILES)" | awk '{print NF}'`; \
	if [ $$MISSING_COUNT -gt 0 ]; then \
    	printf "\033[1;33mMissing files: $(MISSING_FILES)\033[0m\n\n"; \
    	source gen_test_data.sh; \
		printf "\n\n"; \
	else \
    	printf "\033[1;32mAll input files are present.\033[0m\n\n"; \
	fi