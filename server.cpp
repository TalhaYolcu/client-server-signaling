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

std::atomic_bool running(true);
using namespace std;

map<string, string> user_pass_map;
map<string, bool> login_states; 



void signal_handler(int sig) {
    if (sig == SIGINT) {
        running.store(false);
    }
}

void client_handler(int client_socket) {
    std::cout << "Client connected. Socket: " << client_socket << std::endl;

    // Handle client request here


    while(running.load()) {
        // Receive messages from the client
        char buffer[1024];
        memset(buffer,0,1024);
        int num_bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if(num_bytes_received==-1) {
            cout<<"terminating server socket corresponds "<<client_socket<<endl;
            close(client_socket);
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

        if(strncmp(tokens[0],"SIGNUP",6)==0) {
            cout << "This is register\n";
            string username(tokens[1]);
            string password(tokens[2]);
            auto it = user_pass_map.find(username);

            if (it != user_pass_map.end()) {
                // Key was found in the map
                char message[256] = "User already exists\n";
                if (send(client_socket, message, strlen(message), 0) == -1) {
                    std::cerr << "Failed to send message to server" << std::endl;
                    return;
                }

            } else {
                // Key was not found in the map
                user_pass_map[username]=password;
                char message[256] = "Sign up is succesful\n";
                if (send(client_socket, message, strlen(message), 0) == -1) {
                    std::cerr << "Failed to send message to server" << std::endl;
                    return;
                }
            }            

        }
        else if(strncmp(tokens[0],"LOGIN",5)==0) {
            cout<< "This is login\n";
            string username(tokens[1]);
            auto it = user_pass_map.find(username);
            if (it != user_pass_map.end()) {
                // Key was found in the map
                login_states[username]=true;
                char message[256] = "Login is succesful\n";
                if (send(client_socket, message, strlen(message), 0) == -1) {
                    std::cerr << "Failed to send message to server" << std::endl;
                    return;
                }                   

            } else {
                // Key was not found in the map
                cout << "User not found" << endl;
                char message[256] = "User not found\n";
                if (send(client_socket, message, strlen(message), 0) == -1) {
                    std::cerr << "Failed to send message to server" << std::endl;
                    return;
                }                  
            }
     
        }
        else if(strncmp(tokens[0],"LOGOUT",6)==0) {
            cout<< "This is logout\n";
            // Close client connection
            string username(tokens[1]);
            
            login_states[username]=false;
            char message[256] = "Logout is succesful\n";
            if (send(client_socket, message, strlen(message), 0) == -1) {
                std::cerr << "Failed to send message to server" << std::endl;
                return;
            }            
        }
        else if(strncmp(tokens[0],"EXIT",4)==0) {
            cout <<"This is exit\n";
            char message[256] = "Exit is succesful\n";
            if (send(client_socket, message, strlen(message), 0) == -1) {
                std::cerr << "Failed to send message to server" << std::endl;
                return;
            }              
            close(client_socket);
            std::cout << "Client disconnected. Socket: " << client_socket << std::endl;    
            break;             
        }
    }
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
