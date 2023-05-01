CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -lpthread -pthread -I./include -fPIC
LDFLAGS = -L./lib
LIBS =  -ldatachannel -lsrtp2 -ljuice -lpthread

# Add ThreadSafeMap.cpp to the list of source files
SERVER_SRC = src/server.cpp src/User.cpp src/UserStore.cpp

CLIENT_SRC = src/client.cpp 

# Generate object files from source files
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)

# Generate object files from source files
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

# Build server executable from object files
server: $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LIBS) $^ -o $@ && rm -f $(SERVER_OBJ)
    

client: $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,--no-as-needed $(LIBS) $^ -o $@ && rm -f $(CLIENT_OBJ)



# Compile each source file into an object file
%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object files and executables
clean:
	rm -f $(SERVER_OBJ) server $(CLIENT_OBJ) client 

# Compile all files and build executables
all: server client
   

.PHONY: all clean
