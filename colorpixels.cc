#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "common.h"

#include <avif/avif.h>
#include <webp/decode.h>

#include <atomic>
#include <CLI/CLI.hpp>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

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

// Precompute reciprocals for faster reciprocal multiplication
constexpr float inv_1p5 = 1.0f / 1.5f;
constexpr float inv_Xn = 1.0f / 0.95047f;   // D65 Xn
constexpr float inv_Yn = 1.0f / 1.0f;       // D65 Yn, =1.0, so fine
constexpr float inv_Zn = 1.0f / 1.08883f;   // D65 Zn

/// Lookup/interpolate f_xyz without pow
inline float f_xyz(float v) {
    int idx = std::clamp(
        int(v * inv_1p5 * (FSIZE - 1) + 0.5f), 0, FSIZE - 1);
    return f_table[idx];
}

/// Compute chroma given 8-bit srgb inputs, with all float math
inline float computeChroma(uint8_t r, uint8_t g, uint8_t b) {
    float X = R_X[r] + G_X[g] + B_X[b];
    float Y = R_Y[r] + G_Y[g] + B_Y[b];
    float Z = R_Z[r] + G_Z[g] + B_Z[b];

    float fx = f_xyz(X * inv_Xn);
    float fy = f_xyz(Y * inv_Yn);   // or just `Y`
    float fz = f_xyz(Z * inv_Zn);

    float a = 500.0f * (fx - fy);
    float b2 = 200.0f * (fy - fz);

    return std::sqrt(a * a + b2 * b2);
}

constexpr int BLOCKS = 64;
constexpr int BLOCKSIZE = 4;

struct OutputEntry {
    std::string text;
    std::atomic<bool> ready{false};  // published flag
};

inline std::string shell_escape(const std::string& name) {
    std::string out;
    out.reserve(name.size()+2);
    out.push_back('\'');
    for (char c : name) {
        if (c == '\'')
            out += "'\\''";   // end quote, escaped quote, reopen quote
        else
            out.push_back(c);
    }
    out.push_back('\'');
    return out;
}

int main(int argc, char* argv[]) {
    CLI::App app{"LCh chroma detector"};

    std::vector<std::string> filenames;
    float chroma_threshold = 5.0f;
    bool accept_sepia = false;
    bool print_filename = false;

    app.add_option("files", filenames, "Image filename(s)")->required();
    app.add_option("-c,--chroma", chroma_threshold, "Set chroma threshold for gray detection")->check(CLI::PositiveNumber);
    app.add_flag("-s,--sepia", accept_sepia, "Use wider chroma threshold (~13)");
    app.add_flag("-p,--print-filename", print_filename, "Print filename before ratio");

    CLI11_PARSE(app, argc, argv);
    if(accept_sepia) chroma_threshold = 13.0f;

    // ------------ build LUT once ----------------
    static uint16_t gray_range_LUT[BLOCKS][BLOCKS]{};
    for (int Rb = 0; Rb < BLOCKS; ++Rb)
    for (int Gb = 0; Gb < BLOCKS; ++Gb) {
        int rmin = Rb * BLOCKSIZE, rmax = rmin + BLOCKSIZE - 1;
        int gmin = Gb * BLOCKSIZE, gmax = gmin + BLOCKSIZE - 1;
        uint8_t minB=255, maxB=0;
        for (int r = rmin; r <= rmax; ++r)
        for (int g = gmin; g <= gmax; ++g)
        for (int b = 0; b < 256; ++b) {
            float chroma = computeChroma(uint8_t(r), uint8_t(g), uint8_t(b));
            if(chroma < chroma_threshold) {
                if(b < minB) minB = b;
                if(b > maxB) maxB = b;
            }
        }
        gray_range_LUT[Rb][Gb] = (uint16_t(minB)<<8) | maxB;
    }

    // --------- output storage and threads -----------
    std::vector<OutputEntry> outputs(filenames.size());
    std::vector<std::thread> threads;

    for(size_t idx=0; idx < filenames.size(); ++idx) {
        threads.emplace_back([&, idx]() {
            const std::string& filename = filenames[idx];
            std::stringstream ss;

            std::ifstream file(filename, std::ios::binary);
            if (!file) {
                ss << "Failed_to_open ";
                if (print_filename) ss << shell_escape(filename);
                ss << "\n";
                outputs[idx].text = ss.str();
                outputs[idx].ready.store(true, std::memory_order_release);
                return;
            }

            std::vector<uint8_t> header(4096);
            file.read(reinterpret_cast<char*>(header.data()), header.size());
            header.resize(file.gcount());

            FileType ftype = detect_file_type(header);

            int width=0, height=0, channels=0;
            unsigned char* pixels = nullptr;
            bool ok = false;

            if(ftype == FileType::AVIF)
                ok = decode_avif(filename, pixels, width, height);
            else if(ftype == FileType::WebP)
                ok = decode_webp(filename, pixels, width, height);
            else {
                pixels = stbi_load(filename.c_str(), &width, &height, &channels, 3);
                ok = pixels != nullptr;
            }

            if(!ok) {
                ss << "Failed_to_decode ";
                if (print_filename) ss << shell_escape(filename);
                ss << "\n";
                outputs[idx].text = ss.str();
                outputs[idx].ready.store(true, std::memory_order_release);
                return;
            }

            std::size_t total_pixels = std::size_t(width) * height;
            std::size_t count = 0;

            for(std::size_t i = 0; i < total_pixels; ++i){
                uint8_t r = pixels[3*i+0];
                uint8_t g = pixels[3*i+1];
                uint8_t b = pixels[3*i+2];

                int Rb = r >> 2;
                int Gb = g >> 2;
                auto packed = gray_range_LUT[Rb][Gb];
                uint8_t minB = packed >> 8;
                uint8_t maxB = packed & 0xFF;

                bool is_gray = (minB <= b && b <= maxB);
                if(!is_gray) ++count;
            }

            stbi_image_free(pixels);

            float ratio = static_cast<float>(count)/total_pixels;
            if (print_filename) {
                ss << shell_escape(filename) << ' ' << std::format("{:.6f}\n", ratio);
            } else {
                ss << std::format("{:.6f}\n", ratio);
            }

            outputs[idx].text = ss.str();
            outputs[idx].ready.store(true, std::memory_order_release);
        });
    }

    // -------- print outputs as soon as available sequentially ------
    for(size_t next = 0; next < outputs.size(); ++next) {
        // spin/wait until ready
        while(!outputs[next].ready.load(std::memory_order_acquire))
            std::this_thread::yield();
        std::cout << outputs[next].text;
    }

    for(auto& t : threads) t.join();

    return 0;
}
