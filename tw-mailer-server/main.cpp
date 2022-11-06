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

// static ssize_t
// my_read (int fd, char *ptr)
// {
//     static int read_cnt = 0 ;
//     static char *read_ptr ;
//     static char read_buf[MAXLINE] ;
//     if (read_cnt <= 0) {
//         again:
//         if ( (read_cnt = read(fd,read_buf,sizeof(read_buf))) < 0) {
//             if (errno == EINTR)
//                 goto again ;
//             return (-1);
//         } else if (read_cnt == 0)
//             return (0);
//         read_ptr = read_buf;
//     };

//     read_cnt--;
//     *ptr = *read_ptr++;
//     return (1);
// }
// ssize_t readline (int fd, void *vptr, size_t maxlen)
// {
//     ssize_t n, rc;
//     char c, *ptr;
//     ptr = vptr;
//     for (n = 1 ; n < maxlen ; n++) {
//         if ( (rc = my_read(fd,&c)) == 1 ) {
//             *ptr++ = c;
//             if (c == '\n')
//                 break ; // newline is stored
//         } else if (rc == 0) {
//             if (n == 1)
//                 return (0); // EOF, no data read
//             else
//                 break; // EOF, some data was read
//         } else
//             return (-1); // error, errno set by read() in my_read()
//     };
//     *ptr = 0; // null terminate
//     return (n);
// } 


void messageToFile(string message, filesystem::path path){
    fstream myFile(path / "current_id.txt");
    myFile << "1";

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
        
    }

    // Function to read incoming message
    while((n=recv(new_socket, buffer, sizeof(buffer), 0))>0){
        message.append(buffer, buffer+n);
        messageToFile(message, mailspool_userDir);
        messageTokens = splitInputMessage(message);

        for(auto const &tokens : messageTokens){
            cout << tokens << endl;
        }

        //messageToFile(message, mailspool_userDir);
        message.clear();
        messageTokens.clear();
    }

    return 0;
}

