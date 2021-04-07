CC = g++
CFLAGS = -lboost_filesystem -lboost_system -Wall -Wno-unused-result -s -O2 -pthread
TARGET = fprocd

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

clean:
	$(RM) $(TARGET)