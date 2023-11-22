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

run: $(TARGET)
	./$(TARGET) $(LEN) $(F_POS)input_$(LEN).txt
	
clean:
	rm -fR bin $(OBJS)
