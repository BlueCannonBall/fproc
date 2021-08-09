CC = g++
CFLAGS = -Wall -Wno-unused-result -s -O3 -pthread -Wl,-Bstatic -lboost_system `pkg-config gtkmm-3.0 --cflags`
OBJDIR = obj
TARGET = fproc-gui

$(TARGET): $(OBJDIR)/main.o $(OBJDIR)/streampeerbuffer.o
	$(CC) $^ $(CFLAGS) -Wl,-Bdynamic `pkg-config gtkmm-3.0 --libs` -o $@

$(OBJDIR)/main.o: main.cpp streampeerbuffer.hpp
	@mkdir -p $(OBJDIR)
	$(CC) -c $< $(CFLAGS) -o $@

$(OBJDIR)/streampeerbuffer.o: streampeerbuffer.cpp streampeerbuffer.hpp
	@mkdir -p $(OBJDIR)
	$(CC) -c $< $(CFLAGS) -o $@

.PHONY: clean install

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)

install:
	cp $(TARGET) /usr/local/bin
	cp fproc.desktop /usr/local/share/applications
	cp ../icon.png /usr/local/share/icons/hicolor/48x48/apps/fproc.png
