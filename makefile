wflags=-Wall -Wextra -pedantic

release: ./main.cpp ./src/Grammar.cpp ./src/ItemSets.cpp
	g++ -O3 $^

debug: ./main.cpp ./src/Grammar.cpp ./src/ItemSets.cpp
	g++ -std=c++11 $(wflags) -g $^
