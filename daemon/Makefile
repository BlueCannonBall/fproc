CC = g++
CFLAGS = -Wall -Wno-unused-result -s -O2 -pthread -lboost_system
TARGET = fprocd

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

clean:
	$(RM) $(TARGET)