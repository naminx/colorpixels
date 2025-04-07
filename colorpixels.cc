#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <avif/avif.h>
#include <webp/decode.h>

#include <CLI/CLI.hpp>
#include <iostream>
#include <fstream>
#include <string_view>
#include <vector>
#include <memory>
#include <cmath>
#include <format>
#include <span>
#include <cstdint>
#include <cstring>

constexpr double default_threshold = 11.0;

double sRGB_to_linear(double c) {
    return (c <= 0.04045) ? (c / 12.92) : std::pow((c + 0.055) / 1.055, 2.4);
}

double computeLCHChromaSquare(unsigned char r, unsigned char g, unsigned char b) {
    double R = sRGB_to_linear(r / 255.0);
    double G = sRGB_to_linear(g / 255.0);
    double B = sRGB_to_linear(b / 255.0);

    double X = R*0.4124564 + G*0.3575761 + B*0.1804375;
    double Y = R*0.2126729 + G*0.7151522 + B*0.0721750;
    double Z = R*0.0193339 + G*0.1191920 + B*0.9503041;

    constexpr double Xn = 0.95047, Yn = 1.0, Zn = 1.08883;
    X /= Xn; Y /= Yn; Z /= Zn;

    auto f = [](double t) { return (t > 0.008856) ? std::cbrt(t) : (7.787 * t + 16.0 / 116.0); };
    double fx = f(X), fy = f(Y), fz = f(Z);

    double a = 500.0 * (fx - fy);
    double b2 = 200.0 * (fy - fz);
    return a*a + b2*b2;
}

bool isAboveThreshold(unsigned char r, unsigned char g, unsigned char b)
{
    int sum = r + g + b;
    int diffSum = abs((int)r - (int)g) + abs((int)g - (int)b) + abs((int)b - (int)r);

    int threshold;
    if (sum < 240)
        threshold = 45;
    else if (sum < 390)
        threshold = 50;
    else
        threshold = 55;

    return diffSum > threshold;
}

enum class FileType { AVIF, WebP, Other, Unknown };

FileType detect_file_type(std::span<const uint8_t> data) {
    if(data.size() >= 12) {
        if(std::memcmp(data.data(), "RIFF", 4) == 0 &&
           std::memcmp(data.data() + 8, "WEBP", 4) == 0) {
            return FileType::WebP;
        }

        const uint8_t* p = data.data();
        uint32_t box_size = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
        if(box_size >= 16 && data.size() >= box_size) {
            if(std::memcmp(p+4, "ftyp", 4)==0 &&
               std::memcmp(p+8, "avif", 4)==0) {
                return FileType::AVIF;
            }
            // optionally parse compatible brands here
        }
    }
    return FileType::Other;
}

bool decode_avif(std::string_view filename, unsigned char*& pixels, int& width, int& height) {
    std::ifstream in(std::string(filename), std::ios::binary | std::ios::ate);
    if (!in) return false;
    size_t size = static_cast<size_t>(in.tellg());
    in.seekg(0, std::ios::beg);

    std::vector<uint8_t> buf(size);
    if (!in.read(reinterpret_cast<char*>(buf.data()), size)) return false;

    avifImage* image = avifImageCreateEmpty();
    if (!image) return false;

    avifDecoder* decoder = avifDecoderCreate();
    if (!decoder) {
        avifImageDestroy(image);
        return false;
    }

    avifResult res = avifDecoderReadMemory(decoder, image, buf.data(), size);
    if (res != AVIF_RESULT_OK) {
        avifDecoderDestroy(decoder);
        avifImageDestroy(image);
        return false;
    }

    width = image->width;
    height = image->height;

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, image);
    rgb.format = AVIF_RGB_FORMAT_RGB;
    rgb.depth = 8;
    rgb.rowBytes = width * 3;

    // Allocate RGB buffer in libavif
    avifResult allocRes = avifRGBImageAllocatePixels(&rgb);
    if (allocRes != AVIF_RESULT_OK) {
        avifDecoderDestroy(decoder);
        avifImageDestroy(image);
        return false;
    }

    avifResult convertRes = avifImageYUVToRGB(image, &rgb);
    if (convertRes != AVIF_RESULT_OK) {
        avifRGBImageFreePixels(&rgb);  // avoid leak of allocated pixel buffer
        avifDecoderDestroy(decoder);
        avifImageDestroy(image);
        return false;
    }
    
    // Allocate our own buffer of size width*height*3
    pixels = static_cast<unsigned char*>(malloc(width * height * 3));
    if (!pixels) {
        avifRGBImageFreePixels(&rgb);
        avifDecoderDestroy(decoder);
        avifImageDestroy(image);
        return false;
    }

    memcpy(pixels, rgb.pixels, width * height * 3);

    avifRGBImageFreePixels(&rgb);
    avifDecoderDestroy(decoder);
    avifImageDestroy(image);
    return true;
}

bool decode_webp(std::string_view filename, unsigned char*& pixels, int& width, int& height) {
    std::ifstream in(std::string(filename), std::ios::binary | std::ios::ate);
    if (!in) return false;
    auto size = static_cast<size_t>(in.tellg());
    in.seekg(0, std::ios::beg);

    std::vector<uint8_t> buf(size);
    if (!in.read(reinterpret_cast<char*>(buf.data()), size)) return false;

    int w=0,h=0;
    uint8_t* result = WebPDecodeRGB(buf.data(), size, &w, &h);
    if (!result) return false;

    pixels = result;
    width = w;
    height = h;
    return true;
}

int main(int argc, char *argv[]) {
    CLI::App app{"LCh chroma detector"};

    std::vector<std::string> filenames;
    // double threshold = default_threshold;

    app.add_option("files", filenames, "Image filename(s)")->required();
    // app.add_option("-t,--threshold", threshold, "Chroma threshold (default 11)");

    CLI11_PARSE(app, argc, argv);

    for(const auto& filename : filenames) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file '" << filename << "'\n";
            continue;
        }
        std::vector<uint8_t> header(4096);
        file.read(reinterpret_cast<char*>(header.data()), header.size());
        header.resize(file.gcount());

        FileType ftype = detect_file_type(header);

        int width=0, height=0, channels=0;
        unsigned char *pixels = nullptr;
        bool ok = false;

        if(ftype == FileType::AVIF) {
            ok = decode_avif(filename, pixels, width, height);
        } else if(ftype == FileType::WebP) {
            ok = decode_webp(filename, pixels, width, height);
        } else {
            pixels = stbi_load(filename.c_str(), &width, &height, &channels, 3);
            ok = (pixels != nullptr);
        }

        if (!ok) {
            std::cerr << "Failed to decode image '" << filename << "'\n";
            continue;
        }

        std::size_t total_pixels = static_cast<size_t>(width) * height;
        std::size_t count = 0;

        // const double thresholdSquare = threshold*threshold;
        for(std::size_t i = 0; i < total_pixels; ++i) {
            unsigned char r = pixels[3*i + 0];
            unsigned char g = pixels[3*i + 1];
            unsigned char b = pixels[3*i + 2];
            // double chroma = computeLCHChromaSquare(r, g, b);
            // if(chroma > thresholdSquare) ++count;
            if (isAboveThreshold(r, b, g)) ++count;
        }

        std::cout << std::format("{:.6f}\n", static_cast<double>(count)/total_pixels);

        stbi_image_free(pixels);
    }

    return 0;
}
