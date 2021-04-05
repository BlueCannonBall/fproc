use clap::{App, Arg, SubCommand};
use prettytable::{Table, Row, Cell, row, cell};

use std::io::prelude::*;
use std::net::TcpStream;
use std::process;

mod binary;
mod packet_ids;
mod model;

fn main() -> std::io::Result<()> {
    let matches = App::new("fproc")
        .subcommand(
            SubCommand::with_name("run")
                .about("Run a process")
                .version("0.1")
                .arg(
                    Arg::with_name("command")
                        .help("The command to run")
                        .index(1)
                        .multiple(true)
                        .required(true),
                ),
        )
        .subcommand(
            SubCommand::with_name("stop")
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
                .about("List all managed processes.")
                .version("0.1")
        )
        .get_matches();
    
    match matches.subcommand_name() {
        Some("run") => {
            if let Some(matches) = matches.subcommand_matches("run") {
                if matches.is_present("command") {
                    let mut buf = binary::StreamPeerBuffer::new();
                    buf.put_u8(packet_ids::RUN);
                    
                    
                    let cmd: Vec<&str> = matches.values_of("command").unwrap().collect();
                    let cmd = cmd.join(" ");
                    buf.put_utf8(cmd);

                    // open socket
                    let mut stream = TcpStream::connect("127.0.0.1:11881")?;
                    stream.write_all(buf.cursor.get_ref().as_slice())?;

                    let mut read_buf = [0; 128];
                    stream.read(&mut read_buf)?;
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
        },
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
                    let mut stream = TcpStream::connect("127.0.0.1:11881")?;
                    stream.write_all(buf.cursor.get_ref().as_slice())?;

                    let mut read_buf = [0; 128];
                    stream.read(&mut read_buf)?;
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
        },
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
                    let mut stream = TcpStream::connect("127.0.0.1:11881")?;
                    stream.write_all(buf.cursor.get_ref().as_slice())?;

                    let mut read_buf = [0; 128];
                    stream.read(&mut read_buf)?;
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
        },
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
                    let mut stream = TcpStream::connect("127.0.0.1:11881")?;
                    stream.write_all(buf.cursor.get_ref().as_slice())?;

                    let mut read_buf = [0; 128];
                    stream.read(&mut read_buf)?;
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
        },
        Some("list") => {
            if let Some(matches) = matches.subcommand_matches("list") {
                let mut buf = binary::StreamPeerBuffer::new();
                buf.put_u8(packet_ids::LIST);

                // open socket
                let mut stream = TcpStream::connect("127.0.0.1:11881")?;
                stream.write_all(buf.cursor.get_ref().as_slice())?;

                let mut read_buf = [0; 1024];
                stream.read(&mut read_buf)?;
                stream.shutdown(std::net::Shutdown::Both);

                let mut buf = binary::StreamPeerBuffer::new();
                buf.set_data_array(read_buf.to_vec());

                let amount = buf.get_u32();
                if amount == 0 {
                    println!("fproc-list: Error: No processes found");
                    std::process::exit(1);
                }
                println!("fproc-list: Found {} processes", amount);

                let mut processes = vec![];

                for _ in 0..amount {
                    processes.push(model::ManagedProcess {
                        id: buf.get_u32(),
                        name: buf.get_utf8(),
                        pid: buf.get_u32(),
                        running: buf.get_u8() != 0
                    });
                }

                let mut table = Table::new();
                table.add_row(row!["ID", "NAME", "PID", "RUNNING"]);
                for process in processes {
                    table.add_row(row![process.id, process.name, process.pid, process.running]);
                }
                table.printstd();
            }
        },
        None => println!("fproc: The fproc cli cannot run the fproc server yet!"),
        _ => println!("fproc: Error: Unknown option")
    }

    Ok(())
}
