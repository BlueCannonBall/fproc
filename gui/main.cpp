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
#include "gtkmm/filechooser.h"
#include "gtkmm/filechooserbutton.h"
#include "streampeerbuffer.hpp"

#define MESSAGE_SIZE 65536

using namespace std;

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

struct Result {
    int code = 0;
    string error;
};

struct Process {
    unsigned int id;
    string name;
    unsigned int pid;
    bool running;
    unsigned int restarts;
};

int open_fproc_sock() {
    string socket_path;
    if (argc > 1) {
        socket_path = argv[1];
    } else if (home) {
        socket_path = string(home) + "/.fproc.sock";
    } else {
        cout << "fproc-gui-open_fproc_socket: Error: Failed to find HOME environment variable\n";
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

Result run_process(const string& name, unsigned int id = 0, bool custom_id = false) {
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
        boost::split(var_pair, string(*var), boost::is_any_of("="));
        environ_map[var_pair[0]] = var_pair[1];
    }
    buf.put_u32(environ_map.size());
    for (const auto& var : environ_map) {
        buf.put_string(var.first);
        buf.put_string(var.second);
    }
    write(sock, buf.data_array.data(), buf.size());
    close(sock);
    return Result{};
}

Result delete_process(unsigned int id) {
    int sock = open_fproc_sock();
    spb::StreamPeerBuffer buf(true);
    buf.put_u8((uint8_t) Packet::Delete);
    buf.put_u32(id);
    write(sock, buf.data_array.data(), buf.size());

    buf.reset();
    buf.data_array.resize(MESSAGE_SIZE);
    int valread = read(sock, buf.data_array.data(), buf.data_array.size());
    if (valread == 0) {
        cout << "fproc-gui-delete_process: Error: Server disconnected\n";
        return Result{1, "Server disconnected"};
    }
    buf.data_array.resize(valread);
    close(sock);

    uint8_t code = buf.get_u8();
    string error;
    if (code) {
        error = buf.get_string();
        cout << "fproc-gui-delete_process: Error: " << error << endl;
    }
    return Result{code, error};
}

Result stop_process(unsigned int id) {
    int sock = open_fproc_sock();
    spb::StreamPeerBuffer buf(true);
    buf.put_u8((uint8_t) Packet::Stop);
    buf.put_u32(id);
    write(sock, buf.data_array.data(), buf.size());

    buf.reset();
    buf.data_array.resize(MESSAGE_SIZE);
    int valread = read(sock, buf.data_array.data(), buf.data_array.size());
    if (valread == 0) {
        cout << "fproc-gui-stop_process: Error: Server disconnected\n";
        return Result{1, "Server disconnected"};
    }
    buf.data_array.resize(valread);
    close(sock);

    uint8_t code = buf.get_u8();
    string error;
    if (code) {
        error = buf.get_string();
        cout << "fproc-gui-stop_process: Error: " << error << endl;
    }
    return Result{code, error};
}

Result get_processes(vector<Process>& processes) {
    int sock = open_fproc_sock();
    spb::StreamPeerBuffer buf(true);
    buf.put_u8((uint8_t) Packet::Get);
    write(sock, buf.data_array.data(), buf.size());

    buf.reset();
    buf.data_array.resize(MESSAGE_SIZE);
    int valread = read(sock, buf.data_array.data(), buf.data_array.size());
    if (valread == 0) {
        cout << "fproc-gui-get_processes: Error: Server disconnected\n";
        return Result{1, "Server disconnected"};
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
    return Result{0};
}

Result start_process(unsigned int id) {
    int sock = open_fproc_sock();
    spb::StreamPeerBuffer buf(true);
    buf.put_u8((uint8_t) Packet::Start);
    buf.put_u32(id);
    write(sock, buf.data_array.data(), buf.size());

    buf.reset();
    buf.data_array.resize(MESSAGE_SIZE);
    int valread = read(sock, buf.data_array.data(), buf.data_array.size());
    if (valread == 0) {
        cout << "fproc-gui-start_process: Error: Server disconnected\n";
        return Result{1, "Server disconnected"};
    }
    buf.data_array.resize(valread);
    close(sock);

    uint8_t code = buf.get_u8();
    string error;
    if (code) {
        error = buf.get_string();
        cout << "fproc-gui-start_process: Error: " << error << endl;
    }
    return Result{code, error};
}

class FprocModelColumns: public Gtk::TreeModel::ColumnRecord {
    public:
        Gtk::TreeModelColumn<unsigned int> id;
        Gtk::TreeModelColumn<string> name;
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
            this->set_title("FprocGUI Run");
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

            }
        }
};

class FprocGUI: public Gtk::Window {
    public:
        FprocGUI() {
            this->set_title("FprocGUI");

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

            run_btn.set_image_from_icon_name("system-run");
            start_btn.set_image_from_icon_name("media-playback-start");
            stop_btn.set_image_from_icon_name("media-playback-stop");
            delete_btn.set_image_from_icon_name("user-trash");
            refresh_btn.set_image_from_icon_name("view-refresh");
            run_btn.signal_clicked().connect(sigc::mem_fun(this, &FprocGUI::on_run_clicked));
            refresh_btn.signal_clicked().connect(sigc::mem_fun(this, &FprocGUI::on_refresh_clicked));
            vbox.pack_start(run_btn, false, false, 0);
            vbox.pack_start(start_btn, false, false, 0);
            vbox.pack_start(stop_btn, false, false, 0);
            vbox.pack_start(delete_btn, false, false, 0);
            vbox.pack_start(refresh_btn, false, false, 0);

            start_btn.set_sensitive(false);
            stop_btn.set_sensitive(false);
            delete_btn.set_sensitive(false);

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

        Gtk::Button run_btn{"Run"};
        Gtk::Button start_btn{"Start"};
        Gtk::Button stop_btn{"Stop"};
        Gtk::Button delete_btn{"Delete"};
        Gtk::Button refresh_btn{"Refresh"};

        void on_refresh_clicked() {
            list_store->clear();
            vector<Process> processes;
            get_processes(processes);
            for (const auto& process : processes) {
                auto row = *(list_store->append());
                row[columns.id] = process.id;
                row[columns.name] = process.name;
                row[columns.pid] = process.pid;
                row[columns.running] = process.running;
                row[columns.restarts] = process.restarts;
            }
        }

        void on_run_clicked() {
            RunDialog dialog(*this);
            dialog.run();
        }
};

int main(int argc, char** argv) {
    ::argc = argc;
    ::argv = argv;

    auto app = Gtk::Application::create(argc, argv, "org.fproc.gui");
    FprocGUI fproc;

    return app->run(fproc);
}
