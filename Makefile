# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -pedantic -g

# Linker flags
LDFLAGS = -lsimlib -lm

# Source files
SRCS = sim.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Executable name
TARGET = simulation

# Arguments
RIDE = 10
DIA = 5
SWI = 15
OW = 3
RW = 2
PRO = 0

# Default target
all: clean $(TARGET)

run: all
	./$(TARGET) $(RIDE) $(DIA) $(SWI) $(OW) $(RW) $(PRO)

# Link the executable
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)