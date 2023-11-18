CXXFLAGS := -Wall -Wextra -Werror -pedantic -std=c++11 -O0 -march=native
INCLUDES := 
TARGET := bin
FILES := ./src/chacha.cpp main.cpp
OBJS := $(FILES:.cpp=.o)

all: $(TARGET)

CXX := g++

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)
	
clean:
	rm -fR bin $(OBJS)
