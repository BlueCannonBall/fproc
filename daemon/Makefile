CC = g++
CFLAGS = -fdiagnostics-color=always -Wall -Wno-unused-result -s -Ofast -static-libstdc++ -Wl,-Bstatic -lboost_system -Wl,-Bdynamic -lpthread
TARGET = fprocd

$(TARGET): main.cpp streampeerbuffer.cpp streampeerbuffer.hpp
	$(CC) $< streampeerbuffer.cpp $(CFLAGS) -o $@

.PHONY: clean install

install:
	cp ./fprocd /usr/local/bin

clean:
	rm -f $(TARGET)
