#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <map>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <boost/process.hpp>
#include <signal.h>
#include <chrono>
#include <iomanip>
#include "streampeerbuffer.hpp"

using namespace std;
namespace bp = boost::process;
typedef unordered_map<string, string> Env;

template<class T1, class T2>
inline bool in_map(T1& map, const T2& object) {
    return map.find(object) != map.end();
}

template<class T1, class T2>
inline bool in_vec(T1& vec, const T2& object) {
    return find(vec.begin(), vec.end(), object) != vec.end();
}

unsigned int uid;

struct Process {
    std::string command;
    bp::child* child;
    bool running = true;
    Env env;
    string working_dir;
    unsigned int restarts = 0;
};

enum class Packet {
    Run = 0,
    Delete = 1,
    Stop = 2,
    List = 3,
    Start = 4
};

mutex data_mtx;
map<unsigned int, Process*> processes;
const char* home = getenv("HOME");
string socket_path;

unsigned int alloc_id() {
    for (;;) {
        if (!in_map(processes, uid++)) {
            return uid - 1;
        }
    }
}

void signal_handler(int signum, siginfo_t *siginfo, void *context) {
    cout << "fprocd-signal_handler: Signal (" << signum << ") received from process " <<
        (long) siginfo->si_pid << endl;
    if (signum != SIGPIPE) {
        unlink(socket_path.c_str());
        data_mtx.lock();
        for (const auto& process : processes) {
            unsigned int pid = process.second->child->id();
            bp::system("/usr/bin/pkill -TERM -P " + to_string(pid));
            cout << "fprocd-signal_handler: Killed process "
                << pid << endl;
        }
        data_mtx.unlock();
        exit(signum);
    }
}

std::vector<std::string> string_split(const std::string& str) {
    std::vector<std::string> result;
    std::istringstream iss(str);
    for (std::string s; iss >> s;)
        result.push_back(s);
    return result;
}

void launch_process(Process* proc) {
    vector<string> command = {"-i", "--chdir=" + proc->working_dir};
    for (const auto& var : proc->env) {
        command.push_back(var.first + "=" + var.second);
    }
    vector<string> actual_command = {"/bin/sh", "-c", proc->command};
    command.insert(command.end(), actual_command.begin(), actual_command.end());
    proc->child = new bp::child("/usr/bin/env", command);
    cout << "fprocd-launch_process: Launched process with pid " << proc->child->id() << endl;
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
            case (int) Packet::Run: {
                data_mtx.lock();
                Process *new_proc = new Process;
                new_proc->command = buf.get_string();
                unsigned int id;
                if (buf.get_u8() == 1) {
                    id = buf.get_u32();
                    if (in_map(processes, id)) {
                        bp::system("/usr/bin/pkill -TERM -P " + to_string(processes[id]->child->id()));
                        delete processes[id]->child;
                        delete processes[id];
                    }
                } else {
                    id = alloc_id();
                }
                unsigned int env_size = buf.get_u32();
                for (unsigned i = 0; i<env_size; i++) {
                    string key = buf.get_string();
                    string value = buf.get_string();
                    new_proc->env[key] = value;
                }
                new_proc->working_dir = buf.get_string();
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
            case (int) Packet::Delete: { // delete
                unsigned int id = buf.get_u32();
                data_mtx.lock();
                if (!in_map(processes, id)) {
                    buf.data_array = std::vector<unsigned char>();
                    buf.offset = 0;
                    buf.put_u8(1);
                    buf.put_string("That process does not exist");
                    write(socket, buf.data_array.data(), buf.data_array.size());
                    data_mtx.unlock();
                    break;
                }
                bp::system("/usr/bin/pkill -TERM -P " + to_string(processes[id]->child->id()));
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
            case (int) Packet::Stop: {
                unsigned int id = buf.get_u32();
                data_mtx.lock();
                if (!in_map(processes, id)) {
                    buf.data_array = std::vector<unsigned char>();
                    buf.offset = 0;
                    buf.put_u8(1);
                    buf.put_string("That process does not exist");
                    write(socket, buf.data_array.data(), buf.data_array.size());
                    data_mtx.unlock();
                    break;
                }
                bp::system("/usr/bin/pkill -TERM -P " + to_string(processes[id]->child->id()));
                buf.data_array = std::vector<unsigned char>();
                buf.offset = 0;
                buf.put_u8(0);
                write(socket, buf.data_array.data(), 1);
                processes[id]->running = false;
                data_mtx.unlock();
                break;
            }
            case (int) Packet::List: {
                data_mtx.lock();
                buf.data_array = std::vector<unsigned char>();
                buf.offset = 0;
                buf.put_u32(processes.size());
                for (auto const& process : processes) {
                    buf.put_u32(process.first);
                    buf.put_string(process.second->command);
                    buf.put_u32(process.second->child->id());
                    buf.put_u8(process.second->running);
                    buf.put_u32(process.second->restarts);
                }
                write(socket, buf.data_array.data(), buf.data_array.size());
                data_mtx.unlock();
                break;
            }
            case (int) Packet::Start: {
                unsigned int id = buf.get_u32();
                data_mtx.lock();
                if (!in_map(processes, id)) {
                    buf.data_array = std::vector<unsigned char>();
                    buf.offset = 0;
                    buf.put_u8(1);
                    buf.put_string("That process does not exist");
                    write(socket, buf.data_array.data(), buf.data_array.size());
                    data_mtx.unlock();
                    break;
                }
                bp::system("/usr/bin/pkill -TERM -P " + to_string(processes[id]->child->id()));
                delete processes[id]->child;
                launch_process(processes[id]);
                processes[id]->restarts++;
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
                process.second->restarts++;
            }
        }
        data_mtx.unlock();
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
}

int main(int argc, char **argv) {
    cout << "fprocd: For help, run `fproc help`" << endl;

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_sigaction = &signal_handler;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGPIPE, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGHUP, &act, NULL);

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
    if (::bind(server_fd, (struct sockaddr*) &address, 
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
