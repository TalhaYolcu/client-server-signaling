#include "rtc/rtc.hpp"

#include <iostream>
#include <memory>
#include <utility>
#include <fstream>
#include <nlohmann/json.hpp>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>


using nlohmann::json;
using namespace std;

const int BUFFER_SIZE = 2048;

bool sdp_received=false;

bool sdp_generated=false;

string my_local_answer_sdp;

vector<string> local_candidates_vector;
vector<string> remote_candidates_vector;

int main(int argc, char const *argv[])
{

    cout<<"Example run:./sdp_send 192.168.43.19 8080"<<endl;

    if(argc<1) {
        cout<<"Please enter peer ip address and port"<<endl;
    }

    int PORT=atoi(argv[2]);

    char remote_ip[50]={0};
    strcpy(remote_ip,argv[1]);

    cout<<"Remote ip:"<<endl;
    cout<<string(remote_ip)<<endl;

    cout<<"Remote port:"<<PORT<<endl;



    rtc::InitLogger(rtc::LogLevel::Debug);
	rtc::Configuration config;
	config.iceServers.emplace_back("stun.l.google.com:19302");


    auto pc = std::make_shared<rtc::PeerConnection>(config);


    pc->onLocalCandidate([](rtc::Candidate candidate) {
        std::cout << "Local Candidate (Paste this to the other peer after the local description):"
                << std::endl;
        std::cout << std::string(candidate) << std::endl << std::endl;
        //push to the vector

        local_candidates_vector.push_back(std::string(candidate));
        

    });


    

    pc->onStateChange(
        [](rtc::PeerConnection::State state) { std::cout << "State: " << state << std::endl; });

    pc->onGatheringStateChange([pc](rtc::PeerConnection::GatheringState state) {
        std::cout << "Gathering State: " << state << std::endl;
        if (state == rtc::PeerConnection::GatheringState::Complete) {
            auto description = pc->localDescription();
            json message = {{"type", description->typeString()},
                            {"sdp", std::string(description.value())}};
            std::cout << message << std::endl;
            std::ofstream myfile;
            myfile.open("answer.txt");
            myfile<<message;
            myfile.close();
            my_local_answer_sdp=message.dump();        
            sdp_generated=true;    
        }
    });


	shared_ptr<rtc::DataChannel> dc;
	pc->onDataChannel([&](shared_ptr<rtc::DataChannel> _dc) {
		std::cout << "[Got a DataChannel with label: " << _dc->label() << "]" << std::endl;
		dc = _dc;

		dc->onClosed(
		    [&]() { std::cout << "[DataChannel closed: " << dc->label() << "]" << std::endl; });

		dc->onMessage([](auto data) {
			if (std::holds_alternative<std::string>(data)) {
				std::cout << "[Received message: " << std::get<std::string>(data) << "]"
				          << std::endl;
			}
		});
	});


    // Create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Convert the server address from text to binary form
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, remote_ip, &server_addr.sin_addr) != 1) {
        std::cerr << "Invalid server address" << std::endl;
        return 1;
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }



    char buffer[BUFFER_SIZE] = {0};


    cout<<"Waiting for offerer sdp"<<endl;
    recv(client_socket,buffer,BUFFER_SIZE,0);
    
    std::string remote_offer_sdp=std::string(buffer);

    cout<<"Offerer sdp received:\n";
    cout<<remote_offer_sdp<<endl;

    
    json j = json::parse(remote_offer_sdp);
    rtc::Description offer(j["sdp"].get<std::string>(), j["type"].get<std::string>());
    pc->setRemoteDescription(offer);    

    //sdp is received, generate sdp
    sdp_received=true;


    //change it with mutex
    while(!sdp_generated) ;
    
    //answerer sdp sent
    send(client_socket,my_local_answer_sdp.c_str(), strlen(my_local_answer_sdp.c_str()), 0);
    

    //wait for receiver candidate count
    int remote_cand_count=0;
    recv(client_socket,&remote_cand_count,4,0);

    cout<<"Getting remote cand count "<<remote_cand_count<<endl;

    if(remote_cand_count<=0) {
        std::cout<<"Error getting remote cand count"<<endl;
    }
    
    // wait for receiver candiates

/*
    for(int i=0;i<remote_cand_count;i++) {
        char remote_candidates[BUFFER_SIZE]={0};
        recv(client_socket, remote_candidates, BUFFER_SIZE, 0);
        remote_candidates_vector.push_back(string(remote_candidates));
        cout<<"One candidate received:"<<endl<<string(remote_candidates)<<endl;
        pc->addRemoteCandidate(rtc::Candidate(std::string(remote_candidates)));
    }
        cout<<"All candidates received"<<endl;

*/
    char remote_candidates[BUFFER_SIZE]={0};
    recv(client_socket, remote_candidates, BUFFER_SIZE, 0);
    cout<<"Received candidate:"<<string(remote_candidates)<<endl;
    pc->addRemoteCandidate(rtc::Candidate(std::string(remote_candidates)));


    //send count of local candidates
    int length_of_vector=local_candidates_vector.size();
    send(client_socket,&length_of_vector,4,0);

    cout<<"Local sdp count sent"<<endl;

    string my_local_cand=local_candidates_vector.at(0);

    send(client_socket,my_local_cand.c_str(),my_local_cand.length(),0);

    /*for(auto const& cand:local_candidates_vector) {
        send(client_socket,cand.c_str(),std::strlen(cand.c_str()),0);
    }*/

    cout<<"All local cands sent"<<endl;

    sleep(100);
    close(client_socket);
    return 0;
}
