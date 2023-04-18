#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <unistd.h>

using namespace std;
string username;
string password;
bool server_disconnected=false;
std::atomic_bool running(true);

void signal_handler(int sig) {
    if (sig == SIGINT) {
        running.store(false);
    }
}

void sign_up_login_user(int client_socket,int option) {

    cout << "Enter username: ";
    cin >> username;
    cout << "Enter password: ";
    cin >> password;

    string state = option==1 ? "SIGNUP" : "LOGIN";

    string register_request = state + " " + username + " " + password;

    // Send a message to the server
    const char* message = register_request.c_str();
    if (send(client_socket, message, strlen(message), 0) == -1) {
        std::cerr << "Failed to send message to server" << std::endl;
        return;
    }

    // Receive a response from the server
    char buffer[1024] = {0};
    if (recv(client_socket, buffer, sizeof(buffer), 0) == -1) {
        std::cerr << "Failed to receive response from server" << std::endl;
        return;
    }
    if(strlen(buffer)==0) {
        server_disconnected=true;
    }

    std::cout << "Server response: " << buffer << std::endl;
   

}

void logout_user(int client_sock,string username) {
    string logout_request = "LOGOUT " + username;

    // Send a message to the server
    const char* message = logout_request.c_str();
    if (send(client_sock, message, strlen(message), 0) == -1) {
        std::cerr << "Failed to send message to server" << std::endl;
        return;
    }

    // Receive a response from the server
    char buffer[1024] = {0};
    if (recv(client_sock, buffer, sizeof(buffer), 0) == -1) {
        std::cerr << "Failed to receive response from server" << std::endl;
        return;
    }
    if(strlen(buffer)==0) {
        server_disconnected=true;
    }

    std::cout << "Server response: " << buffer << std::endl;

}
void exit_user(int client_sock) {

    if(!username.empty()) {
        logout_user(client_sock,username);
    }

    string logout_request = "EXIT EXIT";

    // Send a message to the server
    const char* message = logout_request.c_str();
    if (send(client_sock, message, strlen(message), 0) == -1) {
        std::cerr << "Failed to send message to server" << std::endl;
        return;
    }

    // Receive a response from the server
    char buffer[1024] = {0};
    if (recv(client_sock, buffer, sizeof(buffer), 0) == -1) {
        std::cerr << "Failed to receive response from server" << std::endl;
        return;
    }
    if(strlen(buffer)==0) {
        server_disconnected=true;
    }    

    std::cout << "Server response: " << buffer << std::endl;
}

void open_menu(int client_sock) {
    while(1) {
        int option=-1;
        cout << "1 Sign up new User\n";
        cout << "2 Login user\n";
        cout << "3 Make a call\n";
        cout << "4 Log out\n";
        cout << "5 Exit\n";
        cin >> option;
        if(!running.load()) {
            cout<<"SIGINT Received, exiting from menu\n";
            break;
        }
        if(server_disconnected) {
            cout << "Lost server connection, exiting...\n";
            break;
        }

        switch (option)
        {
        case 1:

        case 2:
            sign_up_login_user(client_sock,option);
            break;
        
        case 4:
            logout_user(client_sock,username);
            break;

        case 5:
            exit_user(client_sock);
            break;

        default:
            break;
        }
        if(server_disconnected) {
            cout << "Lost server connection, exiting...\n";
            break;
        }
        
        if(option==5) {
            cout << "Exiting...\n";
            break;
        }
        
    }
    

    
}

void server_handler(int port) {
    //create socket that handles messages comes from server
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Failed to set socket options" << std::endl;
        return;
    }
    // Bind socket to port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind socket" << std::endl;
        return;
    }

    // Listen for incoming connections
    if (listen(server_socket, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen for incoming connections" << std::endl;
        return;
    }

    // Accept incoming connection
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_socket == -1) {
        std::cerr << "Failed to accept incoming connection" << std::endl;
        return;
    }

    while(running.load()) {
        char buffer[1024] = {0};
        if (recv(client_socket, buffer, sizeof(buffer), 0) == -1) {
            std::cerr << "Failed to receive response from server" << std::endl;
            break;;
        }
        if(strlen(buffer)==0) {
            break;
        }
        if(running.load()==false) {
            break;
        }
        cout<<buffer<<endl;
    }

    close(client_socket);


}

int send_ip_and_port_to_server(const int client_socket,const char*client_address,const int client_port) {
    char message[256];
    memset(message,0,256);
    sprintf(message,"CLIENT %s %d",client_address,client_port);
    return send(client_socket,message,strlen(message),0);

}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <server_ip_address> <server_port> <client_ip_address> <client_port>" << std::endl;
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

    const char* server_address = argv[1];
    const int server_port = std::stoi(argv[2]);
    const char* client_address = argv[3];
    const int client_port = std::stoi(argv[4]);

    // Create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Convert the server address from text to binary form
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_address, &server_addr.sin_addr) != 1) {
        std::cerr << "Invalid server address" << std::endl;
        return 1;
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    
    //send client ip address and port
    if(send_ip_and_port_to_server(client_socket,client_address,client_port)==-1) {
        cout<<"Failed send ip and port to server\n";
        close(client_socket);
        return -1;
    }


    //create listener thread
    std::thread t(server_handler,client_port);

    open_menu(client_socket);

    // Close the socket
    close(client_socket);
    t.join();

    return 0;
}
