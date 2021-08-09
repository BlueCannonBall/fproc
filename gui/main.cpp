#include <iostream>
#include <string>
#include <gtkmm.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <unordered_map>
#include <array>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <atomic>
#include <boost/process.hpp>
#include <thread>
#include <chrono>
#include <sys/stat.h>
#include <fcntl.h>
#include "streampeerbuffer.hpp"

#define MESSAGE_SIZE 65536

using namespace std;
namespace bp = boost::process;

int argc;
char** argv;
const char* home = getenv("HOME");

enum class Packet {
    Run = 0,
    Delete = 1,
    Stop = 2,
    Get = 3,
    Start = 4
};

struct Error {
    int code = 0;
    string error;
};

struct Process {
    unsigned int id;
    string name;
    unsigned int pid;
    bool running;
    unsigned int restarts;

    bool operator==(const Process& p) {
        return (
            id == p.id &&
            name == p.name &&
            pid == p.pid &&
            running == p.running &&
            restarts == p.restarts
        );
    }

    bool operator!=(const Process& p) {
        return !(
            id == p.id &&
            name == p.name &&
            pid == p.pid &&
            running == p.running &&
            restarts == p.restarts
        );
    }
};

int is_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

bool is_number(string s) {
    for (unsigned i = 0; i < s.length(); i++)
        if (isdigit(s[i]) == false)
            return false;

    return true;
}

int open_fproc_sock() {
    string socket_path;
    if (argc > 1) {
        socket_path = argv[1];
    } else if (home) {
        socket_path = string(home) + "/.fproc.sock";
    } else {
        cout << "fproc-gui-open_fproc_socket: Error: HOME variable not present in environment\n";
        exit(EXIT_FAILURE);
    }

    int sock = 0;
    struct sockaddr_un address;

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket(2)");
        exit(EXIT_FAILURE);
    }

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, socket_path.c_str());

    if (connect(sock, (struct sockaddr*) &address, sizeof(struct sockaddr_un)) == 0) {
        return sock;
    } else {
        perror("connect(2)");
        exit(EXIT_FAILURE);
        return -1;
    }
}

Error run_process(const string& name, const string& working_dir, unsigned int id = 0, bool custom_id = false) {
    int sock = open_fproc_sock();
    spb::StreamPeerBuffer buf(true);
    buf.put_u8((uint8_t) Packet::Run);
    buf.put_string(name);
    buf.put_u8(custom_id);
    if (custom_id) {
        buf.put_u32(id);
    }

    unordered_map<string, string> environ_map;
    for (char** var = environ; *var != 0; var++) {
        vector<string> var_pair;
        std::string var_str(*var);
        boost::split(var_pair, var_str, boost::is_any_of("="));
        environ_map[var_pair[0]] = var_pair[1];
    }
    buf.put_u32(environ_map.size());
    for (const auto& var : environ_map) {
        buf.put_string(var.first);
        buf.put_string(var.second);
    }
    buf.put_string(working_dir);
    write(sock, buf.data(), buf.size());
    close(sock);
    return Error{};
}

Error delete_process(unsigned int id) {
    int sock = open_fproc_sock();
    spb::StreamPeerBuffer buf(true);
    buf.put_u8((uint8_t) Packet::Delete);
    buf.put_u32(id);
    write(sock, buf.data(), buf.size());

    buf.reset();
    buf.data_array.resize(MESSAGE_SIZE);
    int valread = read(sock, buf.data(), buf.data_array.size());
    if (valread == 0) {
        cout << "fproc-gui-delete_process: Error: Server disconnected\n";
        return Error{1, "Server disconnected"};
    }
    buf.data_array.resize(valread);
    close(sock);

    uint8_t code = buf.get_u8();
    string error;
    if (code) {
        error = buf.get_string();
        cout << "fproc-gui-delete_process: Error: " << error << endl;
    }
    return Error{code, error};
}

Error stop_process(unsigned int id) {
    int sock = open_fproc_sock();
    spb::StreamPeerBuffer buf(true);
    buf.put_u8((uint8_t) Packet::Stop);
    buf.put_u32(id);
    write(sock, buf.data(), buf.size());

    buf.reset();
    buf.data_array.resize(MESSAGE_SIZE);
    int valread = read(sock, buf.data(), buf.data_array.size());
    if (valread == 0) {
        cout << "fproc-gui-stop_process: Error: Server disconnected\n";
        return Error{1, "Server disconnected"};
    }
    buf.data_array.resize(valread);
    close(sock);

    uint8_t code = buf.get_u8();
    string error;
    if (code) {
        error = buf.get_string();
        cout << "fproc-gui-stop_process: Error: " << error << endl;
    }
    return Error{code, error};
}

