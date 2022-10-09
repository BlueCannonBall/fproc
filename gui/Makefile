CXX = g++
CXXFLAGS = -fdiagnostics-color=always -Wall -Wno-unused-result -s -O3 -flto -pthread -fopenmp -Wl,-Bstatic -lboost_system `pkg-config gtkmm-3.0 --cflags`
OBJDIR = obj
TARGET = fproc-gui

$(TARGET): $(OBJDIR)/main.o $(OBJDIR)/streampeerbuffer.o
	$(CXX) $^ $(CXXFLAGS) -Wl,-Bdynamic `pkg-config gtkmm-3.0 --libs` -o $@

$(OBJDIR)/main.o: main.cpp streampeerbuffer.hpp
	@mkdir -p $(OBJDIR)
	$(CXX) -c $< $(CXXFLAGS) -o $@

$(OBJDIR)/streampeerbuffer.o: streampeerbuffer.cpp streampeerbuffer.hpp
	@mkdir -p $(OBJDIR)
	$(CXX) -c $< $(CXXFLAGS) -o $@

.PHONY: clean install

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)

install:
	cp $(TARGET) /usr/local/bin
	mkdir -p /usr/local/share/applications
	cp fproc.desktop /usr/local/share/applications
	mkdir -p /usr/local/share/icons/hicolor/48x48/apps
	cp ../icon.png /usr/local/share/icons/hicolor/48x48/apps/fproc.png
