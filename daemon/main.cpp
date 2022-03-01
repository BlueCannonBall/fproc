#include "streampeerbuffer.hpp"
#include <boost/process.hpp>
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <signal.h>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#define MESSAGE_SIZE       65536
#define BACKLOG            128
#define INV_PACKET_MESSAGE "Invalid packet"
#define NO_PROC_MESSAGE    "That process does not exist"

using namespace std;
namespace bp = boost::process;
typedef unordered_map<string, string> Env;

template <class T1, class T2>
inline bool in_map(const T1& map, const T2& object) {
    return map.find(object) != map.end();
}

template <class T1, class T2>
inline bool in_vec(const T1& vec, const T2& object) {
    return find(vec.begin(), vec.end(), object) != vec.end();
}

unsigned int uid;

struct Process {
    std::string command;
    bp::group group;
    unique_ptr<bp::child> child;
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

inline void kill_process(Process* proc) {
    //bp::system("/usr/bin/pkill -TERM -P" + to_string(proc->child->id()));
    try {
        proc->group.terminate();
    } catch (std::exception& e) { }
}

void launch_process(Process* proc) {
    vector<string> command = {"-i", "--chdir=" + proc->working_dir};
    for (const auto& var : proc->env) {
        command.push_back(var.first + "=" + var.second);
    }
    vector<string> actual_command = {"/bin/sh", "-c", proc->command};
    command.insert(command.end(), actual_command.begin(), actual_command.end());
    kill_process(proc);
    proc->group = bp::group();
    proc->child = std::move(make_unique<bp::child>("/usr/bin/env", command, bp::std_out > bp::null, bp::std_in<bp::null, bp::std_err> bp::null, proc->group));
    cout << "fprocd-launch_process: Launched process with pid " << proc->child->id() << endl;
}

std::vector<std::string> string_split(const std::string& str) {
    std::vector<std::string> result;
    std::istringstream iss(str);
    for (std::string s; iss >> s;)
        result.push_back(s);
    return result;
}

void signal_handler(int signum, siginfo_t* siginfo, void* context) {
    cout << "fprocd-signal_handler: Signal (" << signum << ") received from process " << (long) siginfo->si_pid << endl;
    if (signum != SIGPIPE) {
        unlink(socket_path.c_str());
        data_mtx.lock();
        for (const auto& process : processes) {
            unsigned int pid = process.second->child->id();
            kill_process(process.second);
            cout << "fprocd-signal_handler: Killed process "
                 << pid << endl;
        }
        data_mtx.unlock();
        exit(signum);
    }
}

void handle_error(spb::StreamPeerBuffer& buf, int socket, const std::string& error) {
    buf.put_u8(1);
    buf.put_string(error);
    cout << "fprocd-handle_error: Sending error \"" << error << "\" to client\n";
    write(socket, buf.data(), buf.size());
}

void handle_conn(int socket) {
    for (;;) {
    listen:;
        spb::StreamPeerBuffer buf(true);
        buf.resize(MESSAGE_SIZE);
        int valread = read(socket, buf.data(), buf.size());
        if (valread == 0) {
            cout << "fprocd-handle_conn: Client disconnected\n";
            return;
        }
        buf.resize(valread);
        unsigned char pckt_id = buf.get_u8();
        switch (pckt_id) {
            case (int) Packet::Run: {
                data_mtx.lock();
                Process* new_proc = new Process;
                if (buf.get_string(new_proc->command)) {
                    buf.reset();
                    handle_error(buf, socket, INV_PACKET_MESSAGE);
                    data_mtx.unlock();
                    delete new_proc;
                    break;
                }
                unsigned int id;
                if (buf.get_u8() == 1) {
                    id = buf.get_u32();
                    if (in_map(processes, id)) {
                        kill_process(processes[id]);
                        delete processes[id];
                    }
                } else {
                    id = alloc_id();
                }
                unsigned int env_size = buf.get_u32();

                for (unsigned i = 0; i < env_size; i++) {
                    string key;
                    if (buf.get_string(key)) {
                        buf.reset();
                        handle_error(buf, socket, INV_PACKET_MESSAGE);
                        data_mtx.unlock();
                        delete new_proc;
                        goto listen;
                    }
                    string value;
                    if (buf.get_string(value)) {
                        buf.reset();
                        handle_error(buf, socket, INV_PACKET_MESSAGE);
                        data_mtx.unlock();
                        delete new_proc;
                        goto listen;
                    }
                    new_proc->env[key] = value;
                }
                if (buf.get_string(new_proc->working_dir)) {
                    buf.reset();
                    handle_error(buf, socket, INV_PACKET_MESSAGE);
                    data_mtx.unlock();
                    delete new_proc;
                    break;
                }

                launch_process(new_proc);
                processes[id] = new_proc;
                new_proc->running = true;
                buf.reset();
                buf.put_u8(0);
                write(socket, buf.data(), 1);
                data_mtx.unlock();
                break;
            }
            case (int) Packet::Delete: {
                unsigned int id = buf.get_u32();
                data_mtx.lock();
                if (!in_map(processes, id)) {
                    buf.reset();
                    handle_error(buf, socket, NO_PROC_MESSAGE);
                    data_mtx.unlock();
                    break;
                }
                kill_process(processes[id]);
                processes[id]->running = false;
                delete processes[id];
                processes.erase(id);
                buf.reset();
                buf.put_u8(0);
                write(socket, buf.data(), 1);
                data_mtx.unlock();
                break;
            }
            case (int) Packet::Stop: {
                unsigned int id = buf.get_u32();
                data_mtx.lock();
                if (!in_map(processes, id)) {
                    buf.reset();
                    handle_error(buf, socket, NO_PROC_MESSAGE);
                    data_mtx.unlock();
                    break;
                }
                kill_process(processes[id]);
                processes[id]->running = false;
                buf.reset();
                buf.put_u8(0);
                write(socket, buf.data(), 1);
                data_mtx.unlock();
                break;
            }
            case (int) Packet::List: {
                data_mtx.lock();
                buf.reset();
                buf.put_u32(processes.size());
                for (const auto& process : processes) {
                    buf.put_u32(process.first);
                    buf.put_string(process.second->command);
                    buf.put_u32(process.second->child->id());
                    buf.put_u8(process.second->running);
                    buf.put_u32(process.second->restarts);
                }
                write(socket, buf.data(), buf.size());
                data_mtx.unlock();
                break;
            }
            case (int) Packet::Start: {
                unsigned int id = buf.get_u32();
                data_mtx.lock();
                if (!in_map(processes, id)) {
                    buf.reset();
                    handle_error(buf, socket, NO_PROC_MESSAGE);
                    data_mtx.unlock();
                    break;
                }
                kill_process(processes[id]);
                launch_process(processes[id]);
                processes[id]->restarts++;
                processes[id]->running = true;
                buf.reset();
                buf.put_u8(0);
                write(socket, buf.data(), 1);
                data_mtx.unlock();
                break;
            }
        }
    }
}

void maintain_procs() {
    for (;;) {
        data_mtx.lock();
        for (const auto& process : processes) {
            if (!process.second->child->running() && process.second->running) {
                cout << "fprocd-maintain_procs: Process (" << process.first << ") died\n";
                process.second->child->join();
                launch_process(process.second);
                process.second->restarts++;
            }
        }
        data_mtx.unlock();
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
}

int main(int argc, char** argv) {
    cout << "fprocd: For help, run `fproc help`\n";

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_sigaction = &signal_handler;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGPIPE, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGHUP, &act, NULL);

    if (argc > 1) {
        socket_path = argv[1];
    } else if (home) {
        socket_path = std::string(home) + "/.fproc.sock";
    } else {
        cerr << "fprocd: Error: HOME variable not present in environment\n";
        exit(EXIT_FAILURE);
    }

    int server_fd;
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket(2)");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path.c_str(), sizeof(address.sun_path) - 1);

    if (::bind(server_fd, (struct sockaddr*) &address, sizeof(address)) == -1) {
        perror("bind(2)");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen(2)");
        exit(EXIT_FAILURE);
    }

    cout << "fprocd: Listening on file " << socket_path << endl;
    thread(maintain_procs).detach();

    for (;;) {
        int new_socket;
        struct sockaddr_un client_address;
        int client_address_len = sizeof(address);
        if ((new_socket = accept(server_fd, (struct sockaddr*) &client_address, (socklen_t*) &client_address_len)) == -1) {
            perror("accept(2)");
            exit(EXIT_FAILURE);
        }

        cout << "fprocd: Recieved new connection\n";
        thread(handle_conn, new_socket).detach();
    }

    return 0;
}
