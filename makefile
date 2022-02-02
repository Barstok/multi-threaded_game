CC = gcc
LIBS = -lncurses -pthread


server: main.o server.o
	$(CC) -o server main.o server.o $(LIBS)

.PHONY: clean

clean:
	rm server server.o main.o

