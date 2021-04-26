CC = g++
CFLAGS = -Wall -Wno-unused-result -s -O2 -pthread -lboost_system
TARGET = fprocd

$(TARGET): $(TARGET).cpp streampeerbuffer.hpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

clean:
	$(RM) $(TARGET)