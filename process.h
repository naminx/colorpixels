#pragma once
#include <string>
#include <cstdint>

struct Result {
    std::string output;
    std::atomic<bool> ready{false};
};

void process_one(const std::string& filename,
                 bool reverse, bool want_maxc, bool print_fname,
                 Result& res);
