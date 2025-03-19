var2 = gg
mak2 = import "sample3.mk"

build: main1.o utils1.o mak2.run mak2.run2 run
	gcc main1.o utils1.o
	echo "hello world2"

run: main.c mak2.main4.o
	gcc main.c
	./a.out