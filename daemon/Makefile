CC = g++
CFLAGS = -Wall -s -O2 -static -pthread -lboost_system
TARGET = fprocd

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

clean:
	$(RM) $(TARGET)
