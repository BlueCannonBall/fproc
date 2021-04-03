#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <boost/process.hpp>
#include <signal.h>
#include "streampeerbuffer.hpp"

using namespace std;
namespace bp = boost::process;

unsigned int uuid;

struct Process {
    unsigned int id;
    unsigned long int pid;
    std::string command;
};

mutex data_mtx;
unordered_map<unsigned int, Process*> processes;

void handle_conn(int socket) {
    for (;;) {
        spb::StreamPeerBuffer buf(true);
        read(socket, buf.data_array.data(), 1024);
        unsigned char pckt_id = buf.get_u8();
        switch (pckt_id) {
            case 0:
            do {
                Process *new_proc = new Process;
                unsigned int id = uuid++;
                new_proc->id = id;
                new_proc->command = buf.get_utf8();
                buf.data_array = std::vector<unsigned char>();
                buf.offset = 0;
                buf.put_u8(0);
                write(socket, buf.data_array.data(), 1);
                processes[id] = new_proc;
                new_proc->pid = bp::child(new_proc->command).id();
                break;
            } while (0);
            case 1:
            do {
                unsigned int id = buf.get_u32();
                kill(processes[id]->pid, SIGTERM);
                buf.data_array = std::vector<unsigned char>();
                buf.offset = 0;
                buf.put_u8(0);
                write(socket, buf.data_array.data(), 1);
                delete processes[id];
                processes.erase(id);
                break;
            } while(0);
        }
    }
}

int main(int argc, char **argv) {
    int port;
    if (argc <= 1) {
        port = 11881;
    } else {
        port = atoi(argv[1]);
    }

    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
       
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
       
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
       
    if (bind(server_fd, (struct sockaddr*) &address, 
                                 sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    cout << "Listening on port " << port << endl;
    for (;;) {
        if ((new_socket = accept(server_fd, (struct sockaddr*) &address, 
                           (socklen_t*) &addrlen))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        cout << "Recieved new connection.\n";
        thread(handle_conn, new_socket).detach();
    }
    return 0;
}
