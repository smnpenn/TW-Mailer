#include <iostream>
#include <sys/socket.h>
#include <sys/wait.h>
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
#include <signal.h>
#include <pthread.h>
#include "ThreadArguments.cpp"
#include <ldap.h>

#define MAXLINE 1500
#define PORT 8080

using namespace std;

int server_fd;
vector<pthread_t> threadPool;
vector<int> socketPool;

int CreateUserDir(string username, filesystem::path mailspool_userDir){

    if(!filesystem::exists(mailspool_userDir)){
        filesystem::create_directory(mailspool_userDir);

        ofstream myFile(mailspool_userDir / "current_id.txt");
        if(myFile.is_open()){
            myFile << "1";
            myFile.close();
        }else{
            return -1;
        } 
    }
    
    return 0;
}

bool loginToLDAP(string user, string password){

    // BEGIN URI INFO
    string loginURI = "ldap://ldap.technikum-wien.at:389";
    char loginURIArray[loginURI.length()+1];
    strcpy(loginURIArray, loginURI.c_str());

    // END URI INFO

    LDAP* ldap;
    BerValue *serverCredentials;
    BerValue credentials;

    int check = 0;
    

    char password_to_array[password.length()+1];
    strcpy(password_to_array, password.c_str());
    credentials.bv_val = (char *) password_to_array;
    credentials.bv_len = password.length();

    int version = LDAP_VERSION3;

    if(ldap_initialize(&ldap, "ldap://ldap.technikum-wien.at:389") != LDAP_SUCCESS){
        cout << "LDAP Initialization failed!" << endl;
        return EXIT_FAILURE;
    }

    if((check = ldap_set_option(ldap, LDAP_OPT_PROTOCOL_VERSION, &version)) != LDAP_SUCCESS){
        cout << "ERROR: LDAP OPT PROTOCOL VERSION =!= 3" << endl;
        return EXIT_FAILURE;
    }

    if((check = ldap_start_tls_s(ldap, NULL, NULL)) != LDAP_SUCCESS){
        cout << "ERROR: LDAP START TLS FAILED" << endl;
        return EXIT_FAILURE;
    }
    string ldapUser = "uid=" + user + ",ou=People," + "dc=technikum-wien,dc=at";

    char result[ldapUser.length()+1];
    strcpy(result, ldapUser.c_str());

    check = ldap_sasl_bind_s(ldap, result, LDAP_SASL_SIMPLE, &credentials, NULL, NULL, &serverCredentials);

    if(check != LDAP_SUCCESS){
        cout << "ERROR: LDAP BIND FAILED" << endl;
        ldap_unbind_ext_s(ldap, NULL, NULL);
        return false;
    }

    cout << "LDAP BIND SUCCESSFUL" << endl;

    cout << "LDAP SEARCH SUCCESSFUL" << endl;

    ldap_unbind_ext_s(ldap, NULL, NULL);

    return true;

}

string messageToFile(string message, filesystem::path path){
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
            return "ERR";
        }
        
    }else{
        return "ERR";
    }

    return "OK";
}

string ReadMsg(int socket){
    int n = 0;
    char buffer[1024];
    string message = "";

    n = recv(socket, buffer, sizeof(buffer), 0);
    message.append(buffer, buffer+n);

    return message;
}

void sendOK(int socket){
    string OK = "OK";
    sleep(0.1);
    send(socket, OK.data(), OK.size(), 0);
}

void sendERR(int socket){
    string ERR = "ERR";
    sleep(0.1);
    send(socket, ERR.data(), ERR.size(), 0);
}

void sendEOF(int socket){
    string eof = "EOF";
    sleep(0.1);
    send(socket, eof.data(), eof.size(), 0);
}

void sendText(int socket, string text){
    sleep(0.1);
    send(socket, text.data(), text.size(), 0);
}

void Send(int socket, string username, filesystem::path mailspool_userDir){
    string message = "";

    //Sender
    message.append(ReadMsg(socket) + "\n");
    //Receiver
    message.append(ReadMsg(socket) + "\n");
    //Subject
    message.append(ReadMsg(socket) + "\n");
    //Message
    message.append(ReadMsg(socket) + "\n");

    string response = messageToFile(message, mailspool_userDir);
    sendText(socket, response);
}

void List(int socket, filesystem::path mailspoolPath){
    string response = "";
    int id = 1;

    string username = ReadMsg(socket);
    filesystem::path userDir = mailspoolPath / username;

    if(!filesystem::exists(userDir)){
        sendText(socket, "0");
        sendEOF(socket);
        return;
    }

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
            response = name.substr(0,1) + ": " + subject + "\n";
            sendText(socket, response);
            ++id;
        }       
    }

    if(id == 1){
        sendText(socket, "0");
    }
    
    sendEOF(socket);
}

void Delete(int socket, filesystem::path mailspool_path){
    string res = "";
    filesystem::path userDir = mailspool_path / ReadMsg(socket);
    string id = ReadMsg(socket) + ".txt";
    bool deleted = false;

    //Check if user exists
    if(!filesystem::exists(userDir)){
        sendERR(socket);
        return;
    }

    for(auto const &dir_entry : filesystem::directory_iterator(userDir)){
        string name = filesystem::path(dir_entry).filename();
        if(name == id){
            if(filesystem::remove(dir_entry)){
                sendOK(socket);
                deleted = true;
                break;
            }
        }
    }

    if(!deleted){
        sendERR(socket);
    }
}

