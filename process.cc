#include "process.hh"

#include <cmath>
#include <format>  // WHY: Modern C++ way for type-safe text formatting.
#include <sstream> // WHY: Convenient for building the output string incrementally.

#include "decode.hh"
#include "lut.hh"

// Processes a single image file to determine color ratio or max chroma.
void process_image_file(const std::string &filename, bool file_names_only, bool report_max_chroma,
                        std::optional<float> greater_than, std::optional<float> less_than, bool print_filename,
                        processing_result &result_entry, const chroma_lut_t &chroma_check_lut)
{
    int image_width{0};
    int image_height{0};

    // --- Decode Image ---
    // WHY unique_ptr (smart_pixels_ptr)? Manages pixel buffer lifetime automatically (RAII).
    const smart_pixels_ptr pixels{decode_image(filename, image_width, image_height)};

    // Prepare output stream for results or errors.
    std::stringstream output_stream;

    // WHY check pixels? Decoding can fail; handle gracefully.
    if (!pixels) {
        output_stream << "ERROR decoding " << filename << "\n";
        result_entry.output = output_stream.str();
        // WHY atomic store? Signal main thread that this result is ready (with error).
        // std::memory_order_release ensures preceding writes (like output string) are visible
        // to the acquiring thread.
        result_entry.is_ready.store(true, std::memory_order_release);
        return;
    }

    // --- Analyze Pixels ---
    const size_t total_pixels{static_cast<size_t>(image_width) * image_height};
    size_t colored_pixel_count{0};
    // WHY float? Chroma calculation involves floating point.
    // WHY squared? Avoids sqrt in the loop for performance; compare threshold squared later.
    float max_chroma_squared{0.f};

    for (size_t i = 0; i < total_pixels; ++i) {
        // Assuming RGB layout: R=pix[3*i], G=pix[3*i+1], B=pix[3*i+2]
        const uint8_t r{pixels[3 * i + 0]};
        const uint8_t g{pixels[3 * i + 1]};
        const uint8_t b{pixels[3 * i + 2]};

        // --- Chroma Check using LUT ---
        // WHY bit shifts (>> 2)? Divides R and G by 4 to get index into 64x64 LUT.
        // This effectively groups 4x4 blocks of R and G values.
        const uint16_t min_max_b_packed{chroma_check_lut[r >> 2][g >> 2]};
        // WHY bit shifts and masking? Extracts the precomputed min/max B values
        // packed into the uint16_t for the given R,G block.
        const uint8_t min_b_for_gray{static_cast<uint8_t>(min_max_b_packed >> 8)};   // High byte
        const uint8_t max_b_for_gray{static_cast<uint8_t>(min_max_b_packed & 0xff)}; // Low byte

        // WHY check range? If B is outside the precomputed [min, max] range for this R,G block,
        // the pixel's chroma MUST exceed the threshold used to generate the LUT. This is the
        // core optimization - avoids expensive chroma calculation for most pixels.
        if (b < min_b_for_gray || b > max_b_for_gray) {
            colored_pixel_count++;
        }

        // --- Max Chroma Tracking (if needed) ---
        // WHY compute chroma separately? Only needed if max chroma output is requested.
        if (report_max_chroma) {
            // WHY squared? Avoids sqrt until the very end for performance.
            const float current_chroma_squared = compute_chroma_squared(r, g, b);
            // WHY compare squared? Faster than comparing sqrt(value).
            if (current_chroma_squared > max_chroma_squared) {
                max_chroma_squared = current_chroma_squared;
            }
        }
    }

    // --- Format Output ---
    // WHY check total_pixels? Avoid division by zero for empty/invalid images.
    // const float color_ratio{
    //     !report_max_chroma ? (total_pixels ? static_cast<float>(colored_pixel_count) / total_pixels * 100.0f : 0.f) : 0.f};
    // WHY sqrt here? Only calculate the actual max chroma value once at the end if needed.
    // const float max_chroma{report_max_chroma ? std::sqrt(max_chroma_squared) : 0.f};

    const float report_value{report_max_chroma
                                 ? std::sqrt(max_chroma_squared)
                                 : (total_pixels ? static_cast<float>(colored_pixel_count) / total_pixels * 100.0f : 0.f)};
    result_entry.value = report_value;

    // Print output only if
    if ((!greater_than || report_value > *greater_than) && (!less_than || report_value < *less_than)) {
        if (print_filename || file_names_only)
            output_stream << filename;
        if (!file_names_only) {
            if (print_filename || file_names_only)
                output_stream << " ";
            output_stream << std::format("{:.3f}", report_value);
        }
        output_stream << "\n"; // Ensure newline termination.
    }

    // --- Signal Completion ---
    result_entry.output = output_stream.str();
    // WHY atomic store? Signal main thread that this result is ready (successfully).
    result_entry.is_ready.store(true, std::memory_order_release);
}
