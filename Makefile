CC = g++
CFLAGS = -Wall -g

all: myfind

myfind: myfind.cpp
	$(CC) $(CFLAGS) -o myfind myfind.cpp

clean:
	rm -f myfind