#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;
    string username;
    string password;


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
        default:
            break;
        }
        
        if(option==5) {
            cout << "Exiting...\n";
            break;
        }
        
    }
    

    
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip_address> <port>" << std::endl;
        return 1;
    }

    const char* server_address = argv[1];
    const int server_port = std::stoi(argv[2]);

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


    open_menu(client_socket);



    // Close the socket
    close(client_socket);

    return 0;
}
