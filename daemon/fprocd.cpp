#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <boost/process.hpp>
#include <signal.h>
#include <chrono>
#include "streampeerbuffer.hpp"

using namespace std;
namespace bp = boost::process;
typedef unordered_map<string, string> Env;

unsigned int uuid;

struct Process {
    std::string command;
    bp::child* child;
    bool running = true;
    Env env;
    string working_dir;
};

mutex data_mtx;
unordered_map<unsigned int, Process*> processes;
const char* home = getenv("HOME");
string socket_path;

unsigned int allocate_id() {
    for (;;) {
        uuid++;
        if (processes.find(uuid) == processes.end()) {
            return uuid;
        }
    }
}

void signal_handler(int signum) {
    cout << "fprocd-signal_handler: Signal (" << signum << ") received\n";
    if (signum != SIGPIPE) {
        unlink(socket_path.c_str());
        exit(signum);
    }
}

std::vector<std::string> string_split(const std::string& str) {
    std::vector<std::string> result;
    std::istringstream iss(str);
    for (std::string s; iss >> s; )
        result.push_back(s);
    return result;
}

void launch_process(Process* proc) {
    vector<string> command = {"-i", "--chdir=" + proc->working_dir};
    for (const auto& var : proc->env) {
        command.push_back(var.first + "=" + var.second);
    }
    vector<string> actual_command = {"sh", "-c", proc->command};
    command.insert(command.end(), actual_command.begin(), actual_command.end());
    proc->child = new bp::child("/usr/bin/env", command);
}

void handle_conn(int socket) {
    for (;;) {
        spb::StreamPeerBuffer buf(true);
        buf.data_array = vector<unsigned char>(64000);
        int valread = read(socket, buf.data_array.data(), buf.data_array.size());
        if (valread == 0) {
            cout << "fprocd-handle_conn: Client disconnected" << endl;
            return;
        }
        unsigned char pckt_id = buf.get_u8();
        switch (pckt_id) {
            case 0: { // run
                data_mtx.lock();
                Process *new_proc = new Process;
                new_proc->command = buf.get_utf8();
                unsigned int id;
                if (buf.get_u8() == 1) {
                    id = buf.get_u32();
                    if (processes.find(id) != processes.end()) {
                        processes[id]->child->terminate();
                        delete processes[id]->child;
                        delete processes[id];
                    }
                } else {
                    id = allocate_id();
                }
                unsigned int env_size = buf.get_u32();
                for (unsigned i = 0; i<env_size; i++) {
                    string key = buf.get_utf8();
                    string value = buf.get_utf8();
                    new_proc->env[key] = value;
                }
                new_proc->working_dir = buf.get_utf8();
                buf.data_array = std::vector<unsigned char>();
                buf.offset = 0;
                buf.put_u8(0);
                write(socket, buf.data_array.data(), 1);
                launch_process(new_proc);
                processes[id] = new_proc;
                new_proc->running = true;
                data_mtx.unlock();
                break;
            }
            case 1: { // delete
                unsigned int id = buf.get_u32();
                data_mtx.lock();
                if (processes.find(id) == processes.end()) {
                    buf.data_array = std::vector<unsigned char>();
                    buf.offset = 0;
                    buf.put_u8(1);
                    buf.put_utf8("That process does not exist");
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
            }
            case 2: { // stop
                unsigned int id = buf.get_u32();
                data_mtx.lock();
                if (processes.find(id) == processes.end()) {
                    buf.data_array = std::vector<unsigned char>();
                    buf.offset = 0;
                    buf.put_u8(1);
                    buf.put_utf8("That process does not exist");
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
            }
            case 3: { // list
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
                write(socket, buf.data_array.data(), buf.data_array.size());
                data_mtx.unlock();
                break;
            }
            case 4: { // start
                unsigned int id = buf.get_u32();
                data_mtx.lock();
                if (processes.find(id) == processes.end()) {
                    buf.data_array = std::vector<unsigned char>();
                    buf.offset = 0;
                    buf.put_u8(1);
                    buf.put_utf8("That process does not exist");
                    write(socket, buf.data_array.data(), buf.data_array.size());
                    data_mtx.unlock();
                    break;
                }
                processes[id]->child->terminate();
                delete processes[id]->child;
                launch_process(processes[id]);
                buf.data_array = std::vector<unsigned char>();
                buf.offset = 0;
                buf.put_u8(0);
                write(socket, buf.data_array.data(), 1);
                processes[id]->running = true;
                data_mtx.unlock();
                break;
            }
        }
    }
}

void maintain_procs() {
    for (;;) {
        data_mtx.lock();
        for (auto const& process : processes) {
            if (!process.second->child->running() && process.second->running) {
                cout << "fprocd-maintain_procs: Process (" << process.first << ") died" << endl;
                process.second->child->join();
                delete process.second->child;
                launch_process(process.second);
            }
        }
        data_mtx.unlock();
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
}

int main(int argc, char **argv) {
    cout << "fprocd: For help, run `fproc help`" << endl;
    signal(SIGPIPE, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGHUP, signal_handler);

    if (home) {
        socket_path = std::string(home) + "/.fproc.sock";
    } else {
        cerr << "fprocd: Error: Failed to find HOME env var\n";
        exit(EXIT_FAILURE);
    }

    if (argc > 1)
        socket_path = argv[1];

    int server_fd, new_socket, len;
    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
       
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path.c_str(), sizeof(address.sun_path)-1);
    //strcpy((char*) socket_path.c_str(), address.sun_path);
    
    len = sizeof(address);
    if (bind(server_fd, (struct sockaddr*) &address, 
                                 sizeof(address))) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    cout << "fprocd: Listening on file " << socket_path << endl;
    thread(maintain_procs).detach();
    for (;;) {
        if ((new_socket = accept(server_fd, (struct sockaddr*) &address, 
                           (socklen_t*) &len)) == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        cout << "fprocd: Recieved new connection\n";
        thread(handle_conn, new_socket).detach();
    }
    return 0;
}
