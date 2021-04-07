use clap::{App, Arg, SubCommand};
use prettytable::{cell, row, Cell, Row, Table};
use procfs::process::all_processes;

use std::io::prelude::*;
use std::os::unix::net::UnixStream;
use std::process::{Command, Stdio};
use std::thread;
use std::time::Duration;
use std::env;
use std::path::Path;

mod binary;
mod model;
mod packet_ids;

fn main() -> std::io::Result<()> {
    let matches = App::new("fproc")
        .subcommand(
            SubCommand::with_name("run")
                .aliases(&["up", "create", "new"])
                .about("Run a process")
                .version("0.1")
                .arg(
                    Arg::with_name("command")
                        .help("The command to run")
                        .index(1)
                        .multiple(true)
                        .required(true),
                )
                .arg(
                    Arg::with_name("id")
                        .help("The id of the process to launch")
                        .index(2)
                )
        )
        .subcommand(
            SubCommand::with_name("stop")
                .aliases(&["disable", "end"])
                .about("Stop a process")
                .version("0.1")
                .arg(
                    Arg::with_name("id")
                        .help("The process id to stop.")
                        .index(1)
                        .multiple(true)
                        .required(true),
                ),
        )
        .subcommand(
            SubCommand::with_name("restart")
                .aliases(&["start"])
                .about("(Re)start a process")
                .version("0.1")
                .arg(
                    Arg::with_name("id")
                        .help("The process id to (re)start.")
                        .index(1)
                        .multiple(true)
                        .required(true),
                ),
        )
        .subcommand(
            SubCommand::with_name("delete")
                .aliases(&["rm", "del"])
                .about("Delete a process")
                .version("0.1")
                .arg(
                    Arg::with_name("id")
                        .help("The process id to delete.")
                        .index(1)
                        .multiple(true)
                        .required(true),
                ),
        )
        .subcommand(
            SubCommand::with_name("list")
                .aliases(&["ls"])
                .about("List all managed processes.")
                .version("0.1"),
        )
        .get_matches();

    let home = env::var("HOME");
    let home = match home {
        Ok(home) => home,
        Err(_) => {
            println!("fproc: Error: HOME not present in environment");
            std::process::exit(1)
        }
    };

    let socket_path = format!("{}/.fproc.sock", home);

    let procs = all_processes().unwrap();
    let mut found_process = false;
    let my_owner = procfs::process::Process::myself().unwrap().owner;
    for process in procs {
        if process.owner == my_owner {
            let stat = process.status();
            match stat {
                Ok(status) => {
                    if status.name == "fprocd" {
                        found_process = true;
                    }
                },
                Err(_) => ()
            }
        }
    }
    if !found_process {
        if Path::new(&socket_path).exists() {
            std::fs::remove_file(&socket_path).unwrap();
        }
        Command::new("fprocd")
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .stdin(Stdio::null())
            .spawn()
            .expect("failed to execute process");
        println!("fproc: Started daemon");
        thread::sleep(Duration::from_millis(1000));
    }

    match matches.subcommand_name() {
        Some("run") => {
            if let Some(matches) = matches.subcommand_matches("run") {
                if matches.is_present("command") {
                    let mut buf = binary::StreamPeerBuffer::new();
                    buf.put_u8(packet_ids::RUN);

                    let cmd: Vec<&str> = matches.values_of("command").unwrap().collect();
                    let cmd = cmd.join(" ");
                    buf.put_utf8(cmd);
                    if matches.is_present("id") {
                        buf.put_u8(1);
                        let id = matches.value_of("id").unwrap();
                        let id = match id.parse::<u32>() {
                            Ok(v) => v,
                            Err(_) => {
                                println!("fproc-run: Error: Please supply a valid number for argument `id`");
                                std::process::exit(1)
                            }
                        };
                        buf.put_u32(id);
                    } else {
                        buf.put_u8(0);
                    }

                    // send current environment
                    let variables = env::vars();
                    let variables:Vec<_> = variables.collect();
                    buf.put_u32(variables.len() as u32);
                    for (key, value) in variables {
                        buf.put_utf8(key);
                        buf.put_utf8(value);
                    }

                    // current working directory
                    buf.put_utf8(env::current_dir().unwrap().to_string_lossy().as_ref().to_string());

                    println!("fproc-run: Wrote {} bytes to socket", buf.cursor.get_ref().len());

                    // open socket
                    let mut stream = UnixStream::connect(socket_path).unwrap();
                    stream.write_all(buf.cursor.get_ref().as_slice()).unwrap();

                    let mut read_buf = [0; 128];
                    stream.read(&mut read_buf).unwrap();
                    stream.shutdown(std::net::Shutdown::Both);

                    let mut buf = binary::StreamPeerBuffer::new();
                    buf.set_data_array(read_buf.to_vec());

                    let ok = buf.get_u8();
                    if ok == 0 {
                        let cmd: Vec<&str> = matches.values_of("command").unwrap().collect();
                        let cmd = cmd.join(" ");
                        println!("fproc-run: Successfully ran command \"{}\"", cmd);
                    } else {
                        println!("fproc-run: Error: {}", buf.get_utf8());
                        std::process::exit(1);
                    }
                }
            }
        }
        Some("stop") => {
            if let Some(matches) = matches.subcommand_matches("stop") {
                if matches.is_present("id") {
                    let mut buf = binary::StreamPeerBuffer::new();
                    buf.put_u8(packet_ids::STOP);

                    let cmd = matches.values_of("id").unwrap();
                    for id in cmd {
                        let id = match id.parse::<u32>() {
                            Ok(v) => v,
                            Err(e) => {
                                println!("fproc-stop: Error: Please supply a valid number");
                                std::process::exit(1)
                            }
                        };
                        buf.put_u32(id);
                    }

                    // open socket
                    let mut stream = UnixStream::connect(socket_path).unwrap();
                    stream.write_all(buf.cursor.get_ref().as_slice()).unwrap();

                    let mut read_buf = [0; 128];
                    stream.read(&mut read_buf).unwrap();
                    stream.shutdown(std::net::Shutdown::Both);

                    let mut buf = binary::StreamPeerBuffer::new();
                    buf.set_data_array(read_buf.to_vec());

                    let ok = buf.get_u8();
                    if ok == 0 {
                        let cmd: Vec<&str> = matches.values_of("id").unwrap().collect();
                        let cmd = cmd.join(" ");
                        println!("fproc-stop: Successfully stopped process(es) \"{}\"", cmd);
                    } else {
                        println!("fproc-stop: Error: {}", buf.get_utf8());
                        std::process::exit(1);
                    }
                }
            }
        }
        Some("restart") => {
            if let Some(matches) = matches.subcommand_matches("restart") {
                if matches.is_present("id") {
                    let mut buf = binary::StreamPeerBuffer::new();
                    buf.put_u8(packet_ids::START);

                    let cmd = matches.values_of("id").unwrap();
                    for id in cmd {
                        let id = match id.parse::<u32>() {
                            Ok(v) => v,
                            Err(e) => {
                                println!("fproc-start: Error: Please supply a valid number");
                                std::process::exit(1)
                            }
                        };
                        buf.put_u32(id);
                    }

                    // open socket
                    let mut stream = UnixStream::connect(socket_path).unwrap();
                    stream.write_all(buf.cursor.get_ref().as_slice()).unwrap();

                    let mut read_buf = [0; 128];
                    stream.read(&mut read_buf).unwrap();
                    stream.shutdown(std::net::Shutdown::Both);

                    let mut buf = binary::StreamPeerBuffer::new();
                    buf.set_data_array(read_buf.to_vec());

                    let ok = buf.get_u8();
                    if ok == 0 {
                        let cmd: Vec<&str> = matches.values_of("id").unwrap().collect();
                        let cmd = cmd.join(" ");
                        println!("fproc-start: Successfully started process(es) \"{}\"", cmd);
                    } else {
                        println!("fproc-stop: Error: {}", buf.get_utf8());
                        std::process::exit(1);
                    }
                }
            }
        }
        Some("delete") => {
            if let Some(matches) = matches.subcommand_matches("delete") {
                if matches.is_present("id") {
                    let mut buf = binary::StreamPeerBuffer::new();
                    buf.put_u8(packet_ids::DELETE);

                    let cmd = matches.values_of("id").unwrap();
                    for id in cmd {
                        let id = match id.parse::<u32>() {
                            Ok(v) => v,
                            Err(e) => {
                                println!("fproc-delete: Error: Please supply a valid number");
                                std::process::exit(1)
                            }
                        };
                        buf.put_u32(id);
                    }

                    // open socket
                    let mut stream = UnixStream::connect(socket_path).unwrap();
                    stream.write_all(buf.cursor.get_ref().as_slice()).unwrap();

                    let mut read_buf = [0; 128];
                    stream.read(&mut read_buf).unwrap();
                    stream.shutdown(std::net::Shutdown::Both);

                    let mut buf = binary::StreamPeerBuffer::new();
                    buf.set_data_array(read_buf.to_vec());

                    let ok = buf.get_u8();
                    if ok == 0 {
                        let cmd: Vec<&str> = matches.values_of("id").unwrap().collect();
                        let cmd = cmd.join(" ");
                        println!("fproc-delete: Successfully deleted process(es) \"{}\"", cmd);
                    } else {
                        println!("fproc-delete: Error: {}", buf.get_utf8());
                        std::process::exit(1);
                    }
                }
            }
        }
        Some("list") => {
            if let Some(matches) = matches.subcommand_matches("list") {
                let mut buf = binary::StreamPeerBuffer::new();
                buf.put_u8(packet_ids::LIST);

                // open socket
                let mut stream = UnixStream::connect(socket_path).unwrap();
                stream.write_all(buf.cursor.get_ref().as_slice()).unwrap();

                let mut read_buf = [0; 1024];
                stream.read(&mut read_buf).unwrap();
                stream.shutdown(std::net::Shutdown::Both);

                let mut buf = binary::StreamPeerBuffer::new();
                buf.set_data_array(read_buf.to_vec());

                let amount = buf.get_u32();
                if amount == 0 {
                    println!("fproc-list: Error: No processes found");
                    std::process::exit(1);
                }
                println!("fproc-list: Found {} process(es)", amount);

                let mut processes = vec![];

                for _ in 0..amount {
                    processes.push(model::ManagedProcess {
                        id: buf.get_u32(),
                        name: buf.get_utf8(),
                        pid: buf.get_u32(),
                        running: buf.get_u8() != 0,
                    });
                }

                let mut table = Table::new();
                table.add_row(row!["ID", "NAME", "PID", "RUNNING"]);
                for process in processes {
                    table.add_row(row![process.id, process.name, process.pid, process.running]);
                }
                table.printstd();
            }
        }
        None => println!("fproc: Run `fproc --help` for options"),
        _ => println!("fproc: Error: Unknown option"),
    }

    Ok(())
}
