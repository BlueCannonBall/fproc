all:
	cd daemon && make
	cd cli && make
	cd gui && make

.PHONY: install clean

install:
	sh install.sh

clean:
	rm -f ./daemon/fprocd
	rm -rf ./cli/target