void Read(int socket, filesystem::path mailspool_path){
    string res = "";
    bool found = false;
    filesystem::path userDir = mailspool_path / ReadMsg(socket);
    string id = ReadMsg(socket) + ".txt";

    //Check if user exists
    if(!filesystem::exists(userDir)){
        sendERR(socket);
        return;
    }

    //iterate through user directory and look for the right file
    for(auto const &dir_entry : filesystem::directory_iterator(userDir))
    {
        string name = filesystem::path(dir_entry).filename();
        if(name == id) {
            ifstream current_file(userDir / name);
            found = true;
            if (current_file.is_open()) {
                string text;
                sendOK(socket);
                while (getline(current_file, text)) {
                    //send line by line, send EOF at the end
                    sendText(socket, text);
                }
                sendEOF(socket);
            } else {
                cerr << "couldnt open file" << endl;
                sendERR(socket);
                return;
            }
        }
    }
    
    if (!found){
        cerr << " message not found" << endl;
        sendERR(socket);
        return;
    }
}

void Quit(int socket, string username){
    cout << username << " disconnected" << endl;
    close(socket);
}

void* ClientHandler(void* threadargs){
    //get username
    string username;
    string password;
    // int attempt = 0;
    bool authorized = false;
    int n;
    char buffer[1024];

    //TODO: Operation LOGIN (LDAP)
    //TODO: Mailer Pro Features einbauen bei SEND LIST READ DEL (USername bzw Sender soll automatisch gesetzt werden --> authentifizierter User)
    do{
        if((n=recv(((ThreadArguments*)threadargs)->socket, buffer, sizeof(buffer), 0))>0){
            username.append(buffer, buffer+n);
        }
        if((n=recv(((ThreadArguments*)threadargs)->socket, buffer, sizeof(buffer), 0))>0){
            password.append(buffer, buffer+n);
        }

        if(loginToLDAP(username, password)){
            cout << "connected to technikum LDAP" << endl;
            authorized = true;
            sendText(((ThreadArguments*)threadargs)->socket, "OK");
            break;
        } else {
            cout << "connection to technikum LDAP failed" << endl;
            sendERR(((ThreadArguments*)threadargs)->socket);
        }
    }while(!authorized);

    if(authorized){
        cout << username << " connected!" << endl;
        filesystem::path mailspool_userDir = ((ThreadArguments*)threadargs)->mailspool_path / username;

        if(CreateUserDir(username, mailspool_userDir) == -1){
            cerr << "ERR: opening file" << endl;
            exit(EXIT_FAILURE);
        }
    
        //listen for incoming operations
        string operation;
        while((n=recv(((ThreadArguments*)threadargs)->socket, buffer, sizeof(buffer), 0))>0){
            operation.clear();
            operation.append(buffer, buffer+n);
            cout << operation << endl;
            if(operation == "SEND"){
                Send(((ThreadArguments*)threadargs)->socket, username, mailspool_userDir);
            }else if(operation == "LIST"){
                List(((ThreadArguments*)threadargs)->socket, ((ThreadArguments*)threadargs)->mailspool_path);
            }else if(operation == "READ"){
                Read(((ThreadArguments*)threadargs)->socket, ((ThreadArguments*)threadargs)->mailspool_path);
            }else if(operation == "DEL"){
                Delete(((ThreadArguments*)threadargs)->socket, ((ThreadArguments*)threadargs)->mailspool_path);
            }else if(operation == "QUIT"){
                Quit(((ThreadArguments*)threadargs)->socket, username);
            }
        }
    }

    return 0;
}

//when server gets signal --> join all threads and close all sockets
void signal_handler(int signal){
    cout << "Got Signal: " << signal << endl;
    cout << "Shutting down mail server!" << endl;

    cout << "No. of Threads: " << threadPool.size() << endl;
    if(threadPool.size() > 0){
        for(auto &thread : threadPool){
            pthread_cancel(thread);
        }
    }

    if(socketPool.size() > 0){
        for(auto &socket : socketPool){
            if(socket > 0){
                close(socket);
            }
        }
    }

    close(server_fd);
    exit(signal);
}

int main(int argc, char *argv[])
{
    if(argc<3){
        cerr << "Invalid number of arguments\nUsage: ./filename <port> <mail-spool-directory>" << endl;
        return -1;
    }

    struct sockaddr_in address;
    string message;
    int opt = 1;
    int port = atoi(argv[1]);
    filesystem::path mailspool_path = filesystem::current_path() / argv[2];

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

    //TODO: Kritische Bereiche mit Mutex absichern (bspw. current id file)
    while(true){
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen)) < 0) {
            perror("accept");
            //TODO: glaube exit hier keine option, da man ja nicht will dass der ganze server down geht wenn sich ein client nicht verbinden kann
            exit(EXIT_FAILURE);
        }
        pthread_t newThread;
        ThreadArguments* threadargs = new ThreadArguments(mailspool_path, new_socket);

        pthread_create(&newThread, NULL, ClientHandler, (void*)threadargs);
        threadPool.push_back(newThread);

        //SIGINT = CTRL+C
        (void) signal(SIGINT, signal_handler);
    }

    return 0;
}

