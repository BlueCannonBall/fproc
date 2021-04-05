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
    std::string command;
    bp::child* child;
    bool running = false;
};

mutex data_mtx;
unordered_map<unsigned int, Process*> processes;

void signal_handler(int signum) {
   cout << "Signal (" << signum << ") received.\n";
   //exit(signum);  
}

void handle_conn(int socket) {
    for (;;) {
        spb::StreamPeerBuffer buf(true);
        buf.data_array = vector<unsigned char>(1024);
        int valread = read(socket, buf.data_array.data(), buf.data_array.size());
        if (valread == 0) {
            cout << "Client disconnected." << endl;
            return;
        }
        unsigned char pckt_id = buf.get_u8();
        switch (pckt_id) {
            case 0:
            do {
                Process *new_proc = new Process;
                unsigned int id = uuid++;
                new_proc->command = buf.get_utf8();
                buf.data_array = std::vector<unsigned char>();
                buf.offset = 0;
                buf.put_u8(0);
                write(socket, buf.data_array.data(), 1);
                new_proc->child = new bp::child(new_proc->command);
                data_mtx.lock();
                processes[id] = new_proc;
                new_proc->running = true;
                data_mtx.unlock();
                break;
            } while (0);
            case 1:
            do {
                unsigned int id = buf.get_u32();
                data_mtx.lock();
                if (processes.find(id) == processes.end()) {
                    buf.data_array = std::vector<unsigned char>();
                    buf.offset = 0;
                    buf.put_u8(1);
                    buf.put_utf8("That process does not exist.");
                    write(socket, buf.data_array.data(), buf.data_array.size());
                    data_mtx.unlock();
                    break;
                }
                processes[id]->child->terminate();
                buf.data_array = std::vector<unsigned char>();
                buf.offset = 0;
                buf.put_u8(0);
                write(socket, buf.data_array.data(), 1);
                processes[id]->running = false;
                delete processes[id]->child;
                delete processes[id];
                processes.erase(id);
                data_mtx.unlock();
                break;
            } while(0);
            case 2:
            do {
                unsigned int id = buf.get_u32();
                data_mtx.lock();
                if (processes.find(id) == processes.end()) {
                    buf.data_array = std::vector<unsigned char>();
                    buf.offset = 0;
                    buf.put_u8(1);
                    buf.put_utf8("That process does not exist.");
                    write(socket, buf.data_array.data(), buf.data_array.size());
                    data_mtx.unlock();
                    break;
                }
                processes[id]->child->terminate();
                buf.data_array = std::vector<unsigned char>();
                buf.offset = 0;
                buf.put_u8(0);
                write(socket, buf.data_array.data(), 1);
                processes[id]->running = false;
                data_mtx.unlock();
                break;
            } while (0);
            case 3:
            do {
                data_mtx.lock();
                buf.data_array = std::vector<unsigned char>();
                buf.offset = 0;
                buf.put_u32(processes.size());
                for (auto const& process : processes) {
                    buf.put_u32(process.first);
                    buf.put_utf8(process.second->command);
                    buf.put_u32(process.second->child->id());
                    buf.put_u8(process.second->running);
                }
                data_mtx.unlock();
            } while (0);
        }
    }
}

void maintain_procs() {
    for (;;) {
        data_mtx.lock();
        for (auto const& process : processes) {
            cout << process.second->child->running() << endl;
        }
        data_mtx.unlock();
    }
}

int main(int argc, char **argv) {
    signal(SIGPIPE, signal_handler); 
    
    int port;
    if (argc <= 1) {
        port = 11881;
    } else {
        port = atoi(argv[1]);
    }

    int server_fd, new_socket;
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
    thread(maintain_procs).detach();
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
