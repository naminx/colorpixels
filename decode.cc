#include <avif/avif.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <vector>
#include <webp/decode.h>

#include "decode.hh"
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

namespace {

enum class FileType { AVIF, WebP, Other };

FileType detect_type(const std::vector<uint8_t> &header)
{
    if (header.size() >= 12) {
        if (!memcmp(header.data(), "RIFF", 4) &&
            !memcmp(header.data() + 8, "WEBP", 4))
            return FileType::WebP;
        const uint8_t *p{header.data()};
        uint32_t box_size{(uint32_t(p[0]) << 24) | (p[1] << 16) | (p[2] << 8) |
                          p[3]};
        if (box_size >= 16 && header.size() >= box_size) {
            if (!memcmp(p + 4, "ftyp", 4) && !memcmp(p + 8, "avif", 4))
                return FileType::AVIF;
        }
    }
    return FileType::Other;
}

// Unique_ptr for avifDecoder
using DecoderPtr = std::unique_ptr<avifDecoder, decltype(&avifDecoderDestroy)>;
using ImagePtr = std::unique_ptr<avifImage, decltype(&avifImageDestroy)>;
using AvifImagePtr =
    std::unique_ptr<avifRGBImage, decltype(&avifRGBImageFreePixels)>;

SmartPixelsPtr decode_avif(const std::vector<uint8_t> &buf, int &w, int &h)
{
    // manage decoder and input image
    DecoderPtr dec(avifDecoderCreate(), avifDecoderDestroy);
    if (!dec)
        return {};

    ImagePtr img(avifImageCreateEmpty(), avifImageDestroy);
    if (!img)
        return {};

    if (avifDecoderReadMemory(dec.get(), img.get(), buf.data(), buf.size()) !=
        AVIF_RESULT_OK)
        return {};

    w = img->width;
    h = img->height;

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, img.get());
    rgb.format = AVIF_RGB_FORMAT_RGB;
    rgb.depth = 8;
    rgb.rowBytes = w * 3;

    if (avifRGBImageAllocatePixels(&rgb) != AVIF_RESULT_OK)
        return {};

    if (avifImageYUVToRGB(img.get(), &rgb) != AVIF_RESULT_OK)
        return {};

    return SmartPixelsPtr(rgb.pixels, [rgb_copy = rgb](uint8_t *) mutable {
        avifRGBImageFreePixels(&rgb_copy);
    });
}

SmartPixelsPtr decode_webp(const std::vector<uint8_t> &buf, int &w, int &h)
{
    int ww{0};
    int hh{0};

    SmartPixelsPtr pixels{reinterpret_cast<uint8_t *>(
                              WebPDecodeRGB(buf.data(), buf.size(), &ww, &hh)),
                          [](uint8_t *p) { WebPFree(p); }};

    if (!pixels)
        return {};

    w = ww;
    h = hh;
    return pixels;
}

SmartPixelsPtr decode_other(const std::vector<uint8_t> &buf, int &w, int &h)
{
    int ww{0};
    int hh{0};

    SmartPixelsPtr pixels{
        stbi_load_from_memory(buf.data(), buf.size(), &ww, &hh, nullptr, 3),
        [](uint8_t *p) { stbi_image_free(p); }};

    if (!pixels)
        return {};

    w = ww;
    h = hh;
    return pixels;
}

} // namespace

SmartPixelsPtr decode_image(std::string_view fname, int &width, int &height)
{
    std::ifstream in(std::string(fname), std::ios::binary | std::ios::ate);
    if (!in)
        return {};

    size_t sz{size_t(in.tellg())};
    in.seekg(0);
    std::vector<uint8_t> buf(sz);
    if (!in.read((char *)buf.data(), sz))
        return {};

    FileType tp{detect_type(buf)};
    SmartPixelsPtr pixels;

    if (tp == FileType::AVIF) {
        pixels = decode_avif(buf, width, height);
        if (pixels)
            return pixels;
    }

    if (tp == FileType::WebP) {
        pixels = decode_webp(buf, width, height);
        if (pixels)
            return pixels;
    }

    // fallback: try stb_image
    pixels = decode_other(buf, width, height);
    return pixels;
}
