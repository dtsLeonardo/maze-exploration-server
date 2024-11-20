all:
	gcc -Wall -c common.c
	gcc -Wall client.c common.o -o bin/client -lm
	gcc -Wall -lpthread server.c common.o -o bin/server -lm