CXX = g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra $(CPPFLAGS)
CFLAGS   = -O2 -Wall -Wextra $(CPPFLAGS)
LIBS = $(LDFLAGS) -lavif -lwebp -lm
TARGET = colorpixels

SRCFILES = main.cc lut.cc decoder.cc process.cc
OBJS = $(SRCFILES:.cc=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# dependencies (headers used by multiple units)
main.o: lut.h process.h
lut.o: lut.h
decoder.o: decoder.h
process.o: process.h lut.h decoder.h

# compile C++ source files to object files
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# compile C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

include/stb_image.h:
	mkdir -p include
	rm -rf stb
	git clone https://github.com/nothings/stb
	rm -rf include/stb
	mv stb include
	ln -sf stb/stb_image.h include/

clean:
	rm -f $(TARGET) *.o
