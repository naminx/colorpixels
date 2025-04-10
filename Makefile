CXX = g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra $(CPPFLAGS)
CFLAGS   = -O2 -Wall -Wextra $(CPPFLAGS)
LIBS = $(LDFLAGS) -lavif -lwebp -lm
TARGET = colorpixels

SRCFILES = main.cc lut.cc decode.cc process.cc
OBJS = $(SRCFILES:.cc=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# dependencies (headers used by multiple units)
main.o: lut.hh process.hh
lut.o: lut.hh
decode.o: decode.hh
process.o: process.hh lut.hh decode.hh

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
