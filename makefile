ALL= server client

all: $(ALL)

server : server.o utils_v1.o 
	$(CC) $(CCFLAGS) -o server server.o utils_v1.o
	
server.o: server.c message.h ipc_conf.h
	$(CC) $(CCFLAGS) -c server.c

client : client.o utils_v1.o
	$(CC) $(CCFLAGS) -o client client.o utils_v1.o

client.o: client.c message.h
	$(CC) $(CCFLAGS) -c client.c

utils_v1.o: utils_v1.c utils_v1.h
	$(CC) $(CCFLAGS) -c utils_v1.c

clean:
	rm -f *.o
	rm -f $(ALL)		