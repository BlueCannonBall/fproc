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

#define BACKLOG            128
#define INV_PACKET_MESSAGE "Invalid packet"
#define NO_PROC_MESSAGE    "That process does not exist"

namespace bp = boost::process;

template <class T1, class T2>
inline bool in_map(const T1& map, const T2& object) {
    return map.find(object) != map.end();
}

template <class T1, class T2>
inline bool in_vec(const T1& vec, const T2& object) {
    return std::find(vec.begin(), vec.end(), object) != vec.end();
}

unsigned int uid;

struct Process {
    std::string command;
    bp::group group;
    std::unique_ptr<bp::child> child;
    bool running = true;
    bp::environment env;
    std::string working_dir;
    unsigned int restarts = 0;

    void launch() {
        std::vector<std::string> cmd_args = {"-c", this->command};
        this->kill();
        this->group = bp::group();
        this->child = std::make_unique<bp::child>(bp::search_path("sh"), cmd_args, this->env, bp::start_dir(this->working_dir), bp::std_out > bp::null, bp::std_in<bp::null, bp::std_err> bp::null, this->group);
        std::cout << "fprocd-Process::launch: Launched process with pid " << this->child->id() << std::endl;
    }

    inline void kill() {
        if (this->child) {
            try {
                pid_t pid = this->child->id();
                this->group.terminate();
                std::cout << "fprocd-Process::kill: Killed process with pid " << pid << std::endl;
            } catch (std::exception& e) { }
        }
    }
};

enum class Packet {
    Run = 0,
    Delete = 1,
    Stop = 2,
    List = 3,
    Start = 4
};

std::mutex data_mtx;
std::map<unsigned int, Process*> processes;
const char* home = getenv("HOME");
std::string socket_path;

unsigned int alloc_id() {
    for (;;) {
        if (!in_map(processes, uid++)) {
            return uid - 1;
        }
    }
}

std::vector<std::string> string_split(const std::string& str) {
    std::vector<std::string> result;
    std::istringstream iss(str);
    for (std::string s; iss >> s;)
        result.push_back(s);
    return result;
}

void signal_handler(int signum, siginfo_t* siginfo, void* context) {
    std::cout << "fprocd-signal_handler: Signal (" << signum << ") received from process " << (long) siginfo->si_pid << std::endl;
    if (signum != SIGPIPE) {
        unlink(socket_path.c_str());
        data_mtx.lock();
        for (const auto& process : processes) {
            process.second->kill();
        }
        data_mtx.unlock();
        exit(signum);
    }
}

void handle_error(spb::StreamPeerBuffer& buf, int socket, const std::string& error) {
    buf.put_u8(1);
    buf.put_string(error);
    buf.offset = 0;
    buf.put_u16(buf.size());
    std::cout << "fprocd-handle_error: Sending error \"" << error << "\" to client" << std::endl;
    write(socket, buf.data(), buf.size());
}

void handle_conn(int socket) {
    for (;;) {
        spb::StreamPeerBuffer buf(true);
        buf.resize(2);
        int valread = recv(socket, buf.data(), 2, MSG_WAITALL);
        if (valread == 0) {
            std::cout << "fprocd-handle_conn: Client disconnected" << std::endl;
            return;
        }
        buf.resize(2 + buf.get_u16());
        valread = recv(socket, buf.data() + 2, buf.size() - 2, MSG_WAITALL);
        if (valread == 0) {
            std::cout << "fprocd-handle_conn: Client disconnected" << std::endl;
            return;
        }
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
                        processes[id]->kill();
                        delete processes[id];
                    }
                } else {
                    id = alloc_id();
                }

                unsigned int env_size = buf.get_u32();
                for (unsigned i = 0; i < env_size; i++) {
                    std::string key;
                    if (buf.get_string(key)) {
                        buf.reset();
                        handle_error(buf, socket, INV_PACKET_MESSAGE);
                        data_mtx.unlock();
                        delete new_proc;
                        continue;
                    }
                    std::string value;
                    if (buf.get_string(value)) {
                        buf.reset();
                        handle_error(buf, socket, INV_PACKET_MESSAGE);
                        data_mtx.unlock();
                        delete new_proc;
                        continue;
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

                new_proc->launch();
                processes[id] = new_proc;
                new_proc->running = true;
                buf.reset();
                buf.put_u8(0);
                buf.offset = 0;
                buf.put_u16(buf.size());
                write(socket, buf.data(), buf.size());
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
                processes[id]->kill();
                processes[id]->running = false;
                delete processes[id];
                processes.erase(id);
                buf.reset();
                buf.put_u8(0);
                buf.offset = 0;
                buf.put_u16(buf.size());
                write(socket, buf.data(), buf.size());
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
                processes[id]->kill();
                processes[id]->running = false;
                buf.reset();
                buf.put_u8(0);
                buf.offset = 0;
                buf.put_u16(buf.size());
                write(socket, buf.data(), buf.size());
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
                buf.offset = 0;
                buf.put_u16(buf.size());
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
                processes[id]->kill();
                processes[id]->launch();
                processes[id]->restarts++;
                processes[id]->running = true;
                buf.reset();
                buf.put_u8(0);
                buf.offset = 0;
                buf.put_u16(buf.size());
                write(socket, buf.data(), buf.size());
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
            if (process.second->running && !process.second->child->running()) {
                std::cout << "fprocd-maintain_procs: Process (" << process.first << ") died" << std::endl;
                process.second->child->join();
                process.second->launch();
                process.second->restarts++;
            }
        }
        data_mtx.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

int main(int argc, char** argv) {
    std::cout << "fprocd: For help, run `fproc help`" << std::endl;

    struct sigaction act;
    memset(&act, 0, sizeof(act));
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
        std::cerr << "fprocd: Error: HOME variable not present in environment" << std::endl;
        exit(EXIT_FAILURE);
    }

    int server_fd;
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un address = {0};
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path.c_str(), sizeof(address.sun_path) - 1);

    if (::bind(server_fd, (struct sockaddr*) &address, sizeof(address)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "fprocd: Listening on socket " << socket_path << std::endl;
    std::thread(maintain_procs).detach();

    for (;;) {
        int new_socket;
        struct sockaddr_un client_address;
        int client_address_len = sizeof(address);
        if ((new_socket = accept(server_fd, (struct sockaddr*) &client_address, (socklen_t*) &client_address_len)) == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        std::cout << "fprocd: Recieved new connection" << std::endl;
        std::thread(handle_conn, new_socket).detach();
    }

    return 0;
}
