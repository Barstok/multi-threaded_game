CC = gcc
CFLAGS = -I.
LIBS = -lncurses -pthread
DEPS = client.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: main.o client.o
	$(CC) -o client main.o client.o $(LIBS)

.PHONY: clean

clean:
	rm client client.o main.o
