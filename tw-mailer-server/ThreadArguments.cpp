#include <filesystem>
#include <iostream>

using namespace std;

class ThreadArguments{
    public:
        filesystem::path mailspool_path;
        int socket;
        ThreadArguments(filesystem::path path, int socket){
            this->mailspool_path = path;
            this->socket = socket;
        }
};