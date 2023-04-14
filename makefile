# Compiler
CC = g++

# Compiler flags
CFLAGS = -Wall -std=c++11

# Server executable
SERVER = server

# Client executable
CLIENT = client

# Source files
SERVER_SRC = server.cpp
CLIENT_SRC = client.cpp

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC) -lpthread

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)

clean:
	rm -f $(SERVER) $(CLIENT)