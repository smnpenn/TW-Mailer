#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080

using namespace std;

int main(int argc, char *argv[])
{
    
    char username[8];
    
    // Usage ./twmailer-client <ip> <port>
    
    int opt = 0;
    cout << "Hello " << argv[1] << " " << argv[2]<< endl;
    
        
    
    cout << "Chosen ip adrress and port: " << argv[1] << " : " << argv[2] << endl;
    
    
    cout << "Enter username " << endl;
    
    cin >> username;
    
    cout << "Chosen username " << username << endl;
    
    // ---------- SOCKET CREATION -----------
    
    int sock = 0, valread, client_fd;
    
    struct sockaddr_in serv_addr; // Socket Address Structure für IPv4
    
    char* hello = "Hello from client";
    
    char buffer[1024] = {0};
    
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
    
    if((client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0){
        cout << "Connection failed" << endl;
        return -1;
    }

    return 0;
}
