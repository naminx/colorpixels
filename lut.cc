#include "lut.h"
#include "common.h"
#include <cstring>
#include <cmath>
#include <cstdio>

constexpr float inv_1p5 = 1.0f/1.5f;
constexpr float inv_Xn =1.0f/0.95047f;
constexpr float inv_Yn=1.0f;
constexpr float inv_Zn=1.0f/1.08883f;

constexpr int FSIZE=512;
constexpr int BLOCKSIZE=4;
constexpr int BLOCKS=64;

uint16_t gray_range_LUT[64][64]{};

static inline float f_xyz(float v){
    int idx=std::clamp(int(v*inv_1p5*(FSIZE-1)+0.5f),0,FSIZE-1);
    return f_table[idx];
}

float computeChromaSquare(uint8_t r, uint8_t g, uint8_t b){
    float X=R_X[r]+G_X[g]+B_X[b];
    float Y=R_Y[r]+G_Y[g]+B_Y[b];
    float Z=R_Z[r]+G_Z[g]+B_Z[b];
    float fx = f_xyz(X*inv_Xn);
    float fy = f_xyz(Y*inv_Yn);
    float fz = f_xyz(Z*inv_Zn);
    float a = 500*(fx-fy), b2=200*(fy-fz);
    return a*a + b2*b2;   // squared chroma, no sqrt for efficiency
}

void prepare_lut(float chroma_thresh){
    if(chroma_thresh==5.f){
        memcpy(gray_range_LUT,gray_range_LUT5,sizeof gray_range_LUT);
        return;
    }
    if(chroma_thresh==13.f){
        memcpy(gray_range_LUT,gray_range_LUT13,sizeof gray_range_LUT);
        return;
    }
    // WHY: only compute per threshold when not preset for speed on defaults
    for(int Rb=0;Rb<BLOCKS;++Rb)
    for(int Gb=0;Gb<BLOCKS;++Gb){
        int r0=Rb*BLOCKSIZE, g0=Gb*BLOCKSIZE;
        uint8_t minB=255,maxB=0;
        for(int r=r0;r<r0+BLOCKSIZE;++r)
        for(int g=g0;g<g0+BLOCKSIZE;++g)
        for(int b=0;b<256;++b){
            float c=computeChroma(r,g,b);
            if(c<chroma_thresh){
                if(b<minB)minB=b;
                if(b>maxB)maxB=b;
            }
        }
        gray_range_LUT[Rb][Gb]=(minB<<8)|maxB;
    }
}

void dump_lut(int threshold){
    prepare_lut(threshold);
    for(int r=0;r<64;++r){
        for(int g=0;g<64;++g){
            printf("%04x%s",gray_range_LUT[r][g],(g==63?"\n":" "));
        }
    }
}
