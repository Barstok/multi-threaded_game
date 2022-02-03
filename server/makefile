CC = gcc
CFLAGS = -I.
LIBS = -lncurses -pthread
DEPS = server.h const.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: main.o server.o
	$(CC) -o server main.o server.o $(LIBS)

.PHONY: clean

clean:
	rm server server.o main.o

