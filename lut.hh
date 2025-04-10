#pragma once
#include <array>
#include <cstdint>
#include <iostream>

constexpr int BLOCKS = 64;
constexpr int BLOCKSIZE = 4;
constexpr int FSIZE = 512;
constexpr int LUTSIZE = 256;

const std::array<std::array<uint16_t, BLOCKS>, BLOCKS> &
get_lookup_table(float chroma_thresh);

float compute_chroma_square(uint8_t r, uint8_t g, uint8_t b);
double compute_chroma(uint8_t r, uint8_t g, uint8_t b);

void dump_lookup_table(int threshold, std::ostream &out = std::cout);
