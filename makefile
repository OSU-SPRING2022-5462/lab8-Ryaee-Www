all: drone8

drone8: drone8.o
	gcc drone8.o -o drone8 -lm

drone8.o: drone8.c
	gcc -ansi -pedantic -g -c -o drone8.o drone8.c

clean:
	rm -rf *.o drone8