Error get_processes(vector<Process>& processes) {
    int sock = open_fproc_sock();
    spb::StreamPeerBuffer buf(true);
    buf.put_u8((uint8_t) Packet::Get);
    write(sock, buf.data(), buf.size());

    buf.reset();
    buf.data_array.resize(MESSAGE_SIZE);
    int valread = read(sock, buf.data(), buf.data_array.size());
    if (valread == 0) {
        cout << "fproc-gui-get_processes: Error: Server disconnected\n";
        return Error{1, "Server disconnected"};
    }
    buf.data_array.resize(valread);
    close(sock);

    unsigned int len = buf.get_u32();
    for (unsigned int i = 0; i<len; i++) {
        Process process;
        process.id = buf.get_u32();
        process.name = buf.get_string();
        process.pid = buf.get_u32();
        process.running = buf.get_u8();
        process.restarts = buf.get_u32();
        processes.push_back(process);
    }
    return Error{0};
}

Error start_process(unsigned int id) {
    int sock = open_fproc_sock();
    spb::StreamPeerBuffer buf(true);
    buf.put_u8((uint8_t) Packet::Start);
    buf.put_u32(id);
    write(sock, buf.data(), buf.size());

    buf.reset();
    buf.data_array.resize(MESSAGE_SIZE);
    int valread = read(sock, buf.data(), buf.data_array.size());
    if (valread == 0) {
        cout << "fproc-gui-start_process: Error: Server disconnected\n";
        return Error{1, "Server disconnected"};
    }
    buf.data_array.resize(valread);
    close(sock);

    uint8_t code = buf.get_u8();
    string error;
    if (code) {
        error = buf.get_string();
        cout << "fproc-gui-start_process: Error: " << error << endl;
    }
    return Error{code, error};
}

class FprocModelColumns: public Gtk::TreeModel::ColumnRecord {
    public:
        Gtk::TreeModelColumn<unsigned int> id;
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<unsigned int> pid;
        Gtk::TreeModelColumn<bool> running;
        Gtk::TreeModelColumn<unsigned int> restarts;

        FprocModelColumns() {
            add(id);
            add(name);
            add(pid);
            add(running);
            add(restarts);
        }
};

class NumberEntry: public Gtk::Entry {
    public:
        virtual ~NumberEntry() {}
        void on_insert_text(const Glib::ustring& text, int* position) {
            if (contains_only_numbers(text))
                Gtk::Entry::on_insert_text(text, position);
        }

    protected:
        bool contains_only_numbers(const Glib::ustring& text) {
            for (unsigned int i = 0; i < text.length(); i++) {
                if (!Glib::Unicode::isdigit(text[i]))
                    return false;
            }
            return true;
        }
};

