run: main3.o utils3.o
	gcc main3.o utils3.o
	echo "hello world3"

run2: main4.o utils3.o
	gcc main4.o utils3.o
	echo "hello world3"
	
main4.o: main4.c utils3.o
	gcc -c main4.c