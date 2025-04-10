#include <cmath>
#include <format>
#include <sstream>

#include "decode.hh"
#include "lut.hh"
#include "process.hh"

void process_one(
    const std::string &filename, bool reverse, bool want_maxc, bool print_fname,
    Result &entry,
    const std::array<std::array<uint16_t, BLOCKS>, BLOCKS> &gray_range_LUT)
{
  int w{0};
  int h{0};
  const SmartPixelsPtr pix{decode_image(filename, w, h)};
  std::stringstream ss;
  if (!pix) {
    ss << "ERROR decoding " << filename << "\n";
    entry.output = ss.str();
    entry.ready.store(true, std::memory_order_release);
    return;
  }
  const size_t N{size_t(w) * h};
  size_t count{0};
  float maxc_square{0.f}; // stores maximal squared chroma
  for (size_t i = 0; i < N; ++i) {
    const uint8_t r{pix[3 * i]};
    const uint8_t g{pix[3 * i + 1]};
    const uint8_t b{pix[3 * i + 2]};
    const auto mm{gray_range_LUT[r >> 2][g >> 2]};
    const uint8_t minB{uint8_t(mm >> 8)};
    const uint8_t maxB{uint8_t(mm & 0xff)};
    if (b < minB || b > maxB)
      ++count;

    const float sq{compute_chroma_square(r, g, b)};
    if (sq > maxc_square)
      maxc_square = sq;
  }

  const float ratio{N ? float(count) / N : 0.f};

  const float maxc{std::sqrt(maxc_square)};

  if (reverse) {
    if (want_maxc)
      ss << std::format("{:.6f}", maxc);
    else
      ss << std::format("{:.6f}", ratio);
    if (print_fname)
      ss << " " << filename;
  } else {
    if (print_fname)
      ss << filename << " ";
    if (want_maxc)
      ss << std::format("{:.6f}", maxc);
    else
      ss << std::format("{:.6f}", ratio);
  }

  ss << "\n";
  entry.output = ss.str();
  entry.ready.store(true, std::memory_order_release);
}
