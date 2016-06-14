
.PHONY: clean

default: client server

CPPFLAGS=-DPORT=\"8081\"
CFLAGS=-std=gnu99 -O3

clean:
	rm -f client server

client: client.c
	gcc $(CPPFLAGS) $(CFLAGS) client.c -o client

server: server.c
	gcc $(CPPFLAGS) $(CFLAGS) server.c -o server
