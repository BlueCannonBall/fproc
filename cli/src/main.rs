use clap::{App, Arg, SubCommand};

use std::io::prelude::*;
use std::net::TcpStream;

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
                        println!("fproc-run: Command failed: {}", buf.get_utf8());
                    }
                }
            }
        },
        None => println!("No subcommand was used"),
        _ => println!("Some other subcommand was used"),
    }

    Ok(())
}
