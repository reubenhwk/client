

default: client server

client: client.c
	gcc -g client.c -o client

server: server.c
	gcc server.c -o server
