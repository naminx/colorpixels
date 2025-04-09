#include "process.h"
#include "lut.h"
#include "decoder.h"

#include <sstream>
#include <format>

void process_one(const std::string& filename,
                 bool reverse, bool want_maxc, bool print_fname,
                 Result& entry)
{
    unsigned char* pix=nullptr;
    int w=0,h=0;
    bool ok=decode_image(filename,pix,w,h);
    std::stringstream ss;
    if(!ok){
        ss<<"ERROR decoding "<<filename<<"\n";
        entry.output = ss.str();
        entry.ready.store(true,std::memory_order_release);
        return;
    }
    size_t N=size_t(w)*h, count=0;
    float max_c2 = 0.f;   // stores maximal squared chroma
    for(size_t i=0; i<N; ++i){
        uint8_t r=pix[3*i], g=pix[3*i+1], b=pix[3*i+2];
        auto mm=gray_range_LUT[r>>2][g>>2];
        uint8_t minB=mm>>8, maxB=mm&0xff;
        if(b<minB||b>maxB) ++count;

        float sq = computeChromaSquare(r,g,b);
        if(sq > max_c2) max_c2 = sq;
    }
    stbi_image_free(pix);

    float ratio = N ? float(count)/N : 0.f;

    uint8_t maxc_ui8 = uint8_t( std::sqrt(max_c2) + 0.5f );  // final max chroma (rounded int)

    if(reverse){
        ss << std::format("{:.6g}", ratio);
        if(want_maxc) ss << " " << int(maxc_ui8);
        if(print_fname) ss << " " << filename;
    }else{
        if(print_fname) ss << filename << " ";
        ss << std::format("{:.6g}", ratio);
        if(want_maxc) ss << " " << int(maxc_ui8);
    }

    ss << "\n";
    entry.output = ss.str();
    entry.ready.store(true,std::memory_order_release);
}
