
.PHONY: clean

default: client server

clean:
	rm -f client server

client: client.c
	gcc -std=gnu99 -g client.c -o client

server: server.c
	gcc -std=gnu99 -g server.c -o server
