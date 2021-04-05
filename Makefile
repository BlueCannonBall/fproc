all:
		cd daemon && make
		cd cli && cargo build --release

install:
		cp ./daemon/fprocd /usr/local/bin
		cp ./cli/target/release/fproc /usr/local/bin