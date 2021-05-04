CC = g++
CFLAGS = -Wall -Wno-unused-result -s -Ofast -pthread -lboost_system
TARGET = fprocd

$(TARGET): $(TARGET).cpp streampeerbuffer.hpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

clean:
	$(RM) $(TARGET)