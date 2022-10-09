CXX = g++
CXXFLAGS = -fdiagnostics-color=always -Wall -Wno-unused-result -g -flto -static-libstdc++ -Wl,-Bstatic -lboost_system -lboost_filesystem -Wl,-Bdynamic -lpthread
TARGET = fprocd

$(TARGET): main.cpp streampeerbuffer.cpp streampeerbuffer.hpp
	$(CXX) $< streampeerbuffer.cpp $(CXXFLAGS) -o $@

.PHONY: clean install

install:
	cp $(TARGET) /usr/local/bin

clean:
	rm -f $(TARGET)
