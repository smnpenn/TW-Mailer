#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <errno.h>
#include <filesystem>
#include <fstream>
#include <cstring>

#define MAXLINE 1500
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

void messageToFile(string message, filesystem::path path){
    ifstream current_id(path / "current_id.txt"); //, ios_base::in | ios_base::out
    string id_str;

    if(current_id.is_open()){
        getline(current_id, id_str);
        current_id.close();
        int nextid = atoi(id_str.c_str())+1;
        id_str = id_str + ".txt";
        ofstream messageFile(path / id_str);
        if(messageFile.is_open()){
            messageFile << message;
            messageFile.close();
            ofstream current_idof(path / "current_id.txt");
            if(current_idof.is_open()){
                current_idof << nextid;
                current_idof.close();
            }
            
        }else{
            cout << "Error opening id file" << endl;
        }
        
    }else{
        cout << "Error opening file" << endl;
    }
}

string listMessagesFromUser(filesystem::path userDir){
    string msgList = "";
    for (auto const &dir_entry : filesystem::directory_iterator(userDir))
    {
        string name = filesystem::path(dir_entry).filename();
        if(name == "current_id.txt")
            continue;
        ifstream current_file(userDir / name);
        if(current_file.is_open()){
            string subject;
            
            for(int i=0;i<3;++i){
                getline(current_file, subject);
            }
            msgList.append(subject + "\n");
            
        }       
    }

    if(msgList == "")
        return "empty";
    else
        return msgList;
}

int main(int argc, char *argv[])
{
    if(argc<3){
        cout << "Invalid number of arguments\nUsage: ./filename <port> <mail-spool-directory>" << endl;
        return -1;
    }

    int server_fd, new_socket;
    struct sockaddr_in address;
    char buffer[1024];
    vector<string> messageTokens;
    string message;
    int opt = 1;
    int n = 0;
    int port = atoi(argv[1]);
    string mailspool_dir = argv[2];
    filesystem::path mailspool_path = filesystem::current_path() / mailspool_dir;

    cout << "Path: " << mailspool_path << endl;

    if(!filesystem::exists(mailspool_path)){
        filesystem::create_directory(mailspool_path);
    }

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
    address.sin_port = htons(port);

    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    cout << "Server running and listening on port " << port << endl;

    if ((new_socket = accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen))< 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    string username;
    
    //get username
    if((n=recv(new_socket, buffer, sizeof(buffer), 0))>0){
        username.append(buffer, buffer+n);
    }

    cout << username << " connected!" << endl;
    filesystem::path mailspool_userDir = mailspool_path / username;

    if(!filesystem::exists(mailspool_userDir)){
        filesystem::create_directory(mailspool_userDir);

        ofstream myFile(mailspool_userDir / "current_id.txt");
        if(myFile.is_open()){
            myFile << "1";
            myFile.close();
        }else{
            cout << "ERR: opening file" << endl;
            return -1;
        } 
    }
    
    // Function to read incoming message
    while((n=recv(new_socket, buffer, sizeof(buffer), 0))>0){
        message.append(buffer, buffer+n);
        messageTokens = splitInputMessage(message);

        if(messageTokens[1] == "SEND"){
            if(messageTokens.size() < 5){
                cout << "ERR" << endl;
            }else{
                cout << "SEND" << endl;
                cout << "Sender: " << username << endl;
                cout << "Receiver: " << messageTokens[2] << endl;
                cout << "Subject: " << messageTokens[3] << endl;
                cout << "Message:" << endl;

                string msgWithoutHeaders = username + "\n" + messageTokens[2] + "\n" + messageTokens[3] + "\n";
                for(unsigned int i=4;i<messageTokens.size();++i){
                    cout << messageTokens[i] << endl;
                    msgWithoutHeaders.append(messageTokens[i]);
                }

                messageToFile(msgWithoutHeaders, mailspool_userDir);
                cout << "OK" << endl;
            }
            
        }
        else if(messageTokens[1] == "LIST"){
            cout << "LIST" << endl;
            
            string msgList = listMessagesFromUser(mailspool_path / messageTokens[2]);
            cout << msgList << endl;
            if(send(new_socket, msgList.data(), msgList.size(), 0) < 0){
                perror("Error sending msgList");
                //return errno;
            }else{
                cout << "Sent to client!" << endl;
            }
        }
        else if(messageTokens[1] == "DEL"){
            cout << "DEL" << endl;
            //delete message with given id from given user
        }else if(messageTokens[1] == "READ"){
            cout << "READ" << endl;
            //read message with given id from given user
        }

        message.clear();
        messageTokens.clear();
    }

    cout << username << " disconnected!" << endl;
    shutdown(new_socket, SHUT_WR);

    return 0;
}

