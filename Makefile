UNAME := $(shell uname)
TARGET := bin
F_POS := input_data/
FILES := ./src/chacha.cpp main.cpp ./src/test_client.cpp
CXXFLAGS := -D $(UNAME) -D CHEC$(if $(findstring Y,$(CHECK)),K) -Wall -Wextra -Werror -pedantic -std=c++11 -O3 -march=native -fopenmp
OBJS := $(FILES:.cpp=.o)

ifeq ($(UNAME), Darwin)
	CXX := g++-13
else
	CXX := g++
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(INCLUDES)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDES)

s_run: $(TARGET) check-env check-files
	./$(TARGET) $(LEN) $(F_POS)input_$(LEN).txt

m_run: $(TARGET) check-env
	./$(TARGET) $(LEN)
	@if [ $$? -eq 0 ]; then \
	    printf "\n\033[1;32mRun ended without errors.\033[0m\n\nTo plot result please do:\n\033[1mpython3 ./output_data/graphs.py\033[0m\n\n"; \
	else \
	    printf "\033[1;31mError: The run failed.\033[0m\n"; \
	fi
	
clean:
	rm -fR bin $(OBJS)

clean-all: clean
	rm -fR input_data

check-env:
	@[ -n "$(OMP_NUM_THREADS)" ] printf "\033[1;32mThread number set up correctly.\033[0m\n\n" || (printf "\033[1;33mERROR\033[0m\n\033[33mThe number of threads is not set. Please do: \n\nsource num_thread_definer.sh\033[0m\n\n")

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