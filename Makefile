all: daemon cli gui

.PHONY: install clean daemon cli gui

daemon:
	cd $@ && make -j
cli:
	cd $@ && make -j
gui:
	cd $@ && make -j

install:
	sh install.sh

clean:
	rm -f ./daemon/fprocd ./gui/fproc-gui
	rm -rf ./cli/target ./gui/obj
