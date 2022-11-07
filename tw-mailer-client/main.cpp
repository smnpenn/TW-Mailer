#include <iostream>
#include <unistd.h>
#include <string>
#include <algorithm>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <sstream>

#define PORT 8080

using namespace std;

vector<string> splitInputMessage(const string& string){
    std::vector<std::string> tokens;
    stringstream stream(string);
    std::string token;
    while(std::getline(stream, token, '\n')){
        tokens.push_back(token);
    }
    return tokens;
}

void sendMessageToServer(int sock, string message){
    if(send(sock, message.data(), message.size(), 0) < 0){
        perror("Server error");
        //return errno;
    }else{
        cout << "Sent to server!" << endl;
    }
}

bool checkSenderReceiverLength(vector<string> messageTokens){
    if((messageTokens[2].length() > 8) || (messageTokens[3].length() > 8))
        return false;
    else
        return true;
}

int main(int argc, char *argv[])
{
    if(argc<3){
        cout << "Invalid number of arguments\nUsage: ./filename <server_ip> <port>" << endl;
        return -1;
    }

    //char username[8];
    string userstr;
    int n = 0;
    char* server_ip = argv[1];
    int server_port = atoi(argv[2]);
    char buffer[1024];

    // Usage ./twmailer-client <ip> <port>
    
    cout << "Chosen ip adrress and port: " << server_ip << " : " << server_port << endl;
    
    while(true){
        cout << "Enter username: " << endl;
        //TODO: nur a-z, 0-9 erlaubt
        
        cin >> userstr;

        if(userstr.length()>8){
            cout << "Username is too long (max 8 character)" << endl;
        }else{
            break;
        }
    }
    
    cout << "Chosen username " << userstr << endl;

    // ---------- SOCKET CREATION -----------
    
    int sock = 0, client_fd;
    
    struct sockaddr_in serv_addr; // Socket Address Structure für IPv4

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ // AF_INT -> Family, type: SOCK_STREAM (TCP), SOCK_DGRAM is for UDP, 0 -> for TCP /UDP
        cout << "Socket creation failed!" << endl;
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0){
        cout << "Address invalid/not supported" << endl;
        return -1;
    }

    if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr,sizeof(serv_addr)))
        < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    cout << "Connected" << endl;

    //send username
    string username_str;
    sendMessageToServer(sock, userstr);
    string response;

    while(true){
        vector<string> messageTokens;
        string message;

        getline(cin, message, '.');

        messageTokens = splitInputMessage(message);

        if(messageTokens[1] == "SEND"){ // maybe um die Modes zu checken -> wird wsh eh nicht gebraucht.
            sendMessageToServer(sock, message);
        } else if(messageTokens[1] == "LIST"){
            sendMessageToServer(sock, message);
            n = recv(sock, buffer, sizeof(buffer), 0);
            response.append(buffer, buffer+n);
            cout << response << endl;
        }else if(messageTokens[1] == "QUIT"){
            cout << "QUITTING" << endl;
            shutdown(sock, SHUT_WR);
        }

        cout << endl;

        message.clear();
    }

}
