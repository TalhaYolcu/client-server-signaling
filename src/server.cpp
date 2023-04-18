#include <iostream>
#include <signal.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <string>
#include "ThreadSafeMap.hpp"
#include "ThreadSafeMap.cpp"

std::atomic_bool running(true);
using namespace std;


void signal_handler(int sig) {
    if (sig == SIGINT) {
        running.store(false);
    }
}

int connect_to_client(char*client_ip_address,char*tokens1,char*tokens2,
    char*client_port_char,int* client_port_int,int* client_socket_fd) {

    //get client ip address
    strncpy(client_ip_address,tokens1,strlen(tokens1));
    
    //get client port as a char
    strncpy(client_port_char,tokens2,strlen(tokens2));
    //get client port as number
    *client_port_int=atoi(client_port_char);

    //create socket and connect to client
    *client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*client_socket_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }

    // Convert the server address from text to binary form
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;   
    client_addr.sin_port=htons(*client_port_int);
    if (inet_pton(AF_INET, client_ip_address, &client_addr.sin_addr) != 1) {
        std::cerr << "Invalid client address" << std::endl;
        return -1;
    }

    // Connect to the server
    if (connect(*client_socket_fd, (struct sockaddr*) &client_addr, sizeof(client_addr)) == -1) {
        std::cerr << "Failed to connect to client" << std::endl;
        return -1;
    }
    return 0;
}

void client_handler(int client_socket) {
    std::cout << "Client connected. Socket: " << client_socket << std::endl;

    // Handle client request here

    char client_ip_address[15];
    char client_port_char[6];
    int client_port_int=-1;
    int client_socket_fd=-1;
    memset(client_ip_address,0,15);
    memset(client_port_char,0,6);
    while(running.load()) {
        // Receive messages from the client
        char buffer[1024];
        memset(buffer,0,1024);
        int num_bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if(num_bytes_received==-1) {
            cout<<"terminating server socket corresponds "<<client_socket<<endl;
            break;
        }

        if(running.load()==false)  {
            char message[256] = "ERR Server shut down\n";
            if (send(client_socket, message, strlen(message), 0) == -1) {
                std::cerr << "Failed to send message to client" << std::endl;
            }     
            break;
        }

        // Print the received message to the console
        std::cout << "Received message from client: " << buffer << std::endl;

        // Split the buffer into tokens separated by spaces
        char* token;
        char* rest = buffer;
        int i = 0;
        char* tokens[100];
        while ((token = strtok_r(rest, " ", &rest))) {
            tokens[i++] = token;
        }
        if(strncmp(tokens[0],"CLIENT",6)==0) {
            cout<<"Client ip address and port received\n";
            if(connect_to_client(client_ip_address,tokens[1],tokens[2]
            ,client_port_char,&client_port_int,&client_socket_fd)!=0) {
                cout<<"Error connecting to the server\n";
                break;
            }
            char message[256] = "Hello from new server\n";
            send(client_socket_fd,message,strlen(message),0);


        }
        else if(strncmp(tokens[0],"SIGNUP",6)==0) {
            cout << "This is register\n";
           

        }
        else if(strncmp(tokens[0],"LOGIN",5)==0) {
            cout<< "This is login\n";
            const string username(tokens[1]);

     
        }
        else if(strncmp(tokens[0],"LOGOUT",6)==0) {
            cout<< "This is logout\n";
           
        }
        else if(strncmp(tokens[0],"EXIT",4)==0) {
            cout <<"This is exit\n";
            char message[256] = "Exit is succesful\n";
            if (send(client_socket, message, strlen(message), 0) == -1) {
                std::cerr << "Failed to send message to server" << std::endl;
                break;
            }              
            std::cout << "Client disconnected. Socket: " << client_socket << std::endl;    
            break;             
        }
    }
    std::cout << "Server closes socket: " << client_socket << std::endl;    
    close(client_socket);

}

int main(int argc, char *argv[]) {
    int port = 12345;

    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }


    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Failed to set socket options" << std::endl;
        return 1;
    }

    // Bind socket to port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind socket" << std::endl;
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_socket, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen for incoming connections" << std::endl;
        return 1;
    }

    // Set signal handler for SIGINT signal
    struct sigaction sig_handler;
    memset(&sig_handler, 0, sizeof(sig_handler));
    sig_handler.sa_handler = signal_handler;
    if (sigaction(SIGINT, &sig_handler, NULL) == -1) {
        std::cerr << "Failed to set signal handler" << std::endl;
        return 1;
    }

    std::cout << "Server started on port " << port << std::endl;


    // Wait for incoming connections
    while (running.load()) {
        // Accept incoming connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_socket == -1) {
            std::cerr << "Failed to accept incoming connection" << std::endl;
            continue;
        }

        // Create a new thread to handle client request
        std::thread t(client_handler, client_socket);
        t.detach();
    }

    // Close server socket
    close(server_socket);

    std::cout << "Server stopped" << std::endl;

    return 0;
}
