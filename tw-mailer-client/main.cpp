#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <string>
#include <algorithm>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <sstream>

#define PORT 8080

using namespace std;

static inline bool isNotAlphaNumeric(char c)
{
    return !(isalnum(c));
}

bool checkStringValidity(string &string){
    return find_if(string.begin(), string.end(), isNotAlphaNumeric) == string.end();
}

void SendMessageToServer(int sock, string message){
    sleep(0.1);
    if(send(sock, message.data(), message.size(), 0) < 0){
        perror("Server error");
        //return errno;
    }
}


string GetServerResponse(int sock){
    string response = "";
    char buffer[1024];
    int n = 0;

    //sleep(0.1);
    n = recv(sock, buffer, sizeof(buffer), 0);
    response.append(buffer, buffer+n);
    
    return response;
}

void Send(int sock){
    string message = "SEND";
    string textMessage(80, '\n');
    bool fitsSize = false;
    SendMessageToServer(sock, message);

    message.clear();
    cout << "Receiver:" << endl;
    cin >> message;
    SendMessageToServer(sock, message);

    message.clear();
    cout << "Subject:" << endl;
    getline(cin >> ws, message); // um das mit dem space to fixen.
    SendMessageToServer(sock, message);

    message.clear();
    cout << "Message:" << endl;
    do{
        getline(cin, textMessage, '.');
        if((textMessage.length() > 0) && (textMessage.length() <= 80)){
            fitsSize = true;
            break;
        }
    }while(fitsSize == false);
    SendMessageToServer(sock, textMessage);

    cout << "Server response: " << GetServerResponse(sock) << endl;
}

void List(int sock){
    string response = "";
    string message = "LIST";
    SendMessageToServer(sock, message);
    message.clear();

    while(true){
        response.clear();
        response = GetServerResponse(sock);

        if(response != "EOF"){
            cout << response;
        }else{
            break;
        }
    }
    cout << endl;
}

void Read(int sock){
    string message = "";
    string response = "";
    SendMessageToServer(sock, "READ");

    cout << "Message No.: " << endl;
    cin >> message;
    SendMessageToServer(sock, message);

    //get OK or ERR
    response = GetServerResponse(sock);
    cout << response << endl;
    if(response == "OK"){
        //Wait for all the lines, stop at EOF
        while(true){
            response.clear();
            response = GetServerResponse(sock);

            if(response != "EOF"){
                cout << response << endl;
            }else{
                break;
            }
        }
    }
}

void Delete(int sock){
    string message = "";
    string response = "";
    SendMessageToServer(sock, "DEL");

    cout << "Message No.: " << endl;
    cin >> message;
    SendMessageToServer(sock, message);

    //get OK or ERR
    response = GetServerResponse(sock);
    cout << response << endl;
}

void Quit(int sock){
    cout << "Quitting application..." << endl;
    SendMessageToServer(sock, "QUIT");
    close(sock);
}

string validUsername(){
    string userString;
    while(true){
        cout << "Enter Technikum LDAP username:" << endl;
        do {
            cin >> userString;
            transform(userString.begin(), userString.end(), userString.begin(),[](unsigned char c){return std::tolower(c); }); // converting string to lower case
            if(!checkStringValidity(userString)){
                cout << "Username should only contain a-z and 0-9" << endl;
            }
        }while(!checkStringValidity(userString));
        if(userString.length()>8){
            cout << "Username is too long (max 8 character)" << endl;
        }else{
            break;
        }
    }
    return userString;
}

string loginFunction(int sock){

    int attempts = 0;
    string userstr;
    string userpw;

    do{
        userstr.clear();
        userstr = validUsername();

        // Hide input
        termios oldt;
        tcgetattr(STDIN_FILENO, &oldt);
        termios newt = oldt;
        newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        cout << "Enter password: " << endl;
        cin >> userpw;
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        // display input


        SendMessageToServer(sock, userstr);
        SendMessageToServer(sock, userpw);
        string response = GetServerResponse(sock);
        cout << "Response " << response << endl;
        if(response == "ERR"){
            cout << "Invalid username/password!" << endl;
            if(attempts == 2){
                cout << "Client locked!" << endl;
                return "LOCKED";
            }
            attempts++;
            userstr = "UNDEFINED";
        } else if(response == "OK") {
            break;
        }

    }while(attempts < 3);

    return userstr;
}

int main(int argc, char *argv[])
{
    if(argc<3){
        cerr << "Invalid number of arguments\nUsage: ./filename <server_ip> <port>" << endl;
        return -1;
    }

    // int attempts = 0;
    bool authorized = false;

    string userstr;
    string userpw;
    char* server_ip = argv[1];
    int server_port = atoi(argv[2]);

    // Usage ./twmailer-client <ip> <port>
    
    // ---------- SOCKET CREATION -----------
    
    int sock = 0, client_fd;
    
    struct sockaddr_in serv_addr; // Socket Address Structure für IPv4

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ // AF_INT -> Family, type: SOCK_STREAM (TCP), SOCK_DGRAM is for UDP, 0 -> for TCP /UDP
        cerr << "Socket creation failed!" << endl;
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0){
        cerr << "Address invalid/not supported" << endl;
        return -1;
    }

    if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr,sizeof(serv_addr)))< 0) {
        cerr << "Connection failed!" << endl;
        return -1;
    }
    cout << "Connected" << endl;

    

    while(true){
        string operation;
        cout << "Choose operation (type help to see all the options):" << endl;
        cin >> operation;
        transform(operation.begin(), operation.end(), operation.begin(), ::toupper);
        
        if(operation == "SEND"){
            if(authorized == true){
                Send(sock);
            } else {
                cout << "ERR: unauthorized" << endl;
            }
        }else if(operation == "LIST"){
            if(authorized == true){
                List(sock);
            } else {
                cout << "ERR: unauthorized" << endl;
            }
        }else if(operation == "READ"){
            if(authorized == true){
                Read(sock);
            } else {
                cout << "ERR: unauthorized" << endl;
            }
        }else if(operation == "DEL"){
            if(authorized == true){
                Delete(sock);
            } else {
                cout << "ERR: unauthorized" << endl;
            }
        }else if(operation == "QUIT"){
            Quit(sock);
            break;
        }else if(operation == "HELP"){
            if(authorized == false){
                cout << "LOGIN: Login to LDAP" << endl;
                cout << "QUIT: logout" << endl;
            } else if(authorized == true){
                cout << "SEND: Send a message to the server" << endl;
                cout << "LIST: List all messages of a certain user" << endl;
                cout << "READ: Read a message of a certain user" << endl;
                cout << "DELETE: Delete a message of a certain user" << endl;
                cout << "QUIT: logout" << endl;
            }
        } else if(operation == "LOGIN"){
            userstr = loginFunction(sock);
            if(userstr == "UNDEFINED" || userstr == "LOCKED"){
                authorized = false;
                if(userstr == "LOCKED"){
                    return -1;
                }
            } else {
                authorized = true;
            }
        }
        else{
            cout << "Unknown operation!\nTry again!" << endl;
        }

        operation.clear();
    }

    return 0;
}
