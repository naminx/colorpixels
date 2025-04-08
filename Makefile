CXX = g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra $(CPPFLAGS)
LIBS = $(LDFLAGS) -lavif -lwebp -lm
TARGET = colorpixels
SRCS = colorpixels.cc
.PHONY: all clean

all: $(TARGET)

$(TARGET): include/stb_image.h $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@ $(LIBS)

include/stb_image.h:
	mkdir -p include
	rm -rf stb
	git clone https://github.com/nothings/stb
	rm -rf include/stb
	mv stb include
	ln -sf stb/stb_image.h include/

clean:
	rm -rf $(TARGET)
