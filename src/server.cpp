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
#include "rtc/rtc.hpp"
#include "rtc/peerconnection.hpp"
#include "rtc/global.hpp"
#include <nlohmann/json.hpp>

std::atomic_bool running(true);
using namespace std;
using nlohmann::json;

UserStore userStore;

void signal_handler(int sig) {
    if (sig == SIGINT) {
        running.store(false);
    }
}

int connect_to_client(char*client_ip_address,int* client_port_int,int* client_socket_fd) {


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
    int client_port_int=-1;
    int client_socket_fd=-1;
    memset(client_ip_address,0,15);
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

        if(strncmp(buffer,"CLIENT",6)==0) {
            cout<<"Client ip address and port received\n";
            char client_port_char[10]={0};

            sscanf(buffer,"CLIENT %s %d",client_ip_address,&client_port_int);

            cout<<client_ip_address<<endl;
            cout<<client_port_int<<endl;

            if(connect_to_client(client_ip_address,
            &client_port_int,&client_socket_fd)!=0) {
                cout<<"Error connecting to the server\n";
                break;
            }
            char message[SOCKET_BUFFER_SIZE] = "Hello from new server\n";
            send(client_socket_fd,message,sizeof(message),0);


        }
        else if(strncmp(buffer,"SIGNUP",6)==0) {
            cout << "This is register\n";

            char username_char[50]={0};
            char password_char[50]={0};

            sscanf(buffer,"SIGNUP %s %s",username_char,password_char);

            username=string(username_char);
            password=string(password_char);

            userStore.addUser(User(username,password,false,client_socket_fd));

            char message[SOCKET_BUFFER_SIZE] = "Signup is successful\n";
            send(client_socket,message,sizeof(message),0);            

        }
        else if(strncmp(buffer,"LOGIN",5)==0) {
            cout<< "This is login\n";

            char username_char[50]={0};
            char password_char[50]={0};

            sscanf(buffer,"LOGIN %s %s",username_char,password_char);

            username=string(username_char);
            password=string(password_char);

            userStore.loginUser(username,password);

            char message[SOCKET_BUFFER_SIZE] = "Login is successful\n";
            send(client_socket,message,sizeof(message),0);    
     
        }

        else if(strncmp(buffer,"CALL",4)==0) {
            cout<<"This is call\n";

            char receiver_username_char[50]={0};
            char first_peer_sdp[SOCKET_BUFFER_SIZE]={0};

            //take senders sdp
            sscanf(buffer,"CALL %s",receiver_username_char);

            int len=4+1+strlen(receiver_username_char)+1; //CALL username 
            strncpy(first_peer_sdp,buffer+len,strlen(buffer)-len);

            cout<<"First peer sdp\n";
            cout<<first_peer_sdp<<endl;
            string receiver_username=string(receiver_username_char);

            if(!userStore.isLogin(receiver_username)) {
                //user is not login, send error message to client
            }
            else if(!userStore.hasUser(receiver_username)) {
                //user not found, send error message to client
            }
            else {
                

                //send to receiver
                try {
                    User receiverUser=userStore.getUser(receiver_username);
                    int receiver_fd=receiverUser.getSocketFd();
                    
                    char message[SOCKET_BUFFER_SIZE*2];
                    memset(message,0,SOCKET_BUFFER_SIZE*2);

                    sprintf(message,"INVITE %s %s",receiver_username_char,first_peer_sdp);

                    cout<<"Sending message to the receiver\n";
                    cout<<message<<endl;
                    send(receiver_fd,message,sizeof(message),0);

                    //wait receiver to send its sdp

                    char incoming_message[SOCKET_BUFFER_SIZE];
                    memset(incoming_message,0,SOCKET_BUFFER_SIZE);

                    recv(receiver_fd,incoming_message,sizeof(incoming_message),0);
                    
                    cout<<"Receiver's sdp:\n"<<endl;
                    cout<<incoming_message<<endl;

                    char accept_command[50]={0};

                    sscanf(incoming_message,"%s",accept_command);

                    if(strncmp(accept_command,"ACCEPT",6)==0) {
                        //receiver accepted request

                        cout<<"Forwarding receivers sdp to sender:\n";
                        cout<<incoming_message<<endl;
                        //forward it to first sender

                        
                        send(client_socket_fd,incoming_message,sizeof(incoming_message),0);
                        

                        // rtc::InitLogger(rtc::LogLevel::Debug);
                        // rtc::Configuration config;
                        // config.iceServers.emplace_back("stun:stun.l.google.com:19302");

                        // // Create three PeerConnections: offerer_peer, answerer_peer, and local_peer
                        // auto offerer_peer = std::make_shared<rtc::PeerConnection>(config); //peer that sends offer and server sends answer
                        
                        // std::string local_to_offerer_sdp;
                        // offerer_peer->onStateChange(
                        // [](rtc::PeerConnection::State state) { std::cout << "State: " << state << std::endl; });

                        // offerer_peer->onGatheringStateChange([offerer_peer, &local_to_offerer_sdp](rtc::PeerConnection::GatheringState state) {
                        //     std::cout << "Gathering State for offerer peer: " << state << std::endl;
                        //     if (state == rtc::PeerConnection::GatheringState::Complete) {
                               
                        //         auto description = offerer_peer->localDescription();
                        //         json local_to_offerer = {{"type", description->typeString()},
                        //                         {"sdp", std::string(description.value())}};
                        //         std::cout << local_to_offerer << std::endl;
                        //         local_to_offerer_sdp = local_to_offerer.dump();

                        //     }
                        // });

                        // const rtc::SSRC ssrc = 42;
                        // rtc::Description::Video media("video", rtc::Description::Direction::SendRecv);
                        // media.addH264Codec(96); // Must match the payload type of the external h264 RTP stream
                        // media.addSSRC(ssrc, "video-sendrecv");
                        // auto track = offerer_peer->addTrack(media);

                        // json j = json::parse(first_peer_sdp);
                        // rtc::Description offer_sdp(j["description"].get<std::string>(),"offer");
                        // offerer_peer->setRemoteDescription(rtc::Description(incoming_message,rtc::Description::Type::Offer));
                        // offerer_peer->setLocalDescription(rtc::Description::Type::Answer);
                        // send(client_socket_fd, &local_to_offerer_sdp ,sizeof(local_to_offerer_sdp),0);

                        // ///////////Answerer////////////
                        
                        // auto answerer_peer = std::make_shared<rtc::PeerConnection>(config); // peer that server sends offer and peer sends answer
                        
                        // std::string local_to_answerer_sdp;
                        // answerer_peer->onStateChange(
                        // [](rtc::PeerConnection::State state) { std::cout << "State: " << state << std::endl; });

                        // answerer_peer->onGatheringStateChange([offerer_peer,&local_to_answerer_sdp](rtc::PeerConnection::GatheringState state) {
                        //     std::cout << "Gathering State for answerer peer: " << state << std::endl;
                        //     if (state == rtc::PeerConnection::GatheringState::Complete) {
                               
                        //         auto description = offerer_peer->localDescription();
                        //         json local_to_answerer = {{"type", description->typeString()},
                        //                         {"sdp", std::string(description.value())}};
                        //         std::cout << local_to_answerer << std::endl;
                        //         local_to_answerer_sdp = local_to_answerer.dump();

                        //     }
                        // });

                        // const rtc::SSRC ssrc2 = 44;
                        // rtc::Description::Video media2("video", rtc::Description::Direction::SendRecv);
                        // media.addH264Codec(96); // Must match the payload type of the external h264 RTP stream
                        // media.addSSRC(ssrc2, "video-sendrecv");
                        // auto track2 = answerer_peer->addTrack(media2);

                        // json j2 = json::parse(incoming_message);
                        // rtc::Description answer_sdp(j2["description"].get<std::string>(),"answer");
                        // answerer_peer->setLocalDescription(rtc::Description::Type::Offer);
                        // answerer_peer->setRemoteDescription(rtc::Description(answer_sdp,rtc::Description::Type::Answer));
                        // send(client_socket_fd, &local_to_answerer_sdp,sizeof(local_to_answerer_sdp),0);

                        // auto session = std::make_shared<rtc::RtcpReceivingSession>();
                        // track->setMediaHandler(session);
                        // auto session2 = std::make_shared<rtc::RtcpReceivingSession>();
                        // track2->setMediaHandler(session2);

                        // int BufferSize = 2048;
                        // char buffer[BufferSize];
                        // char buffer2[BufferSize];
                        // int len;
                        // int len2;

                        

                        // offerer_peer->onTrack([&track,&track2](std::shared_ptr<rtc::Track> trackPtr2) {

                        //     if(track->mid()=="video"){
                        //         track->onMessage( 
                        //             [&track2](rtc::binary message){
                        //                 track2->send(message.data(),message.size());

                        //         },
                        //         nullptr);
                        //     }    
                        //     // auto rtp = reinterpret_cast<rtc::RtpHeader *>(message);
			            //     // rtp->setSsrc(ssrc); 
                        //     // track2->send(message.data(),message.size());
                        // });

                        // answerer_peer->onTrack([&track,&track2](std::shared_ptr<rtc::Track> trackPtr){
                            
                        //     if(track2->mid()=="video"){
                        //         track2->onMessage([track](rtc::binary message){
                        //             track->send(message.data(),message.size());

                        //         },nullptr);
                        //     }    

                        // });


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
        else if(strncmp(buffer,"LOGOUT",6)==0) {
            cout<< "This is logout\n";

            userStore.logOutUser(username);

            char message[SOCKET_BUFFER_SIZE] = "Logout is successful\n";
            send(client_socket,message,sizeof(message),0);             
        }
        else if(strncmp(buffer,"EXIT",4)==0) {
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

    //check arguments
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

    //create vector of threads
    std::vector<std::thread> threads;

    // Wait for incoming connections
    while (running.load()) {
        // Accept incoming connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        
        //if sigint is received
        if(!running.load()) {
            break;
        }

        //error occured on accept
        if (client_socket == -1) {
            std::cerr << "Failed to accept incoming connection" << std::endl;
            break;;
        }

        // Create a new thread to handle client request       
        threads.push_back( std::thread (client_handler, client_socket));
    }

    //wait for threads to finish
    for (auto& t : threads) {
        t.join();
    }

    // Close server socket
    close(server_socket);
    std::cout << "Server stopped" << std::endl;
    return 0;
}
