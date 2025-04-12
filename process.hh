#pragma once
#include <atomic>
#include <string>

#include "lut.hh"

struct Result {
    std::string output;
    std::atomic<bool> ready{false};
};

void process_one(
    const std::string &filename, bool reverse, bool want_maxc, bool print_fname,
    Result &res,
    const std::array<std::array<uint16_t, BLOCKS>, BLOCKS> &gray_range_LUT);
