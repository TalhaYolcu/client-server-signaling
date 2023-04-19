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
#include "rtc/rtc.hpp"
#include "rtc/peerconnection.hpp"
#include <nlohmann/json.hpp>
#include "util.hpp"

using namespace std;
using nlohmann::json;



string username;
string password;
bool server_disconnected=false;
std::atomic_bool running(true);
bool exit_selected=false;
string local_sdp;
bool isLogin=false;

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
    char message[SOCKET_BUFFER_SIZE];
    memset(message,0,SOCKET_BUFFER_SIZE);
    strncpy(message,register_request.c_str(),strlen(register_request.c_str()));
    if (send(client_socket, message, sizeof(message), 0) == -1) {
        std::cerr << "Failed to send message to server" << std::endl;
        return;
    }

    // Receive a response from the server
    char buffer[SOCKET_BUFFER_SIZE] = {0};
    if (recv(client_socket, buffer, sizeof(buffer), 0) == -1) {
        std::cerr << "Failed to receive response from server" << std::endl;
        return;
    }
    if(strlen(buffer)==0) {
        server_disconnected=true;
    }

    std::cout << "Server response: " << buffer << std::endl;
    exit_selected=false;
    isLogin=true;

}

void logout_user(int client_sock,string username) {
    string logout_request = "LOGOUT " + username;

    // Send a message to the server
    char message[SOCKET_BUFFER_SIZE];
    memset(message,0,SOCKET_BUFFER_SIZE);
    strncpy(message,logout_request.c_str(),strlen(logout_request.c_str()));
    if (send(client_sock, message, sizeof(message), 0) == -1) {
        std::cerr << "Failed to send message to server" << std::endl;
        return;
    }

    // Receive a response from the server
    char buffer[SOCKET_BUFFER_SIZE] = {0};
    if (recv(client_sock, buffer, sizeof(buffer), 0) == -1) {
        std::cerr << "Failed to receive response from server" << std::endl;
        return;
    }
    if(strlen(buffer)==0) {
        server_disconnected=true;
    }

    std::cout << "Server response: " << buffer << std::endl;
    isLogin=false;

}
void exit_user(int client_sock) {

    if(!username.empty()) {
        logout_user(client_sock,username);
    }

    string logout_request = "EXIT EXIT";

    // Send a message to the server
    char message[SOCKET_BUFFER_SIZE];
    memset(message,0,SOCKET_BUFFER_SIZE);
    strncpy(message,logout_request.c_str(),strlen(logout_request.c_str()));
    if (send(client_sock, message, sizeof(message), 0) == -1) {
        std::cerr << "Failed to send message to server" << std::endl;
        return;
    }

    // Receive a response from the server
    char buffer[SOCKET_BUFFER_SIZE] = {0};
    if (recv(client_sock, buffer, sizeof(buffer), 0) == -1) {
        std::cerr << "Failed to receive response from server" << std::endl;
        return;
    }
    if(strlen(buffer)==0) {
        server_disconnected=true;
    }    

    std::cout << "Server response: " << buffer << std::endl;
    exit_selected=true;
}

void call_peer(int client_sock) {
    char peer_username[50];
    memset(peer_username,0,50);

    cout<<"Enter username to call"<<endl;
    cin>>peer_username;

    // Send a message to the server
    char message[SOCKET_BUFFER_SIZE];
    memset(message,0,SOCKET_BUFFER_SIZE);
    sprintf(message,"CALL %s %s",peer_username,local_sdp.c_str());
    if (send(client_sock, message, sizeof(message), 0) == -1) {
        std::cerr << "Failed to send message to server" << std::endl;
        return;
    }    

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
            if(!exit_selected) {
                exit(client_sock);
            }
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
        
        case 3:
            if(isLogin) {
                call_peer(client_sock);
            }
            else {
                cout<<"Please login first\n"<<endl;
            }
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
            pid_t pid = getpid();

            // Send the SIGINT signal to the current process
            int result = kill(pid, SIGINT);       
            if(result==-1) {
                cout<<"Error to send kill yourself\n";
            }     
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

    if(!running.load()) {
        cout<<"SIGINT received"<<endl;
        return;
    }

    if (client_socket == -1) {
        std::cerr << "Failed to accept incoming connection" << std::endl;
        return;
    }

    while(running.load()) {
        char buffer[SOCKET_BUFFER_SIZE] = {0};
        if (recv(client_socket, buffer, sizeof(buffer), 0) == -1) {
            std::cerr << "Failed to receive response from server" << std::endl;
            break;;
        }

        if(strlen(buffer)==0) {
            break;
        }
        
        if(strncmp(buffer,"EXIT",4)==0) {
            cout<<"Exiting...\n";
            break;
        }

        else if(strncmp(buffer,"INVITE",6)==0) {
            
            //received a call
            char remote_side_sdp[SOCKET_BUFFER_SIZE]={0};
            char remote_side_user[50]={0};
            sscanf(buffer,"INVITE %s %s",remote_side_user,remote_side_sdp);


            cout<<"Offerer side: "<<remote_side_user<<"\nSDP:\n";
            cout<<remote_side_sdp<<endl;
                        
            //accept-decline
            
            //assume accepted

            char answer_to_call[SOCKET_BUFFER_SIZE];
            memset(answer_to_call,0,SOCKET_BUFFER_SIZE);

            //send it is accepted
            sprintf(answer_to_call,"ACCEPT %s",local_sdp.c_str());
            send(client_socket,answer_to_call,sizeof(answer_to_call),0);



        }
        
        if(!running.load()) {
            break;
        }
        cout<<buffer<<endl;
    }

    close(client_socket);


}

int send_ip_and_port_to_server(const int client_socket,const char*client_address,const int client_port) {
    char message[SOCKET_BUFFER_SIZE];
    memset(message,0,SOCKET_BUFFER_SIZE);
    sprintf(message,"CLIENT %s %d",client_address,client_port);
    return send(client_socket,message,sizeof(message),0);

}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <server_ip_address> <server_port> <client_ip_address> <client_port>" << std::endl;
        return 1;
    }


		rtc::InitLogger(rtc::LogLevel::Debug);
		auto pc = std::make_shared<rtc::PeerConnection>();

		pc->onStateChange(
		    [](rtc::PeerConnection::State state) { std::cout << "State: " << state << std::endl; });

		pc->onGatheringStateChange([pc](rtc::PeerConnection::GatheringState state) {
			std::cout << "Gathering State: " << state << std::endl;
			if (state == rtc::PeerConnection::GatheringState::Complete) {
				auto description = pc->localDescription();
				json message = {{"type", description->typeString()},
				                {"sdp", std::string(description.value())}};
				std::cout << message << std::endl;
                local_sdp=to_string(message);
			}
		});


		const rtc::SSRC ssrc = 42;
		rtc::Description::Video media("video", rtc::Description::Direction::SendOnly);
		media.addH264Codec(96); // Must match the payload type of the external h264 RTP stream
		media.addSSRC(ssrc, "video-send");
		auto track = pc->addTrack(media);

		pc->setLocalDescription();


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
