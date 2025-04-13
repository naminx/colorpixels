#pragma once
#include <array>
#include <cstdint>
#include <iostream> // WHY: For std::ostream default in dump_lookup_table.

// Constants defining the structure of the R-G lookup table.
constexpr int RG_LUT_BLOCKS = 64; // WHY 64? LUT dimension (64x64), R & G are divided by 4 (256/4 = 64).
constexpr int RG_BLOCK_SIZE = 4;  // WHY 4? Size of R/G block aggregated per LUT entry (4x4 = 16 R,G pairs).
// constexpr int FSIZE = 512; // This constant seems unused, removed.
constexpr int RGB_LUT_SIZE = 256; // WHY 256? Size needed for direct lookup using 8-bit R,G,B values.

// Type alias for the 2D LUT array. Stores packed min/max B values.
using chroma_lut_t = std::array<std::array<uint16_t, RG_LUT_BLOCKS>, RG_LUT_BLOCKS>;

// Retrieves or generates the lookup table for a given chroma threshold.
const chroma_lut_t &get_chroma_lut(float chroma_threshold);

// Calculates squared chroma using precomputed tables (optimized).
float compute_chroma_squared(uint8_t r_srgb, uint8_t g_srgb, uint8_t b_srgb);

// Calculates actual chroma (slower, used for LUT generation).
double compute_chroma(uint8_t r_srgb, uint8_t g_srgb, uint8_t b_srgb);

// Dumps the lookup table for a given threshold to an output stream (defaults to std::cout).
void dump_lookup_table(int threshold, std::ostream &output_stream = std::cout);
