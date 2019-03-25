
all:
	gcc server.c -o server -lsqlite3 -lpthread
	gcc client.c -o client
clean:
	rm server client
	
