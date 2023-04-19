CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -lpthread -I./include -fPIC
LDFLAGS = -L/usr/local/lib
LIBS =  -ldatachannel -lsrtp2 -ljuice
DISTFILES += \
    lib/unix/libdatachannel.so \
    lib/unix/libdatachannel.so.0.18.3 \
    lib/unix/libjuice-static.a \
    lib/unix/libjuice.so \
    lib/unix/libjuice.so.1.2.2 \
    lib/unix/libsrtp2.a \
    lib/unix/libusrsctp.a \
    lib/unix/libsrtp2.so \

INCLUDEPATH += $$PWD/include/rtc
DEPENDPATH += $$PWD/include/rtc


# Add ThreadSafeMap.cpp to the list of source files
SERVER_SRC = src/server.cpp src/User.cpp src/UserStore.cpp

CLIENT_SRC = src/client.cpp 

# Generate object files from source files
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)

# Generate object files from source files
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

# Build server executable from object files
server: $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LIBS) $^ -o $@

client: $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,--no-as-needed $(LIBS) $^ -o $@



# Compile each source file into an object file
%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object files and executables
clean:
	rm -f $(SERVER_OBJ) server $(CLIENT_OBJ) client 

# Compile all files and build executables
all: server client

.PHONY: all clean
