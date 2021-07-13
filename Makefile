all:
	cd daemon && make
	cd cli && cargo build --release

.PHONY: install clean

install:
	sh install.sh

clean:
	rm -rf ./daemon/fprocd
	rm -rf ./cli/target
