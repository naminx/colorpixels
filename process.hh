#pragma once
#include <atomic> // WHY: For atomic<bool> flag for thread synchronization.
#include <string>

#include "lut.hh" // Includes definition of chroma_lut_t

// Holds the formatted output string and a ready flag for a single image processing task.
struct processing_result {
    std::string output; // Pre-formatted output string (result or error).
    // WHY atomic? Ensures safe communication of ready status between threads without explicit locks.
    std::atomic<bool> is_ready{false};
};

// Function signature for processing a single image file.
// Takes parameters controlling output format and the precomputed LUT.
// Modifies the passed processing_result struct.
void process_image_file(const std::string &filename, bool reverse_output_order, bool report_max_chroma, bool print_filename,
                        processing_result &result_entry, const chroma_lut_t &chroma_check_lut);
