g++ -std=c++17 -Wall -Iinclude main.cpp src/Memory.cpp src/RegisterFile.cpp src/ALU.cpp src/Decoder.cpp src/CPU.cpp src/HazardDetector.cpp src/ForwardingUnit.cpp -o simulator 2>&1 | more
pause