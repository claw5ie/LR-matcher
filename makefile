all: release_build

WFLAGS=-Wall -Wextra -pedantic

release_build: ./main.cpp ./Grammar.cpp ./ItemSets.cpp
	g++ -O3 -o main.out $^

debug_build: ./main.cpp ./Grammar.cpp ./ItemSets.cpp
	g++ -std=c++11 $(WFLAGS) -g -o main.out $^
