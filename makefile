CXX     = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -Iinclude

# All .cpp files in src/ + main.cpp
SRCS = main.cpp \
       src/ALU.cpp \
       src/CPU.cpp \
       src/Decoder.cpp \
       src/Memory.cpp \
       src/RegisterFile.cpp \
       src/HazardDetector.cpp \
       src/ForwardingUnit.cpp

TARGET = simulator

# Default target — build the simulator
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

run: $(TARGET)
	./$(TARGET) $(FILE)

# Remove compiled binary
clean:
	rm -f $(TARGET)

.PHONY: all run clean