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


int main(int argc, char *argv[])
{
    char username[8];
    int n = 0;

    // Usage ./twmailer-client <ip> <port>
    
    cout << "Hello " << argv[1] << " " << argv[2]<< endl;
    
        
    
    cout << "Chosen ip adrress and port: " << argv[1] << " : " << argv[2] << endl;
    
    
    cout << "Enter username " << endl;
    
    cin >> username;
    
    cout << "Chosen username " << username << endl;
    
    // ---------- SOCKET CREATION -----------



    
    int sock = 0, client_fd;
    
    struct sockaddr_in serv_addr; // Socket Address Structure für IPv4

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ // AF_INT -> Family, type: SOCK_STREAM (TCP), SOCK_DGRAM is for UDP, 0 -> for TCP /UDP
        cout << "Socket creation failed!" << endl;
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0){
        cout << "Address invalid/not supported" << endl;
        return -1;
    }

    if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr,sizeof(serv_addr)))
        < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    while(true){
        vector<string> messageTokens;

        string message;

        getline(cin, message, '.');

        messageTokens = splitInputMessage(message);

        if(messageTokens[1] == "SEND"){ // maybe um die Modes zu checken -> wird wsh eh nicht gebraucht.
            cout << "MODE: SENDING" << endl;
        } else if(messageTokens[1] == "LIST"){
            cout << "MODE: LISTING" << endl;
        } else if(messageTokens[1] == "QUIT"){
            cout << "QUITTING" << endl;
            break;
        }

        n = (send(sock, message.data(), message.size(), 0)) < 0;
        if(n){
            perror("Server error");
            return errno;
        }
        cout << "Sent message" << endl;

        message.clear();


    }











}
