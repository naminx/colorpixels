#pragma once
#include <cstdint>

// Externally supplied optimized conversion data and preset LUTs
extern float R_X[256],R_Y[256],R_Z[256];
extern float G_X[256],G_Y[256],G_Z[256];
extern float B_X[256],B_Y[256],B_Z[256];
extern float f_table[];

extern uint16_t gray_range_LUT5[64][64];
extern uint16_t gray_range_LUT13[64][64];

void prepare_lut(float chroma_threshold);

// final 64x64 LUT for gray range threshold, to be used after prepare_lut()
extern uint16_t gray_range_LUT[64][64];

float computeChromaSquare(uint8_t r, uint8_t g, uint8_t b);

void dump_lut(int threshold);
