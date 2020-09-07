CC = gcc
CFLAGS = -Wall
SERVER = server.out
CLIENT = client.out

SERVER_OBJS = AuthenticationService.o ClientHandler.o FileChecksum.o Protocol.o StorageService.o md5.o
CLIENT_OBJS = FileChecksum.o Protocol.o StorageService.o md5.o

# compile object file from corresponding .c and .h file
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

# build everything
all: server client

# build only the server
server: $(SERVER)
$(SERVER): Server.c  $(SERVER_OBJS) NetworkHeader.h
	$(CC) $(CFLAGS) Server.c $(SERVER_OBJS) -o $@

# build only the client
client: $(CLIENT)
$(CLIENT): Client.c $(CLIENT_OBJS) NetworkHeader.h
	$(CC) $(CFLAGS) Client.c $(CLIENT_OBJS) -o $@

clean:
	-rm -f *.o *.out $(SERVER) $(CLIENT)
	-rm -r serverdata/
	-rm -r clientdata/
