# Highly disciplined compiler variables
CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall

# Target binary name
TARGET = engine

# Default build rule
all:
	$(CXX) $(CXXFLAGS) parser.cpp -o $(TARGET)

# Clean rule to clear build artifacts instantly
clean:
	rm -f $(TARGET)

