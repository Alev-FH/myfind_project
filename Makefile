CC = gcc
CFLAGS = -Wall -g

all: myfind

myfind: myfind.c
	$(CC) $(CFLAGS) -o myfind myfind.c

clean:
	rm -f myfind