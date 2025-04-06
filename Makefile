# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra \
    -I/nix/store/061xk2id0csp7kyq2yw8vbbgs602hvj7-libavif-1.1.1/include \
    -I/nix/store/crd3hb8imxvyp88cvdw0nyhzklpn05zn-libwebp-1.4.0/include \
    -I/nix/store/2ac361gmhap35sn83j0ljvpw60h1mxyr-cli11-2.3.2/include
LIBS = -L/nix/store/061xk2id0csp7kyq2yw8vbbgs602hvj7-libavif-1.1.1/lib \
    -L/nix/store/crd3hb8imxvyp88cvdw0nyhzklpn05zn-libwebp-1.4.0/lib \
    -lavif -lwebp -lm

TARGET = colorpixels
SRCS = colorpixels.cc

.PHONY: all clean

all: $(TARGET)

# clone only once if missing
stb/.git:
	git clone https://github.com/nothings/stb stb

stb_image.h: stb/.git
	ln -sf stb/stb_image.h .

$(TARGET): stb_image.h $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@ $(LIBS)

clean:
	rm -rf result stb stb_image.h $(TARGET)
