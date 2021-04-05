use clap::{App, Arg, SubCommand};

use std::io::prelude::*;
use std::net::TcpStream;
use std::process;

mod binary;
mod packet_ids;

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
        None => println!("fproc: The fproc cli cannot run the fproc server yet!"),
        _ => println!("fproc: Error: Unknown option")
    }

    Ok(())
}
