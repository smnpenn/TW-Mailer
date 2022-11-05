#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <sstream>
#define PORT 8080

using namespace std;

vector<string> splitInputMessage(const string& string){
    std::vector<std::string> tokens;
    std::stringstream stream(string);
    std::string token;
    while(std::getline(stream, token, '\n')){
        tokens.push_back(token);
    }
    return tokens;
}

int main(int argc, char *argv[])
{
    int server_fd, new_socket;

    struct sockaddr_in address;

    char buffer[1024];

    vector<string> messageTokens;

    string message;

    int opt = 1;

    int n = 0;

    int addrlen = sizeof(address);

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen))< 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Function to read incoming message
    while((n=recv(new_socket, buffer, sizeof(buffer), 0))>0){
        message.append(buffer, buffer+n);

        messageTokens = splitInputMessage(message);

        for(auto const &tokens : messageTokens){
            cout << tokens << endl;
        }
    }

    return 0;
}