all:
	cd daemon && make -j
	cd cli && make -j
	cd gui && make -j

.PHONY: install clean

install:
	sh install.sh

clean:
	rm -f ./daemon/fprocd ./gui/fproc-gui
	rm -rf ./cli/target ./gui/obj
