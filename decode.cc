#include "decoder.h"
#include <avif/avif.h>
#include <webp/decode.h>
#include "stb_image.h"
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

namespace {
enum class FileType { AVIF, WebP, Other };

FileType detect_type(const std::vector<uint8_t>& header) {
    if(header.size() >=12) {
        if(!memcmp(header.data(), "RIFF", 4) && !memcmp(header.data()+8, "WEBP",4)) return FileType::WebP;
        const uint8_t* p=header.data();
        uint32_t size = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
        if(size>=16 && header.size() >= size) {
            if(!memcmp(p+4,"ftyp",4)&&!memcmp(p+8,"avif",4)) return FileType::AVIF;
        }
    }
    return FileType::Other;
}

bool decode_avif(const std::vector<uint8_t>& buf, unsigned char*& pixels, int& w, int& h){
    auto dec = avifDecoderCreate();
    auto img = avifImageCreateEmpty();
    if(!dec||!img){if(dec)avifDecoderDestroy(dec);if(img)avifImageDestroy(img);return false;}
    if(avifDecoderReadMemory(dec,img,buf.data(),buf.size())!=AVIF_RESULT_OK){avifDecoderDestroy(dec);avifImageDestroy(img);return false;}
    w=img->width; h=img->height;
    avifRGBImage rgb; avifRGBImageSetDefaults(&rgb,img);
    rgb.format=AVIF_RGB_FORMAT_RGB; rgb.depth=8; rgb.rowBytes=w*3;
    if(avifRGBImageAllocatePixels(&rgb)!=AVIF_RESULT_OK){avifDecoderDestroy(dec);avifImageDestroy(img);return false;}
    if(avifImageYUVToRGB(img,&rgb)!=AVIF_RESULT_OK){avifRGBImageFreePixels(&rgb);avifDecoderDestroy(dec);avifImageDestroy(img);return false;}
    pixels=(unsigned char*)malloc(w*h*3);
    memcpy(pixels,rgb.pixels,w*h*3);
    avifRGBImageFreePixels(&rgb); avifDecoderDestroy(dec); avifImageDestroy(img);
    return true;
}
}

bool decode_image(std::string_view fname, unsigned char*& pixels, int& w, int& h){
    std::ifstream in(std::string(fname),std::ios::binary|std::ios::ate);
    if(!in) return false;
    size_t sz=in.tellg();
    in.seekg(0);
    std::vector<uint8_t> buf(sz);
    if(!in.read((char*)buf.data(),sz)) return false;
    int tryw=0, tryh=0;
    FileType tp = detect_type(buf);
    if(tp==FileType::AVIF){
        return decode_avif(buf,pixels,w,h);
    }
    if(tp==FileType::WebP){
        pixels=WebPDecodeRGB(buf.data(),buf.size(),&tryw,&tryh);
        if(pixels){w=tryw;h=tryh; return true;}
        return false;
    }
    // fallback to other
    pixels=stbi_load(fname.data(),&tryw,&tryh,nullptr,3);
    if(pixels){w=tryw;h=tryh; return true;}
    return false;
}
