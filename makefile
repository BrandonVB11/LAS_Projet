CC=gcc

CCFLAGS=-D_DEFAULT_SOURCE -D_XOPEN_SOURCE -D_BSD_SOURCE -std=c11 -pedantic -Wvla -Wall -Werror

ALL= server client

all: $(ALL)

server : server.o utils_v1.o network.o ipc.o
	$(CC) $(CCFLAGS) -o server server.o utils_v1.o network.o ipc.o
	
server.o: server.c messages.h
	$(CC) $(CCFLAGS) -c server.c

network.o: network.c network.h
	$(CC) $(CCFLAGS) -c network.c

ipc.o: ipc.c ipc.h
	$(CC) $(CCFLAGS) -c ipc.c

client : client.o utils_v1.o network.o
	$(CC) $(CCFLAGS) -o client client.o utils_v1.o network.o

client.o: client.c messages.h
	$(CC) $(CCFLAGS) -c client.c

utils_v1.o: utils_v1.c utils_v1.h
	$(CC) $(CCFLAGS) -c utils_v1.c

clean:
	rm -f *.o
	rm -f $(ALL)		