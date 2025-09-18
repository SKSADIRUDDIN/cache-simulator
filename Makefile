CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

all: cache_sim

cache_sim: cache_sim.cpp
	$(CXX) $(CXXFLAGS) -o cache_sim cache_sim.cpp

clean:
	rm -f cache_sim