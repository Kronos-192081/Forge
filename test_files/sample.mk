CC = gcc

mak= import "sample2.mk"

# CC --> dsvdv

build: main.o utils.o mak.build
	$(CC) main.o utils.o
	echo "hello world"

main.o: main.c main.h main4.o
	$(pkg-config --cflags --libs gtk+-3.0) gcc
	@heelo 124@k \
	$(CC) -c main.c x:y \
	./a.out x=y

main4.o: main4.c
	$(CC) -c main4.c