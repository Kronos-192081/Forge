C++ = g++ -std=c++20

all: parser.o main.o
	$(C++) main.o parser.o -lpython3.10 -o forge

main.o: main.cpp parser.o graph.hpp coderunner.hpp
	$(C++) -c main.cpp -I/usr/include/python3.10

parser.o: parser.cpp parser.hpp
	$(C++) -c parser.cpp

clean: 
	rm -f parser parser.o main.o forge