class RunDialog: public Gtk::Dialog {
    public:
        RunDialog(Gtk::Window& parent) {
            this->set_title("Fproc Run");
            this->set_transient_for(parent);
            vbox->set_orientation(Gtk::ORIENTATION_VERTICAL);
            vbox->set_spacing(6);

            vbox->set_margin_top(10);
            vbox->set_margin_bottom(10);
            vbox->set_margin_left(10);
            vbox->set_margin_right(10);
            vbox->pack_start(command_box, false, true, 0);
            vbox->pack_start(id_box, false, true, 0);
            vbox->pack_start(working_dir_box, false, true, 0);
            working_dir_entry.set_action(Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

            command_box.pack_start(command_label, false, false, 0);
            id_box.pack_start(id_label, false, false, 0);
            working_dir_box.pack_start(working_dir_label, false, false, 0);

            command_box.pack_start(command_entry, true, true, 0);
            id_box.pack_start(id_entry, true, true, 0);
            working_dir_box.pack_start(working_dir_entry, false, false, 0);
            if (home) {
                working_dir_entry.set_filename(home);
            } else {
                cout << "fproc-gui-RunDialog::RunDialog: Error: HOME variable not present in environment\n";
                exit(EXIT_FAILURE);
            }

            this->add_button("gtk-cancel", false);
            this->add_button("gtk-ok", true);
            this->signal_response().connect(sigc::mem_fun(this, &RunDialog::on_dialog_response));

            this->show_all();
        }
        virtual ~RunDialog() {};

    private:
        Gtk::Box* vbox = get_content_area();

        Gtk::Box command_box{Gtk::ORIENTATION_HORIZONTAL, 6};
        Gtk::Box id_box{Gtk::ORIENTATION_HORIZONTAL, 6};
        Gtk::Box working_dir_box{Gtk::ORIENTATION_HORIZONTAL, 6};

        Gtk::Label command_label{"Command*"};
        Gtk::Label id_label{"ID"};
        Gtk::Label working_dir_label{"Working Directory*"};

        Gtk::Entry command_entry;
        NumberEntry id_entry;
        Gtk::FileChooserButton working_dir_entry;

        void on_dialog_response(int response_id) {
            if (response_id) {
                Error error;
                if (id_entry.get_text().size() == 0) {
                    error = run_process(
                        command_entry.get_text(),
                        working_dir_entry.get_filename()
                    );
                } else {
                    error = run_process(
                        command_entry.get_text(),
                        working_dir_entry.get_filename(),
                        atoi(id_entry.get_text().c_str()),
                        true
                    );
                }

                if (error.code) {
                    Gtk::MessageDialog error_dialog(
                        *this,
                        error.error,
                        false,
                        Gtk::MESSAGE_ERROR,
                        Gtk::BUTTONS_OK,
                        true
                    );
                    error_dialog.run();
                }
            }
        }
};

class FprocGUI: public Gtk::Window {
    public:
        FprocGUI() {
            this->set_title("Fproc");
            this->set_default_size(640, 480);

            hbox.pack_end(vbox, false, true, 0);
            hbox.set_margin_top(10);
            hbox.set_margin_bottom(10);
            hbox.set_margin_left(10);
            hbox.set_margin_right(10);
            
            treeview.set_model(list_store);
            treeview.append_column("ID", columns.id);
            treeview.get_column(0)->set_sort_column(0);
            treeview.append_column("Name", columns.name);
            treeview.get_column(1)->set_sort_column(1);
            treeview.append_column("PID", columns.pid);
            treeview.get_column(2)->set_sort_column(2);
            treeview.append_column("Running", columns.running);
            treeview.get_column(3)->set_sort_column(3);
            treeview.append_column("Restarts", columns.restarts);
            treeview.get_column(4)->set_sort_column(4);
            hbox.pack_start(treeview, true, true, 0);
            treeview.signal_cursor_changed().connect(sigc::mem_fun(this, &FprocGUI::on_treeview_cursor_changed));

            run_btn.set_image_from_icon_name("system-run");
            start_btn.set_image_from_icon_name("media-playback-start");
            stop_btn.set_image_from_icon_name("media-playback-stop");
            delete_btn.set_image_from_icon_name("user-trash");
            refresh_btn.set_image_from_icon_name("view-refresh");
            run_btn.signal_clicked().connect(sigc::mem_fun(this, &FprocGUI::on_run_clicked));
            start_btn.signal_clicked().connect(sigc::mem_fun(this, &FprocGUI::on_start_clicked));
            stop_btn.signal_clicked().connect(sigc::mem_fun(this, &FprocGUI::on_stop_clicked));
            delete_btn.signal_clicked().connect(sigc::mem_fun(this, &FprocGUI::on_delete_clicked));
            refresh_btn.signal_clicked().connect(sigc::mem_fun(this, &FprocGUI::on_refresh_clicked));
            vbox.pack_start(run_btn, false, false, 0);
            vbox.pack_start(start_btn, false, false, 0);
            vbox.pack_start(stop_btn, false, false, 0);
            vbox.pack_start(delete_btn, false, false, 0);
            vbox.pack_start(refresh_btn, false, false, 0);

            stop_btn.set_sensitive(false);

            on_refresh_clicked();

            this->add(hbox);
            g_timeout_add_seconds(1, [](gpointer data) -> gboolean {
                ((FprocGUI*) data)->on_refresh_clicked();
                return TRUE;
            }, this);
            this->show_all();
        };
        virtual ~FprocGUI() {};

    private:
        Gtk::Box hbox{Gtk::ORIENTATION_HORIZONTAL, 10};
        Gtk::Box vbox{Gtk::ORIENTATION_VERTICAL, 6};

        FprocModelColumns columns;
        Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(columns);
        Gtk::TreeView treeview;
        vector<Process> processes;

        Gtk::Button run_btn{"Run"};
        Gtk::Button start_btn{"Start"};
        Gtk::Button stop_btn{"Stop"};
        Gtk::Button delete_btn{"Delete"};
        Gtk::Button refresh_btn{"Refresh"};

