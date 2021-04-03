CC = g++
CFLAGS = -Wall -s -O2 -pthread -std=c++11
TARGET = fprocd

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

clean:
	$(RM) $(TARGET)
