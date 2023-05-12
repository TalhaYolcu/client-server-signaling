CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -lpthread -pthread -I./include -fPIC
LDFLAGS = -L./lib
LIBS =  -ldatachannel -lsrtp2 -ljuice -lpthread

# Add ThreadSafeMap.cpp to the list of source files
SERVER_SRC = src/server.cpp src/User.cpp src/UserStore.cpp

CLIENT_SRC = src/client.cpp 

OFFERER_SRC = src/simple_offerer.cpp 

ANSWERER_SRC = src/simple_answerer.cpp 

# Generate object files from source files
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)

# Generate object files from source files
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

# Generate object files from source files
OFFERER_OBJ = $(OFFERER_SRC:.cpp=.o)

# Generate object files from source files
ANSWERER_OBJ = $(ANSWERER_SRC:.cpp=.o)

# Build server executable from object files
server: $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LIBS) $^ -o $@ && rm -f $(SERVER_OBJ)
    

client: $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,--no-as-needed $(LIBS) $^ -o $@ && rm -f $(CLIENT_OBJ)

offer: $(OFFERER_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,--no-as-needed $(LIBS) $^ -o $@ && rm -f $(OFFERER_OBJ)


answer: $(ANSWERER_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,--no-as-needed $(LIBS) $^ -o $@ && rm -f $(ANSWERER_OBJ)



# Compile each source file into an object file
%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object files and executables
clean:
	rm -f $(SERVER_OBJ) server $(CLIENT_OBJ) client $(OFFERER_OBJ) offer $(ANSWERER_OBJ) answer

# Compile all files and build executables
all: server client answer offer
   

.PHONY: all clean
