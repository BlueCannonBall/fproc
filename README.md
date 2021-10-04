![alt text](https://raw.githubusercontent.com/BlueCannonBall/fproc/main/fproc.png)

[![forthebadge](https://forthebadge.com/images/badges/made-with-c-plus-plus.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/made-with-rust.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/designed-in-inkscape.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/powered-by-black-magic.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/check-it-out.svg)](https://forthebadge.com)

<img src="https://img.shields.io/github/license/BlueCannonBall/fproc?color=blue"> <img src="https://img.shields.io/tokei/lines/github/BlueCannonBall/fproc?color=green"> <img src="https://img.shields.io/github/languages/top/BlueCannonBall/fproc?color=red"> <img src="https://img.shields.io/github/repo-size/BlueCannonBall/fproc?color=purple">

# `fproc`

A process manager for Linux written in C++ and Rust.

## Motivation

After setting up cpu/memory limits for all the processes running on my server, I was astonished to find that the memory usage was still over 120 MB. After doing some probing, I found that `pm2`, a process manager, was using more CPU/memory than all my services combined!

## Usage (CLI)

```
$ fproc help
fproc

USAGE:
    fproc [SUBCOMMAND]

FLAGS:
    -h, --help       Prints help information
    -V, --version    Prints version information

SUBCOMMANDS:
    delete     Delete a process
    help       Prints this message or the help of the given subcommand(s)
    list       List all managed processes.
    restart    (Re)start a process
    run        Run a process
    stop       Stop a process
```

## Building & Installing

When run from the root folder of this repo, the commands below compile and install the `fproc` daemon, CLI, and GUI. The daemon, CLI, and GUI can be compiled and installed separately from each other using the makefiles provided in their respective directories.

```
$ make
# make install
```

_Note that the `fproc` daemon depends on the Boost C++ Libraries_

_Note that the `fproc` GUI depends on the Boost C++ Libraries and gtkmm 3.0_

_Note that `#` denotes a root shell, while `$` denotes a regular shell._

### One-liner for Debian

The command below compiles and installs the `fproc` daemon, CLI, and GUI on Debian systems.

```sh
git clone https://github.com/BlueCannonBall/fproc && cd fproc && sudo apt-get install libgtk-3-0 libgtkmm-3.0-dev libboost-all-dev build-essential -y && curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y && source ~/.cargo/env && make -j && sudo make install && cd ..
```

## Downloading & Installing from GitHub Actions

`fproc` has a GitHub action which automatically builds the daemon, CLI, and GUI whenever `fproc` is updated. `fproc` can be installed from GitHub Actions by downloading the latest build artifacts and running the install script bundled with the artifacts.

## Installing the JavaScript framework

Go to https://github.com/BlueCannonBall/fproc/tree/main/fprocjs
