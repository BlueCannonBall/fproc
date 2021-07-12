all:
		cd daemon && make
		cd cli && cargo build --release

.PHONY: install clean

install:
		cp ./daemon/fprocd /usr/local/bin
		cp ./cli/target/release/fproc /usr/local/bin
		strip /usr/local/bin/fproc

clean:
		rm -rf ./daemon/fprocd
		rm -rf ./cli/target