        inline void repopulate_list_store(vector<Process>& new_processes) {
            Gtk::TreeModel::Path old_path;
            Gtk::TreeViewColumn* focus_column;
            treeview.get_cursor(old_path, focus_column);
            list_store->clear();
            for (const auto& process : new_processes) {
                auto row = *(list_store->append());
                row[columns.id] = process.id;
                row[columns.name] = process.name;
                row[columns.pid] = process.pid;
                row[columns.running] = process.running;
                row[columns.restarts] = process.restarts;
                treeview.set_cursor(old_path);
            }
            processes = new_processes;
        }

        void on_treeview_cursor_changed() {
            auto row = *(treeview.get_selection()->get_selected());
            stop_btn.set_sensitive(row[columns.running]);
            if (row[columns.running]) {
                start_btn.set_label("Restart");
                start_btn.set_image_from_icon_name("media-seek-backward");
            } else {
                start_btn.set_label("Start");
                start_btn.set_image_from_icon_name("media-playback-start");
            }
        }

        void on_refresh_clicked() {
            vector<Process> new_processes;
            get_processes(new_processes);
            if (processes.size() != new_processes.size()) {
                repopulate_list_store(new_processes);
            } else {
                for (unsigned i = 0; i<processes.size(); i++) {
                    if (processes[i] != new_processes[i]) {
                        repopulate_list_store(new_processes);
                        break;
                    }
                }
            }
        }

        void on_run_clicked() {
            RunDialog dialog(*this);
            dialog.run();
        }

        void on_start_clicked() {
            auto row = *(treeview.get_selection()->get_selected());

            Error error = start_process(row[columns.id]);
            if (error.code) {
                Gtk::MessageDialog error_dialog(
                    *this,
                    error.error,
                    false,
                    Gtk::MESSAGE_ERROR,
                    Gtk::BUTTONS_OK,
                    true
                );
                error_dialog.run();
            } else {
                on_refresh_clicked();
            }
        }

        void on_stop_clicked() {
            auto row = *(treeview.get_selection()->get_selected());

            Error error = stop_process(row[columns.id]);
            if (error.code) {
                Gtk::MessageDialog error_dialog(
                    *this,
                    error.error,
                    false,
                    Gtk::MESSAGE_ERROR,
                    Gtk::BUTTONS_OK,
                    true
                );
                error_dialog.run();
            } else {
                on_refresh_clicked();
            }
        }

        void on_delete_clicked() {
            auto row = *(treeview.get_selection()->get_selected());

            Error error = delete_process(row[columns.id]);
            if (error.code) {
                Gtk::MessageDialog error_dialog(
                    *this,
                    error.error,
                    false,
                    Gtk::MESSAGE_ERROR,
                    Gtk::BUTTONS_OK,
                    true
                );
                error_dialog.run();
            } else {
                on_refresh_clicked();
            }
        }
};

int main(int argc, char** argv) {
    ::argc = argc;
    ::argv = argv;

    std::vector<string> procfs_folders;
    DIR* procdir = opendir("/proc");
    struct dirent* entry;
    while ((entry = readdir(procdir))) {
        if (is_file(entry->d_name) == 0 && is_number(entry->d_name)) {
            if (getpid() != atoi(entry->d_name))
                procfs_folders.push_back(std::string(entry->d_name));       
        }
    }
    closedir(procdir);

    atomic<bool> found_process(false);
    #pragma omp parallel for simd shared(found_process)
    for (unsigned process = 0; process<procfs_folders.size(); process++) {
        if (found_process)
            continue;

        struct stat statbuf;
        if (stat(("/proc/" + procfs_folders[process]).c_str(), &statbuf) == 0) {
            if (getuid() == statbuf.st_uid) {
                ifstream comm_file("/proc/" + procfs_folders[process] + "/comm");
                string comm((std::istreambuf_iterator<char>(comm_file)),
                    std::istreambuf_iterator<char>());

                if (comm == "fprocd") {
                    found_process = true;
                    continue;
                }
            }
        } else {
            perror("stat(2)");
            exit(EXIT_FAILURE);
        }
    }

    if (!found_process) {
        cout << "fproc-gui: Started daemon\n";
        bp::child(
            "fprocd",
            bp::std_out > bp::null,
            bp::std_in < bp::null,
            bp::std_err > bp::null
        ).detach();
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    auto app = Gtk::Application::create(argc, argv, "org.fproc.gui");
    FprocGUI fproc;

    return app->run(fproc);
}
