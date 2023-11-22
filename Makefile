CXXFLAGS := -Wall -Wextra -Werror -pedantic -std=c++11 -O3 -march=native -fopenmp
INCLUDES := 
TARGET := bin
FILES := ./src/chacha.cpp main.cpp ./src/test_client.cpp
OBJS := $(FILES:.cpp=.o)
F_NAME := input.txt

all: $(TARGET)

CXX := g++-13

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(INCLUDES)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDES)

run: $(TARGET)
	base64 -i /dev/urandom | head -c $(LEN) > $(F_NAME)
	export OMP_NUM_THREADS=$(NUM_T)
	./$(TARGET) $(LEN) $(F_NAME)
	
clean:
	rm -fR bin $(OBJS)
