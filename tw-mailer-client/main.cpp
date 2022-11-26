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

bool checkSenderReceiverLength(vector<string> messageTokens){
    if((messageTokens[2].length() > 8) || (messageTokens[3].length() > 8))
        return false;
    else
        return true;
}

string convertVectorToString(const vector<string>& v){
    string s;
    for(const auto &token : v) s += token + "\n";
    return s;
}

string GetServerResponse(int sock){
    string response = "";
    char buffer[1024];
    int n = 0;

    sleep(0.1);
    n = recv(sock, buffer, sizeof(buffer), 0);
    response.append(buffer, buffer+n);
    
    return response;
}

void Send(int sock, string username){
    string message = "SEND";
    SendMessageToServer(sock, message);

    message.clear();
    cout << "Sender:" << endl;
    cin >> message;
    SendMessageToServer(sock, message);

    message.clear();
    cout << "Receiver:" << endl;
    cin >> message;
    SendMessageToServer(sock, message);

    //max 80 chars
    message.clear();
    cout << "Subject:" << endl;
    cin >> message;
    SendMessageToServer(sock, message);

    message.clear();
    cout << "Message:" << endl;
    getline(cin, message, '.');
    SendMessageToServer(sock, message);

    cout << "Server response: " << GetServerResponse(sock) << endl;
}

void List(int sock){
    string response = "";
    string message = "LIST";
    SendMessageToServer(sock, message);

    message.clear();
    cout << "Username:" << endl;
    cin >> message;
    SendMessageToServer(sock, message);

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

void Read(int sock){
    string message = "";
    string response = "";
    SendMessageToServer(sock, "READ");

    cout << "Username: " << endl;
    cin >> message;
    SendMessageToServer(sock, message);

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

    cout << "Username: " << endl;
    cin >> message;
    SendMessageToServer(sock, message);

    cout << "Message No.: " << endl;
    cin >> message;
    SendMessageToServer(sock, message);

    //get OK or ERR
    response = GetServerResponse(sock);
    cout << response << endl;
}

void Quit(int sock){

}

int main(int argc, char *argv[])
{
    if(argc<3){
        cerr << "Invalid number of arguments\nUsage: ./filename <server_ip> <port>" << endl;
        return -1;
    }

    string userstr;
    char* server_ip = argv[1];
    int server_port = atoi(argv[2]);

    // Usage ./twmailer-client <ip> <port>
    
    while(true){
        cout << "Enter username: " << endl;

        do{
            cin >> userstr;
            transform(userstr.begin(), userstr.end(), userstr.begin(),[](unsigned char c){return std::tolower(c); }); // converting string to lower case
            if(!checkStringValidity(userstr)){
                cout << "Username should only contain a-z and 0-9" << endl;
            }
        }while(!checkStringValidity(userstr));

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

    //send username
    SendMessageToServer(sock, userstr);

    while(true){
        string operation;
        cout << "Choose operation (type help to see all the options):" << endl;
        cin >> operation;
        //to upper case
        
        if(operation == "SEND"){
            Send(sock, userstr);
        }else if(operation == "LIST"){
            List(sock);
        }else if(operation == "READ"){
            Read(sock);
        }else if(operation == "DEL"){
            Delete(sock);
        }else if(operation == "QUIT"){
            Quit(sock);
        }else if(operation == "HELP"){
            cout << "SEND: Send a message to the server" << endl;
            cout << "LIST: List all messages of a certain user" << endl;
            cout << "READ: Read a message of a certain user" << endl;
            cout << "DELETE: Delete a message of a certain user" << endl;
            cout << "QUIT: logout" << endl;
        }
        else{
            cout << "Unknown operation!\nTry again!" << endl;
        }

        operation.clear();
    }

    /*while(true){
        vector<string> messageTokens;
        string message;

        do{
            getline(cin, message, '.');
            messageTokens = splitInputMessage(message);

            if(messageTokens[1] == "QUIT"){
                break;
            }

            if(messageTokens[2] == userstr && messageTokens[1] == "SEND"){
                message.clear();
                messageTokens.erase(messageTokens.begin()+2);
                message = convertVectorToString(messageTokens);
                break;
            }

        } while(messageTokens[2] != userstr);

        getline(cin, message, '.');
        messageTokens = splitInputMessage(message);

        if(messageTokens[1] == "SEND" && messageTokens.size() < 6){ 
            sendMessageToServer(sock, message);

        }else if(messageTokens[1] == "LIST" && messageTokens.size() < 4){
            sendMessageToServer(sock, message);
            cout << " ------------------- " << endl;
            cout << "SUBJECT LIST" << endl;
            cout << " ------------------- " << endl;
            string response = "";
            n = recv(sock, buffer, sizeof(buffer), 0);
            response.append(buffer, buffer+n);
            cout << response << endl;
            response.clear();
        }else if(messageTokens[1] == "QUIT"){
            cout << "QUITTING" << endl;
            shutdown(sock, SHUT_WR);
            exit(0);
            break;
        } else if((messageTokens[1] == "READ") && messageTokens.size() < 5){
            sendMessageToServer(sock, message);
            string response = "";
            n = recv(sock, buffer, sizeof(buffer), 0);
            response.append(buffer, buffer+n);
            cout << response << endl;
            response.clear();
        } else if((messageTokens[1] == "DEL") && messageTokens.size() < 5) {
            sendMessageToServer(sock, message);
            string response = "";
            n = recv(sock, buffer, sizeof(buffer), 0);
            response.append(buffer, buffer+n);
            cout << response << endl;
            response.clear();

        }
        cout << endl;
        message.clear();
    }*/

}
