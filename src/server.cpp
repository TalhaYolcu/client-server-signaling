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
#include <vector>
#include "UserStore.cpp"
#include "User.cpp"
#include "util.hpp"

std::atomic_bool running(true);
using namespace std;

UserStore userStore;

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
    std::string username;
    std::string password;
    bool haslogin=false;
    while(running.load()) {
        // Receive messages from the client
        char buffer[SOCKET_BUFFER_SIZE];
        memset(buffer,0,SOCKET_BUFFER_SIZE);
        int num_bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if(num_bytes_received<=-0) {
            cout<<"terminating server socket corresponds "<<client_socket<<endl;
            break;
        }

        if(running.load()==false)  {
            /*char message[256] = "ERR Server shut down\n";
            if (send(client_socket, message, strlen(message), 0) == -1) {
                std::cerr << "Failed to send message to client" << std::endl;
            } */    
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
            char message[SOCKET_BUFFER_SIZE] = "Hello from new server\n";
            send(client_socket_fd,message,sizeof(message),0);


        }
        else if(strncmp(tokens[0],"SIGNUP",6)==0) {
            cout << "This is register\n";
            username=string(tokens[1]);
            password=string(tokens[2]);

            userStore.addUser(User(username,password,false,client_socket_fd));

            char message[SOCKET_BUFFER_SIZE] = "Signup is successful\n";
            send(client_socket,message,sizeof(message),0);            

        }
        else if(strncmp(tokens[0],"LOGIN",5)==0) {
            cout<< "This is login\n";
            username=string(tokens[1]);
            password=string(tokens[2]);
            
            userStore.loginUser(username,password);

            char message[SOCKET_BUFFER_SIZE] = "Login is successful\n";
            send(client_socket,message,sizeof(message),0);    
     
        }

        else if(strncmp(tokens[0],"CALL",4)==0) {
            cout<<"This is call\n";
            string receiver_username=string(tokens[1]);

            if(!userStore.isLogin(receiver_username)) {
                //user is not login, send error message to client
            }
            else if(!userStore.hasUser(receiver_username)) {
                //user not found, send error message to client
            }
            else {
                //take senders sdp
                string first_peer_sdp=string(tokens[2]);

                //send to receiver
                try {
                    User receiverUser=userStore.getUser(receiver_username);
                    int receiver_fd=receiverUser.getSocketFd();
                    
                    char message[SOCKET_BUFFER_SIZE];
                    memset(message,0,SOCKET_BUFFER_SIZE);

                    sprintf(message,"INVITE %s %s",username.c_str(),first_peer_sdp.c_str());

                    send(receiver_fd,message,sizeof(message),0);

                    //wait receiver to send its sdp

                    char incoming_message[SOCKET_BUFFER_SIZE];
                    memset(incoming_message,0,SOCKET_BUFFER_SIZE);

                    recv(receiver_fd,incoming_message,sizeof(incoming_message),0);
                    
                    char response_to_sender[SOCKET_BUFFER_SIZE]={0};

                    strncpy(response_to_sender,incoming_message,strlen(incoming_message));

                    cout<<"Receiver's sdp:\n"<<endl;
                    cout<<incoming_message<<endl;

                    char* receiver_token;
                    char* receiver_rest = incoming_message;
                    int i = 0;
                    char* receiver_tokens[100];
                    while ((receiver_token = strtok_r(receiver_rest, " ", &receiver_rest))) {
                        receiver_tokens[i++] = receiver_token;
                    }

                    if(strncmp(receiver_tokens[0],"ACCEPT",6)==0) {
                        //receiver accepted request
                        char receiver_sdp[SOCKET_BUFFER_SIZE];
                        memset(receiver_sdp,0,SOCKET_BUFFER_SIZE);

                        //take receivers sdp
                        strncpy(receiver_sdp,receiver_tokens[1],strlen(receiver_tokens[1]));

                        cout<<"Forwarding receivers sdp to sender:\n";
                        cout<<response_to_sender<<endl;
                        //forward it to first sender

                        
                        //send(client_socket_fd,response_to_sender,sizeof(response_to_sender),0);


                    }
                    else {
                        //user declined request
                    }
                            

                    
                }
                catch(std::runtime_error err) {
                    err.what();
                }
                
            }
        }
        else if(strncmp(tokens[0],"LOGOUT",6)==0) {
            cout<< "This is logout\n";

            userStore.logOutUser(username);

            char message[SOCKET_BUFFER_SIZE] = "Logout is successful\n";
            send(client_socket,message,sizeof(message),0);             
        }
        else if(strncmp(tokens[0],"EXIT",4)==0) {
            cout <<"This is exit\n";
            char message[SOCKET_BUFFER_SIZE] = "Exit is succesful\n";
            if (send(client_socket, message, sizeof(message), 0) == -1) {
                std::cerr << "Failed to send message to server" << std::endl;
                break;
            }              
            std::cout << "Client disconnected. Socket: " << client_socket << std::endl;    
            if (send(client_socket_fd, "EXIT", sizeof("EXIT"), 0) == -1) {
                std::cerr << "Failed to send message to server" << std::endl;
                break;
            }                 
            break;             
        }
    }
    std::cout << "Server closes socket: " << client_socket << std::endl;    
    close(client_socket);
    if(client_socket_fd!=-1) {
        close(client_socket);
    }

}

int main(int argc, char *argv[]) {

    if(argc!=2) {
        std::cerr << "Usage: " << argv[0] << " <server_port>" << std::endl;
        return 1;        
    }

    int port = std::stoi(argv[1]);

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

    std::vector<std::thread> threads;

    // Wait for incoming connections
    while (running.load()) {
        // Accept incoming connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        
        if(!running.load()) {
            break;
        }

        if (client_socket == -1) {
            std::cerr << "Failed to accept incoming connection" << std::endl;
            break;;
        }

        // Create a new thread to handle client request       
        threads.push_back( std::thread (client_handler, client_socket));
    }

    for (auto& t : threads) {
        t.join();
    }

    // Close server socket
    close(server_socket);



    std::cout << "Server stopped" << std::endl;

    return 0;
}
