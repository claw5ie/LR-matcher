wflags=-Wall -Wextra -pedantic

release: ./src/main.cpp ./src/Grammar.cpp ./src/ItemSets.cpp
	g++ -O3 $^

debug: ./src/main.cpp ./src/Grammar.cpp ./src/ItemSets.cpp
	g++ -std=c++11 $(wflags) -g $^
