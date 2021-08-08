all:
	cargo build --release
	strip target/release/fproc

.PHONY: clean install

clean:
	rm -rf target/

install:
	cp target/release/fproc /usr/local/bin
