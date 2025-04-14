C++ = g++ -std=c++20

forge: parser.o main.o
	$(C++) main.o parser.o -lpython3.10 -lsqlite3 -lssl -lcrypto -pthread -o forge

main.o: main.cpp parser.o graph.hpp coderunner.hpp cache.hpp
	$(C++) -c main.cpp -I/usr/include/python3.10

parser.o: parser.cpp parser.hpp
	$(C++) -c parser.cpp

install: forge
	sudo cp forge /usr/local/bin

uninstall:
	sudo rm -f /usr/local/bin/forge

.PHONY: clean install uninstall

clean: 
	rm -f parser parser.o main.o forge