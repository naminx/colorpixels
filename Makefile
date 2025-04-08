CXX = g++
CC  = gcc
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra $(CPPFLAGS)
CFLAGS   = -O2 -Wall -Wextra $(CPPFLAGS)
LIBS = $(LDFLAGS) -lavif -lwebp -lm
TARGET = colorpixels

SRCS = colorpixels.cc
OBJS = $(SRCS:.cc=.o)

.PHONY: all clean

all: $(TARGET)

# build target from objects
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# explicit dependency
colorpixels.o: include/stb_image.h common.h

# compile C++ source files
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
