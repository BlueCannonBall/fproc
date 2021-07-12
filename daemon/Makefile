CC = g++
CFLAGS = -Wall -Wno-unused-result -s -Ofast -static-libstdc++ -Wl,-Bstatic -lboost_system -Wl,-Bdynamic -pthread
TARGET = fprocd

$(TARGET): $(TARGET).cpp streampeerbuffer.cpp streampeerbuffer.hpp
	$(CC) $@.cpp streampeerbuffer.cpp $(CFLAGS) -o $(TARGET)

clean:
	$(RM) $(TARGET)
